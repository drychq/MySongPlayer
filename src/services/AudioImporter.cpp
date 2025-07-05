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
    // Default artist name to use when metadata is missing, ensuring a consistent display.
    constexpr const char* DEFAULT_ARTIST = "Unknown Artist";

    // Path to a fallback icon for album art when no embedded cover is found, providing a visual placeholder.
    constexpr const char* DEFAULT_ICON_PATH = "qrc:/qt/qml/MySongPlayer/assets/icons/app_icon.png";

    // Subdirectory within the cache location for storing extracted album covers, centralizing temporary image files.
    constexpr const char* COVERS_DIR = "/covers";

    // Suffix appended to extracted cover art filenames to distinguish them and prevent naming conflicts.
    constexpr const char* COVER_SUFFIX = "_cover.jpg";

    // MIME types for common image formats, used when embedding cover art into audio files to ensure proper rendering.
    constexpr const char* MIME_JPEG = "image/jpeg";
    constexpr const char* MIME_PNG = "image/png";
    constexpr const char* MIME_UNKNOWN = "image/unknown";

    // File extensions for common image formats, used for MIME type detection.
    constexpr const char* EXT_JPG = "jpg";
    constexpr const char* EXT_JPEG = "jpeg";
    constexpr const char* EXT_PNG = "png";

    // ID3v2 frame identifier for attached pictures, standardizing how cover art is located within audio file metadata.
    constexpr const char* APIC_FRAME = "APIC";

    // Description for embedded cover art, providing context for the image within the audio file's metadata.
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
    /*  This method orchestrates the extraction of audio metadata (title, artist) and album art
     *  from a given audio file. It prioritizes embedded metadata but falls back to default values
     *  if information is missing, ensuring a complete data set for each imported track.
     */
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
        /* If TagLib fails to read the file or its tags, proceed with default title/artist
         * and the default icon path. This ensures that even unreadable files can be added
         * to the playlist, albeit with limited metadata.
         */
    }

    QUrl audioSource = QUrl::fromLocalFile(filePath);
    emit audioImported(title, authorName, audioSource, imageSource);
}

QUrl AudioImporter::extractCoverArt(const QString &filePath)
{
    // Ensures a dedicated directory exists for storing extracted album covers.
    // This centralizes cover art management and prevents cluttering the original audio file directories.
    QDir coverDir = prepareCoverDirectory();

    QFileInfo fileInfo(filePath);
    // Constructs a unique filename for the cover based on the audio file's base name,
    // ensuring that each extracted cover has a distinct identifier.
    QString coverPath = coverDir.filePath(fileInfo.baseName() + COVER_SUFFIX);
    QFileInfo coverInfo(coverPath);

    // Optimizes performance by checking if the cover art has already been extracted and saved.
    // If an existing cover is found, it's reused to avoid redundant processing and disk I/O.
    if (coverInfo.exists()) {
        return QUrl::fromLocalFile(coverPath);
    }

    // If no existing cover is found, proceed to extract it from the audio file.
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
    // Attempts to open the audio file as an MPEG file and retrieve its ID3v2 tag.
    // The ID3v2 tag is where embedded album art is typically stored.
    TagLib::MPEG::File mpegFile(audioFilePath.toUtf8().constData());
    TagLib::ID3v2::Tag *id3v2Tag = mpegFile.ID3v2Tag(true);

    if (!id3v2Tag) {
        // If no ID3v2 tag is found, it means there's no embedded metadata to extract cover art from.
        return QUrl();
    }

    // Retrieves all 'APIC' (Attached Picture) frames from the ID3v2 tag.
    // These frames contain the actual image data for album covers.
    TagLib::ID3v2::FrameList frameList = id3v2Tag->frameList(APIC_FRAME);
    if (frameList.isEmpty()) {
        // If no APIC frames are present, there's no embedded cover art.
        return QUrl();
    }

    // Iterates through the list of attached picture frames to find the 'Front Cover'.
    // Prioritizing the front cover ensures that the most relevant album art is extracted.
    for (auto it = frameList.begin(); it != frameList.end(); ++it) {
        TagLib::ID3v2::AttachedPictureFrame *pictureFrame =
            static_cast<TagLib::ID3v2::AttachedPictureFrame *>(*it);

        if (pictureFrame && pictureFrame->type() == TagLib::ID3v2::AttachedPictureFrame::FrontCover) {
            if (savePictureToFile(pictureFrame->picture(), targetCoverPath)) {
                return QUrl::fromLocalFile(targetCoverPath);
            }
        }
    }

    // Fallback: If no 'Front Cover' specifically is found, use the first available attached picture frame.
    // This ensures that some form of album art is extracted if a dedicated front cover isn't explicitly tagged.
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

/**
 * @brief Adds a specified image file as the front cover art to an audio file.
 *
 * This function is designed to embed an external image (e.g., a downloaded album cover)
 * into the audio file's metadata. It performs necessary validations to ensure both
 * the audio file and the cover image exist and are accessible before proceeding.
 * The cover art is added as an ID3v2 Attached Picture Frame (APIC) with a specific
 * description and type (Front Cover).
 *
 * @param filePath The absolute path to the audio file (e.g., MP3) to which the cover art will be added.
 * @param coverImagePath The absolute path to the image file (e.g., JPG, PNG) that will be embedded.
 * @return True if the cover art was successfully added and saved to the audio file, false otherwise.
 */
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

    // Determine and set the correct MIME type for the embedded image.
    // This is crucial for players to correctly interpret and display the cover art.
    QString extension = QFileInfo(coverImagePath).suffix().toLower();
    setMimeTypeForFrame(frame, extension);

    // Set the picture type to 'Front Cover', which is a standard ID3v2 tag type
    // indicating the primary album art. This helps players prioritize and display it correctly.
    frame->setType(TagLib::ID3v2::AttachedPictureFrame::FrontCover);
    // Add a descriptive text for the cover art, which can be useful for players
    // that display this information or for debugging purposes.
    frame->setDescription(ENGLISH_COVER_DESC);
    // Specify UTF8 encoding for the text fields within the frame to ensure proper display
    // of various characters across different systems.
    frame->setTextEncoding(TagLib::String::UTF8);

    TagLib::ByteVector pictureData(coverData.constData(), coverData.size());
    frame->setPicture(pictureData);

    id3v2Tag->addFrame(frame);

    // Save the modified MPEG file, persisting the newly added cover art.
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

