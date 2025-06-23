// Written by HanQin Chen (cqnuchq@outlook.com) 2025-06-22
#include "services/AudioImporter.h"

#include <QFileInfo>
#include <QDir>
#include <QStandardPaths>
#include <QTemporaryFile>

#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/tpropertymap.h>
#include <taglib/mpegfile.h>
#include <taglib/id3v2tag.h>
#include <taglib/attachedpictureframe.h>

namespace {
    constexpr const char* DEFAULT_ARTIST = "Unknown Artist";
    constexpr const char* DEFAULT_ICON_PATH = "/MySongPlayer/assets/icons/app_icon.png";
    constexpr const char* COVERS_DIR = "/covers";
    constexpr const char* COVER_SUFFIX = "_cover.jpg";

    constexpr const char* MIME_JPEG = "image/jpeg";
    constexpr const char* MIME_PNG = "image/png";
    constexpr const char* MIME_UNKNOWN = "image/unknown";
    constexpr const char* EXT_JPG = "jpg";
    constexpr const char* EXT_JPEG = "jpeg";
    constexpr const char* EXT_PNG = "png";

    constexpr const char* APIC_FRAME = "APIC";

    constexpr const char* ENGLISH_COVER_DESC = "English Cover";
}

AudioImporter::AudioImporter(QObject *parent)
    : QObject{parent}
{}

void AudioImporter::importLocalAudio(const QList<QUrl>& fileUrls)
{
    for (const QUrl &fileUrl : fileUrls) {
        if (!fileUrl.isLocalFile()) {
            continue;
        }

        QString filePath = fileUrl.toLocalFile();
        QFileInfo fileInfo(filePath);

        if (!fileInfo.exists() || !fileInfo.isFile()) {
            continue;
        }

        processAudioFile(fileInfo);
    }
}

void AudioImporter::processAudioFile(const QFileInfo &fileInfo)
{
    QString filePath = fileInfo.absoluteFilePath();
    QString title = fileInfo.baseName();
    QString authorName = DEFAULT_ARTIST;
    QUrl imageSource = QUrl(DEFAULT_ICON_PATH);

    TagLib::FileRef fileRef(filePath.toUtf8().constData());

    if (!fileRef.isNull() && fileRef.tag()) {
        auto* tag = fileRef.tag();

        if (!tag->title().isEmpty()) {
            title = QString::fromStdString(tag->title().to8Bit(true));
        }

        if (!tag->artist().isEmpty()) {
            authorName = QString::fromStdString(tag->artist().to8Bit(true));
        }

        QUrl coverUrl = extractCoverArt(filePath);
        if (!coverUrl.isEmpty()) {
            imageSource = coverUrl;
        }
    } else {

    }

    QUrl audioSource = QUrl::fromLocalFile(filePath);
    emit audioImported(title, authorName, audioSource, imageSource);
}

QUrl AudioImporter::extractCoverArt(const QString &filePath)
{
    QDir coverDir = prepareCoverDirectory();

    QFileInfo fileInfo(filePath);
    QString coverPath = coverDir.filePath(fileInfo.baseName() + COVER_SUFFIX);
    QFileInfo coverInfo(coverPath);

    if (coverInfo.exists()) {
        return QUrl::fromLocalFile(coverPath);
    }

    return extractCoverFromFile(filePath, coverPath);
}

QDir AudioImporter::prepareCoverDirectory()
{
    QString coverDirPath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + COVERS_DIR;
    QDir coverDir(coverDirPath);

    if (!coverDir.exists()) {
        coverDir.mkpath(".");
    }

    return coverDir;
}

