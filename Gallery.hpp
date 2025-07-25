#pragma once

#include <vector>
#include <QToolButton>
#include <QScrollArea>

class QGridLayout;
class ImageFinder;
class Gallery : public QScrollArea
{
    Q_OBJECT

public:
    Gallery(QWidget* parent = nullptr);
    ~Gallery();

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    void addImage(const QString& path);
    void updateThumbnail(const QString& path, const QImage& thumbnail);
    void handleItemClick(const QString& path);
    void updateLayout();

signals:
    void openFileRequest(const QString& path);

private:
    int thumbnailWidth_;
    ImageFinder* imageFinder_ = nullptr;
    QWidget* thumbnailHolder_ = nullptr;
    QGridLayout* layout_ = nullptr;
    struct Thumbnail
    {
        QString path;
        QToolButton* button;
    };
    std::vector<Thumbnail> thumbnails_;
    std::unordered_map<QString/*path*/, QToolButton*> pathMap_;
    std::unordered_map<QToolButton*, QString/*path*/> buttonMap_;
    QIcon emptyIcon_;
};
