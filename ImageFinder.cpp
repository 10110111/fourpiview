#include "ImageFinder.hpp"
#include <vector>
#include <QDir>
#include <QFileInfo>
#include <QDirIterator>
#include <QImageReader>
#include <QStandardPaths>
#ifdef Q_OS_ANDROID
#include <QtCore/private/qandroidextras_p.h>
#endif
#include <exiv2/exiv2.hpp>

namespace
{
QDateTime getFallbackDateTime(const QString& path)
{
    qWarning() << "Resorting to file date-time for" << path;
    const QFileInfo info(path);
    auto dateTime = info.birthTime();
    if(!dateTime.isValid())
        dateTime = info.lastModified();
    return dateTime;
}

QDateTime getDateTime(const QString& path)
{
    const auto image = Exiv2::ImageFactory::open(path.toStdString());
    if(!image.get())
    {
        qWarning() << "exiv2 read failed for" << path;
        return getFallbackDateTime(path);
    }
    image->readMetadata();
    const auto& exif = image->exifData();

    QString date;
    for(const auto& key : {"Exif.Photo.DateTimeOriginal",
                           "Exif.Image.DateTimeOriginal",
                           "Exif.Image.DateTime"})
    {
        const auto it=exif.findKey(Exiv2::ExifKey(key));
        if(it!=exif.end())
        {
            date = QString::fromStdString(it->toString());
            break;
        }
    }
    QString gpsTime;
    for(const auto& key : {"Exif.GPSInfo.GPSTimeStamp"})
    {
        const auto it=exif.findKey(Exiv2::ExifKey(key));
        if(it!=exif.end())
        {
            if(it->count() != 3)
                continue;
            const auto hour = it->toFloat(0);
            const auto min = it->toFloat(1);
            const auto sec = it->toFloat(2);
            if(hour < 0 || hour > 59 || hour - std::floor(hour) != 0 ||
               min  < 0 || min  > 59 || min  - std::floor(min ) != 0)
                continue;
            gpsTime = QString("%1:%2:%3").arg(int(hour), 2, 10, QChar('0'))
                .arg(int(min), 2, 10, QChar('0'))
                .arg(double(sec), 2, 'f', 0, QChar('0'));
            break;
        }
    }

    QString gpsDate;
    for(const auto& key : {"Exif.GPSInfo.GPSDateStamp"})
    {
        const auto it=exif.findKey(Exiv2::ExifKey(key));
        if(it!=exif.end())
        {
            gpsDate = QString::fromStdString(it->toString());
            break;
        }
    }

    // If GPS date and time are present, take them as more reliable.
    // At the very least, time zone is present there unconditionally.
    if(!gpsDate.isEmpty() && !gpsTime.isEmpty())
    {
        date = gpsDate + " " + gpsTime;
    }

    const auto dateTime = QDateTime::fromString(date, "yyyy:MM:dd HH:mm:ss");
    if(dateTime.isValid()) return dateTime;

    return getFallbackDateTime(path);
}
}

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

    imageInfos_.clear();
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

            const auto dateTime = getDateTime(path);

            imageInfos_.push_back({path, dateTime});
            emit imageFound(imageInfos_.back());
        }
    }
}

void ImageFinder::loadThumbnails()
{
    std::vector<QImage> images;
    for(const auto& info : imageInfos_)
    {
        if(mustStop_) return;

        QImageReader reader(info.path);
        reader.setScaledSize(QSize(thumbnailWidth_, thumbnailWidth_ / 2));
        const auto& img = images.emplace_back(reader.read());
        if(img.isNull())
        {
            qDebug().noquote().nospace() << "Failed to read \"" << info.path << "\":" << reader.errorString();
            continue;
        }
        emit thumbnailReady(info.path, img);
    }
}

void ImageFinder::run()
{
    findAllImages();
    std::sort(imageInfos_.begin(), imageInfos_.end(),
              [](const auto& a, const auto& b) { return a.dateTime > b.dateTime; });
    loadThumbnails();
}

void ImageFinder::stop()
{
    mustStop_ = true;
}
