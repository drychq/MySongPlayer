// Written by HanQin Chen (cqnuchq@outlook.com) 2025-06-22
#pragma once

#include <QObject>
#include <QUrl>
#include <QDir>
#include <QFileInfo>

namespace TagLib {
    class ByteVector;

    namespace ID3v2 {
        class AttachedPictureFrame;
    }
}

/**
 * @brief Audio import service class responsible for importing and parsing audio files
 *
 * This class focuses on the following responsibilities:
 * - Import audio files from local file system
 * - Parse audio metadata (using TagLib)
 * - Pass parsing results to playlist model
 */
class AudioImporter : public QObject
{
    Q_OBJECT

public:
    explicit AudioImporter(QObject *parent = nullptr);

    Q_INVOKABLE void importLocalAudio(const QList<QUrl>& fileUrls);

    Q_INVOKABLE bool addEnglishCoverArt(const QString &filePath, const QString &coverImagePath);

signals:
    void audioImported(const QString& title,
                       const QString& authorName,
                       const QUrl& audioSource,
                       const QUrl& imageSource);

private:
    void processAudioFile(const QFileInfo &fileInfo);

    QUrl extractCoverArt(const QString &filePath);

    QDir prepareCoverDirectory();

    QUrl extractCoverFromFile(const QString &audioFilePath, const QString &targetCoverPath);

    bool savePictureToFile(const TagLib::ByteVector &pictureData, const QString &filePath);

    bool validateInputFiles(const QString &audioFilePath, const QString &coverFilePath);


    QByteArray readCoverImageData(const QString &coverFilePath);

    bool writeCoverToAudioFile(const QString &filePath, const QString &coverImagePath, const QByteArray &coverData);

    void setMimeTypeForFrame(TagLib::ID3v2::AttachedPictureFrame* frame, const QString &extension);
};
