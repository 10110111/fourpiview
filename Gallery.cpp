#include "Gallery.hpp"
#include <QTimer>
#include <QScroller>
#include <QGridLayout>
#include "ImageFinder.hpp"

namespace
{

#ifdef Q_OS_ANDROID
constexpr int BASE_ICON_SIZE = 32;
#else
constexpr int BASE_ICON_SIZE = 128;
#endif

enum
{
    FilePathRole = Qt::UserRole,
};

const char thumbnailButtonCSS[] = R"(
QToolButton {
    border: 0px;
    background: rgba(0,0,0,0);
    padding: 0px, 0px, 0px, 0px;
}
)";
}

Gallery::Gallery(QWidget* parent)
    : QScrollArea(parent)
    , thumbnailWidth_(BASE_ICON_SIZE * devicePixelRatio())
    , imageFinder_(new ImageFinder(thumbnailWidth_, this))
    , thumbnailHolder_(new QWidget)
    , layout_(new QGridLayout)
{
    setWidget(thumbnailHolder_);
    setWidgetResizable(true);

    const auto vbox = new QVBoxLayout(thumbnailHolder_);
    vbox->addLayout(layout_);
    vbox->addStretch(10);

    QScroller::grabGesture(viewport(), QScroller::LeftMouseButtonGesture);

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
    const auto button = new QToolButton;
    button->setStyleSheet(thumbnailButtonCSS);
    button->setIcon(emptyIcon_);
    button->setFixedSize(thumbnailWidth_, thumbnailWidth_ / 2);
    button->setIconSize(QSize(thumbnailWidth_, thumbnailWidth_ / 2));
    connect(button, &QToolButton::clicked, this, [this, path]{ handleItemClick(path); });
    pathMap_[path] = button;
    buttonMap_[button] = path;
    QTimer::singleShot(200, [this]{updateLayout();});
}

void Gallery::updateThumbnail(const QString& path, const QImage& thumbnail)
{
    const auto buttonIt = pathMap_.find(path);
    if(buttonIt == pathMap_.end()) return;
    const auto button = buttonIt->second;
    button->setIcon(QIcon(QPixmap::fromImage(thumbnail)));
}

void Gallery::handleItemClick(const QString& path)
{
    emit openFileRequest(path);
}

void Gallery::updateLayout()
{
    thumbnails_.clear();
    for(const auto& th : buttonMap_)
        thumbnails_.push_back({th.second, th.first});
    std::sort(thumbnails_.begin(), thumbnails_.end(),
              [](const auto& a, const auto& b) { return a.path > b.path; });
    const int numCols = std::max(1, int(width() / (thumbnailWidth_ * 1.1)));
    layout_->setColumnStretch(numCols, 10);
    int itemNum = 0;
    for(const auto& th : thumbnails_)
    {
        const int row = itemNum / numCols;
        const int col = itemNum % numCols;
        if(const auto item = layout_->itemAtPosition(row, col))
            layout_->removeItem(item);
        layout_->removeWidget(th.button);
        layout_->addWidget(th.button, row, col);
        ++itemNum;
    }
}

void Gallery::resizeEvent(QResizeEvent*)
{
    updateLayout();
}

Gallery::~Gallery()
{
    imageFinder_->stop();
    imageFinder_->wait();
}
