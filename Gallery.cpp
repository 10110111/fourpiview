#include "Gallery.hpp"
#include <QTimer>
#include <QPainter>
#include <QScroller>
#include <QGridLayout>
#include <QFontMetrics>
#include "Utils.hpp"
#include "ImageFinder.hpp"

namespace
{

#ifdef Q_OS_ANDROID
constexpr int BASE_ICON_SIZE = 48;
#else
constexpr int BASE_ICON_SIZE = 192;
#endif

constexpr int ICON_SPACING = 2;

enum
{
    FilePathRole = Qt::UserRole,
    ImageDateTimeRole,
};

QDateTime dateTime(const QListWidgetItem*const item)
{
    assert(item);
    return item->data(ImageDateTimeRole).toDateTime();
}

QIcon createDateIcon(const QDate& date, const int width, const int height, const double scale)
{
    QImage img(width * scale, height * scale, QImage::Format_RGBA8888);
    img.fill(Qt::transparent);
    QPainter p(&img);
    const auto string = date.toString("yyyy-MM-dd");
    auto font = p.font();
    font.setPixelSize(100);
    QFontMetrics m(font);
    const auto strWidth = m.horizontalAdvance(string);
    font.setPixelSize(font.pixelSize() * img.width() / strWidth);
    p.setFont(font);
    if(Utils::isDarkMode())
        p.setPen(Qt::white);
    p.drawText(img.rect(), Qt::AlignLeft | Qt::AlignVCenter, string);
    img.setDevicePixelRatio(scale);
    return QIcon(QPixmap::fromImage(img));
}

int compare(const QListWidgetItem*const a, const QListWidgetItem*const b)
{
    const auto dateTimeA = dateTime(a);
    const auto dateTimeB = dateTime(b);
    const auto pathA = a->data(FilePathRole);
    const auto pathB = b->data(FilePathRole);
    const auto normalResult = dateTimeA < dateTimeB ? -1
                                                    : dateTimeA == dateTimeB ?
                                                                           0 : 1;
    if(pathA.isValid() && pathB.isValid())
        return normalResult;

    if(pathA.isValid() && !pathB.isValid())
    {
        if(dateTimeA.date() == dateTimeB.date())
            return -1;
        else
            return normalResult;
    }

    if(!pathA.isValid() && pathB.isValid())
    {
        if(dateTimeA.date() == dateTimeB.date())
            return 1;
        else
            return normalResult;
    }

    return normalResult;
}

}

Gallery::Gallery(QWidget* parent)
    : QListWidget(parent)
    , thumbnailWidth_(BASE_ICON_SIZE * devicePixelRatio())
    , imageFinder_(new ImageFinder(thumbnailWidth_, this))
    , layout_(new QGridLayout)
{
    QScroller::grabGesture(this, QScroller::LeftMouseButtonGesture);
    setViewMode(QListView::IconMode);
    setMovement(QListView::Free);
    setIconSize(QSize(thumbnailWidth_, thumbnailWidth_ / 2));
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    connect(this, &QListWidget::itemClicked, this, &Gallery::handleItemClick);

    QImage empty(1, 1, QImage::Format_RGBA8888);
    for(int n = 0; n < 3; ++n)
        empty.bits()[n] = 127;
    empty.bits()[3] = 255;
    empty = empty.scaled(thumbnailWidth_, thumbnailWidth_ / 2);
    emptyIcon_ = QIcon(QPixmap::fromImage(empty));

    connect(imageFinder_, &ImageFinder::imageFound, this, &Gallery::addImage, Qt::QueuedConnection);
    connect(imageFinder_, &ImageFinder::thumbnailReady, this, &Gallery::updateThumbnail, Qt::QueuedConnection);
    imageFinder_->start();
}

// This function tries to make sure that the items are sorted by datetime in descending order.
int Gallery::findRowForItem(const QListWidgetItem*const newItem) const
{
    const int total = count();
    if(total == 0) return 0;
    if(compare(item(0), newItem) <= 0) return 0;
    if(compare(item(total - 1), newItem) >= 0) return total;

    // Using binary search to find the right position according to datetime.
    // Assuming that all existing items are already in the right order.
    int left = 0, right = total - 1;
    while(right - left > 1)
    {
        const int mid = (right + left) / 2;
        if(compare(item(mid), newItem) < 0)
            right = mid;
        else
            left = mid;
    }

    if(compare(item(left), newItem) > 0) return right;
    return left;
}

void Gallery::addImage(const ImageInfo& info)
{
    const auto item = new QListWidgetItem;
    item->setIcon(emptyIcon_);
    item->setData(FilePathRole, info.path);
    item->setData(ImageDateTimeRole, info.dateTime);
    item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
    const int row = findRowForItem(item);
    insertItem(row, item);
    pathMap_[info.path] = item;
    itemMap_[item] = info.path;

    const auto date = info.dateTime.date();
    if(datesDesignated_.find(date) == datesDesignated_.end())
    {
        const auto dateItem = new QListWidgetItem;
        const auto icon = createDateIcon(date, iconSize().width(),
                                         iconSize().height(), devicePixelRatio());
        dateItem->setIcon(icon);
        dateItem->setData(ImageDateTimeRole, info.dateTime);
        dateItem->setFlags(item->flags() & ~Qt::ItemIsSelectable);
        datesDesignated_[date] = dateItem;
        insertItem(row, dateItem);
    }

    updateLayout();
}

void Gallery::updateThumbnail(const QString& path, const QImage& thumbnail)
{
    const auto itemIt = pathMap_.find(path);
    if(itemIt == pathMap_.end()) return;
    const auto item = itemIt->second;
    assert(item->data(FilePathRole).toString() == path);
    item->setIcon(QIcon(QPixmap::fromImage(thumbnail)));
}

void Gallery::handleItemClick(const QListWidgetItem* item)
{
    if(item->data(FilePathRole).isValid())
        emit openFileRequest(item->data(FilePathRole).toString());
}

void Gallery::resizeEvent(QResizeEvent*)
{
    updateLayout();
}

void Gallery::updateLayout()
{
    const int numCols = std::round(double(width()) / (BASE_ICON_SIZE * devicePixelRatio() / 1.5));
    const int iconHeight = ((width() - ICON_SPACING) / numCols - ICON_SPACING) / 2;
    const QSize iconSize(iconHeight * 2, iconHeight);
    setIconSize(QSize(1,1)); // force re-layout
    setGridSize(iconSize + QSize(ICON_SPACING, ICON_SPACING));
    setIconSize(iconSize);
    int row = 0, col = 0;
    for(int n = 0; n < count(); ++n)
    {
        const auto*const currItem = item(n);
        const auto index = indexFromItem(currItem);
        if(currItem->data(FilePathRole).isValid())
        {
            setPositionForIndex({ICON_SPACING + col * (iconSize.width() + ICON_SPACING),
                                 ICON_SPACING + row * (iconSize.height() + ICON_SPACING)},
                                index);
            ++col;
            if(col >= numCols)
            {
                ++row;
                col = 0;
            }
        }
        else
        {
            if(n && col)
            {
                ++row;
                col = 0;
            }
            setPositionForIndex({ICON_SPACING + col * (iconSize.width() + ICON_SPACING),
                                 ICON_SPACING + row * (iconSize.height() + ICON_SPACING)},
                                index);
            ++row;
        }
    }
}

Gallery::~Gallery()
{
    imageFinder_->stop();
    imageFinder_->wait();
}
