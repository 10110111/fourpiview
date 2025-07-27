#pragma once

#include <atomic>
#include <QThread>
#include <QDateTime>
#include <QStringList>

struct ImageInfo
{
    QString path;
    QDateTime dateTime;
};

class ImageFinder : public QThread
{
    Q_OBJECT

public:
    ImageFinder(int thumbnailWidth, QObject* parent = nullptr);
    void stop();

protected:
    void run() override;

private:
    void findAllImages();
    void loadThumbnails();

signals:
    void imageFound(ImageInfo info);
    void thumbnailReady(QString path, QImage thumbnail);

private:
    std::vector<ImageInfo> imageInfos_;
    int thumbnailWidth_;
    std::atomic_bool mustStop_{false};
};
