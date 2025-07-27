#pragma once

#include <vector>
#include <QListWidget>

struct ImageInfo;
class QGridLayout;
class ImageFinder;
class Gallery : public QListWidget
{
    Q_OBJECT

public:
    Gallery(QWidget* parent = nullptr);
    ~Gallery();

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    void addImage(const ImageInfo& info);
    void updateThumbnail(const QString& path, const QImage& thumbnail);
    void handleItemClick(const QListWidgetItem* item);
    int findRowForItem(const QListWidgetItem*const item) const;
    void updateLayout();

signals:
    void openFileRequest(const QString& path);

private:
    int thumbnailWidth_;
    ImageFinder* imageFinder_ = nullptr;
    QGridLayout* layout_ = nullptr;
    struct Thumbnail
    {
        QString path;
        QListWidgetItem* item;
    };
    std::vector<Thumbnail> thumbnails_;
    std::unordered_map<QString/*path*/, QListWidgetItem*> pathMap_;
    std::unordered_map<QListWidgetItem*, QString/*path*/> itemMap_;
    QIcon emptyIcon_;
};
