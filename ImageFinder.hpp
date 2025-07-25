#pragma once

#include <atomic>
#include <QThread>
#include <QStringList>

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
    void imageFound(QString path);
    void thumbnailReady(QString path, QImage thumbnail);

private:
    QStringList imagePaths;
    int thumbnailWidth_;
    std::atomic_bool mustStop_{false};
};