QUrl AudioImporter::extractCoverFromFile(const QString &audioFilePath, const QString &targetCoverPath)
{
    TagLib::MPEG::File mpegFile(audioFilePath.toUtf8().constData());
    TagLib::ID3v2::Tag *id3v2Tag = mpegFile.ID3v2Tag(true);

    if (!id3v2Tag) {
        return QUrl();
    }

    TagLib::ID3v2::FrameList frameList = id3v2Tag->frameList(APIC_FRAME);
    if (frameList.isEmpty()) {
        return QUrl();
    }


    for (auto it = frameList.begin(); it != frameList.end(); ++it) {
        TagLib::ID3v2::AttachedPictureFrame *pictureFrame =
            static_cast<TagLib::ID3v2::AttachedPictureFrame *>(*it);

        if (pictureFrame && pictureFrame->type() == TagLib::ID3v2::AttachedPictureFrame::FrontCover) {
            if (savePictureToFile(pictureFrame->picture(), targetCoverPath)) {
                return QUrl::fromLocalFile(targetCoverPath);
            }
        }
    }


    TagLib::ID3v2::AttachedPictureFrame *pictureFrame =
        static_cast<TagLib::ID3v2::AttachedPictureFrame *>(frameList.front());

    if (pictureFrame && savePictureToFile(pictureFrame->picture(), targetCoverPath)) {
        return QUrl::fromLocalFile(targetCoverPath);
    }

    return QUrl();
}

bool AudioImporter::savePictureToFile(const TagLib::ByteVector &pictureData, const QString &filePath)
{
    if (pictureData.isEmpty()) {
        return false;
    }

    QFile coverFile(filePath);
    if (!coverFile.open(QIODevice::WriteOnly)) {
        return false;
    }

    coverFile.write(pictureData.data(), pictureData.size());
    coverFile.close();
    return true;
}

bool AudioImporter::addEnglishCoverArt(const QString &filePath, const QString &coverImagePath)
{
    if (!validateInputFiles(filePath, coverImagePath)) {
        return false;
    }


    QByteArray coverData = readCoverImageData(coverImagePath);
    if (coverData.isEmpty()) {
        return false;
    }


    return writeCoverToAudioFile(filePath, coverImagePath, coverData);
}

bool AudioImporter::validateInputFiles(const QString &audioFilePath, const QString &coverFilePath)
{
    QFileInfo fileInfo(audioFilePath);
    QFileInfo coverInfo(coverFilePath);

    if (!fileInfo.exists() || !fileInfo.isFile()) {

        return false;
    }

    if (!coverInfo.exists() || !coverInfo.isFile()) {

        return false;
    }

    return true;
}

QByteArray AudioImporter::readCoverImageData(const QString &coverFilePath)
{
    QFile coverFile(coverFilePath);
    if (!coverFile.open(QIODevice::ReadOnly)) {

        return QByteArray();
    }

    QByteArray coverData = coverFile.readAll();
    coverFile.close();
    return coverData;
}

bool AudioImporter::writeCoverToAudioFile(const QString &filePath, const QString &coverImagePath, const QByteArray &coverData)
{
    TagLib::MPEG::File mpegFile(filePath.toUtf8().constData());
    TagLib::ID3v2::Tag *id3v2Tag = mpegFile.ID3v2Tag(true);

    if (!id3v2Tag) {
        return false;
    }

    auto frame = new TagLib::ID3v2::AttachedPictureFrame;


    QString extension = QFileInfo(coverImagePath).suffix().toLower();
    setMimeTypeForFrame(frame, extension);

    frame->setType(TagLib::ID3v2::AttachedPictureFrame::FrontCover);
    frame->setDescription(ENGLISH_COVER_DESC);
    frame->setTextEncoding(TagLib::String::UTF8);

    TagLib::ByteVector pictureData(coverData.constData(), coverData.size());
    frame->setPicture(pictureData);

    id3v2Tag->addFrame(frame);

    return mpegFile.save();
}

void AudioImporter::setMimeTypeForFrame(TagLib::ID3v2::AttachedPictureFrame* frame, const QString &extension)
{
    if (extension == EXT_JPG || extension == EXT_JPEG) {
        frame->setMimeType(MIME_JPEG);
    } else if (extension == EXT_PNG) {
        frame->setMimeType(MIME_PNG);
    } else {
        frame->setMimeType(MIME_UNKNOWN);
    }
}

