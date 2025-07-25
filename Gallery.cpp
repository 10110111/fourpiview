#include "Gallery.hpp"
#include <QTimer>
#include <QScroller>
#include <QGridLayout>
#include "ImageFinder.hpp"

namespace
{

#ifdef Q_OS_ANDROID
constexpr int BASE_ICON_SIZE = 48;
#else
constexpr int BASE_ICON_SIZE = 192;
#endif

enum
{
    FilePathRole = Qt::UserRole,
};

}

Gallery::Gallery(QWidget* parent)
    : QListWidget(parent)
    , thumbnailWidth_(BASE_ICON_SIZE * devicePixelRatio())
    , imageFinder_(new ImageFinder(thumbnailWidth_, this))
    , layout_(new QGridLayout)
{
    QScroller::grabGesture(this, QScroller::LeftMouseButtonGesture);
    setViewMode(QListView::IconMode);
    setMovement(QListView::Static);
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

void Gallery::addImage(const QString& path)
{
    const auto item = new QListWidgetItem(this);
    item->setIcon(emptyIcon_);
    item->setData(FilePathRole, path);
    item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
    pathMap_[path] = item;
    itemMap_[item] = path;
    if(pathMap_.size() == 1)
        updateLayout();
}

void Gallery::updateThumbnail(const QString& path, const QImage& thumbnail)
{
    const auto itemIt = pathMap_.find(path);
    if(itemIt == pathMap_.end()) return;
    const auto item = itemIt->second;
    item->setIcon(QIcon(QPixmap::fromImage(thumbnail)));
}

void Gallery::handleItemClick(const QListWidgetItem* item)
{
    emit openFileRequest(item->data(FilePathRole).toString());
}

void Gallery::resizeEvent(QResizeEvent*)
{
    updateLayout();
}

void Gallery::updateLayout()
{
    const int numCols = std::round(double(width()) / (BASE_ICON_SIZE * devicePixelRatio() / 1.5));
    const int spacing = 2;
    const int iconHeight = ((width() - spacing) / numCols - spacing) / 2;
    const QSize iconSize(iconHeight * 2, iconHeight);
    setIconSize(QSize(1,1)); // force re-layout
    setGridSize(iconSize + QSize(spacing, spacing));
    setIconSize(iconSize);
}

Gallery::~Gallery()
{
    imageFinder_->stop();
    imageFinder_->wait();
}
