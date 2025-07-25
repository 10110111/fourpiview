#include "ImageFinder.hpp"
#include <vector>
#include <QDir>
#include <QDirIterator>
#include <QImageReader>
#include <QStandardPaths>
#ifdef Q_OS_ANDROID
#include <QtCore/private/qandroidextras_p.h>
#endif

ImageFinder::ImageFinder(const int thumbnailWidth, QObject* parent)
    : QThread(parent)
    , thumbnailWidth_(thumbnailWidth)
{
}

void ImageFinder::findAllImages()
{
#ifdef Q_OS_ANDROID
    // Qt returns Pictures folder as the PicturesLocation, but the actual
    // Gallery is usually stored in DCIM. So let's just ask for the generic
    // data location, hoping that it will include both picture locations as
    // subdirectories.
    const auto dataPaths = QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation);
#else
    const auto dataPaths = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation);
#endif

#ifdef Q_OS_ANDROID
    {
        using namespace QtAndroidPrivate;
        const QString storagePermission = "android.permission.READ_EXTERNAL_STORAGE";
        if(checkPermission(storagePermission).result() == Denied)
        {
            if(requestPermission(storagePermission).result() == Denied)
                qDebug().noquote() << "Failed to get" << storagePermission << "permission";
        }
    }
#endif

    imagePaths.clear();
    for(const auto& dataPath : dataPaths)
    {
        QDirIterator it(dataPath, {"*.JPG", "*.PNG"}, QDir::Files, QDirIterator::Subdirectories);
        while(it.hasNext())
        {
            if(mustStop_) return;

            const auto path = it.next();
            QImageReader reader(path);
            const auto size = reader.size();
            if(size.width() != size.height() * 2)
                continue;
            imagePaths << path;

            emit imageFound(path);
        }
    }
}

void ImageFinder::loadThumbnails()
{
    std::vector<QImage> images;
    for(const auto& path : imagePaths)
    {
        if(mustStop_) return;

        QImageReader reader(path);
        reader.setScaledSize(QSize(thumbnailWidth_, thumbnailWidth_ / 2));
        const auto& img = images.emplace_back(reader.read());
        if(img.isNull())
        {
            qDebug().noquote().nospace() << "Failed to read \"" << path << "\":" << reader.errorString();
            continue;
        }
        emit thumbnailReady(path, img);
    }
}

void ImageFinder::run()
{
    findAllImages();
    std::sort(imagePaths.rbegin(), imagePaths.rend());
    loadThumbnails();
}

void ImageFinder::stop()
{
    mustStop_ = true;
}
