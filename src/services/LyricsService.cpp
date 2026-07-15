#include "services/LyricsService.h"
#include <QByteArray>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>

#include <string>
#include <string_view>

namespace {

std::string toUtf8String(const QString& value)
{
    const QByteArray bytes = value.toUtf8();
    return {bytes.constData(), static_cast<std::size_t>(bytes.size())};
}

} // namespace

LyricsService::LyricsService(QObject *parent)
    : QObject(parent)
{
}

std::vector<LyricsService::LyricLine> LyricsService::parseLrcFile(const QString& audioFilePath)
{
    std::vector<LyricLine> lyrics;

    /* Attempt to locate the corresponding LRC file for the given audio track.
     *  This involves checking for an exact match first, then performing a fuzzy search
     *  if an exact match is not found, to increase the likelihood of finding lyrics.
     */
    QString lrcFilePath = findLrcFile(audioFilePath);
    if (lrcFilePath.isEmpty()) {
        return lyrics;
    }

    QFile lrcFile(lrcFilePath);
    if (!lrcFile.open(QIODevice::ReadOnly | QIODevice::Text)) {

        return lyrics;
    }

    QTextStream stream(&lrcFile);
    stream.setEncoding(QStringConverter::Utf8);
    QString content = stream.readAll();
    lrcFile.close();

    const QByteArray contentBytes = content.toUtf8();
    lyrics = SongPlayer::Core::parseLrcContent(
        std::string_view(contentBytes.constData(), static_cast<std::size_t>(contentBytes.size())));


    return lyrics;
}

bool LyricsService::hasLyricsFile(const QString& audioFilePath)
{
    QString lrcFilePath = findLrcFile(audioFilePath);
    return !lrcFilePath.isEmpty();
}

QString LyricsService::findLrcFile(const QString& audioFilePath)
{
    QFileInfo audioInfo(audioFilePath);

    QString exactLrcPath = audioInfo.absolutePath() + "/" +
                           audioInfo.completeBaseName() + ".lrc";

    QFileInfo exactLrcInfo(exactLrcPath);
    if (exactLrcInfo.exists() && exactLrcInfo.isReadable()) {

        return exactLrcPath;
    }

    QString fuzzyMatch = findBestLrcMatch(audioFilePath);
    if (!fuzzyMatch.isEmpty()) {
        return fuzzyMatch;
    }


    return QString();
}

QStringList LyricsService::scanDirectoryForLrcFiles(const QString& dirPath)
{
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    // Implement a caching mechanism to avoid redundant directory scans.
    // This significantly improves performance when repeatedly searching the same directories
    // by returning cached results within a defined expiration period.
    if (m_directoryCache.contains(dirPath) && m_cacheTimestamp.contains(dirPath)) {
        qint64 cacheTime = m_cacheTimestamp.value(dirPath);
        if (currentTime - cacheTime < CACHE_EXPIRE_TIME) {
            return m_directoryCache.value(dirPath);
        }
    }

    QStringList lrcFiles;

    QDir directory(dirPath);
    if (!directory.exists()) {
        // Return an empty list if the directory does not exist or is inaccessible.
        return lrcFiles;
    }

    // Set name filters to only include files with the ".lrc" extension,
    // and filter for readable files to avoid processing inaccessible entries.
    QStringList nameFilters;
    nameFilters << "*.lrc";
    directory.setNameFilters(nameFilters);
    directory.setFilter(QDir::Files | QDir::Readable);

    QFileInfoList fileList = directory.entryInfoList();
    for (const QFileInfo& fileInfo : std::as_const(fileList)) {
        lrcFiles.append(fileInfo.absoluteFilePath());
    }

    // Update the cache with the newly scanned files and the current timestamp.
    // This ensures that subsequent requests for the same directory benefit from the cache.
    m_directoryCache[dirPath] = lrcFiles;
    m_cacheTimestamp[dirPath] = currentTime;

    return lrcFiles;
}

QString LyricsService::findBestLrcMatch(const QString& audioFilePath)
{
    // This function aims to find the most suitable LRC (lyrics) file for a given audio file.
    // It first looks for an exact match and then performs a fuzzy search based on filename similarity,
    // returning the path to the best-scoring LRC file above a certain threshold.

    if (audioFilePath.isEmpty()) {
        // Early exit if the audio file path is empty, as no match can be found.
        return QString();
    }

    QFileInfo audioInfo(audioFilePath);
    if (!audioInfo.exists()) {
        // Early exit if the audio file does not exist, preventing unnecessary processing.
        return QString();
    }

    QString audioBaseName = audioInfo.completeBaseName();
    QString dirPath = audioInfo.absolutePath();

    if (audioBaseName.isEmpty()) {
        // If the audio file has no base name, it cannot be matched against LRC files.
        return QString();
    }

    // Scan the directory containing the audio file for all available LRC files.
    // This list will be used for fuzzy matching.
    QStringList lrcFiles = scanDirectoryForLrcFiles(dirPath);
    if (lrcFiles.isEmpty()) {
        // If no LRC files are found in the directory, no match is possible.
        return QString();
    }

    QString bestMatch;
    double bestScore = 0.0;
    // Define a threshold for the match score. Only LRC files with a score above this
    // will be considered valid matches, filtering out low-confidence results.
    const double MATCH_THRESHOLD = 60.0;

    // Iterate through all found LRC files to calculate a similarity score for each.
    // The file with the highest score above the threshold is selected as the best match.
    for (const QString& lrcFilePath : std::as_const(lrcFiles)) {
        QFileInfo lrcInfo(lrcFilePath);
        QString lrcBaseName = lrcInfo.completeBaseName();

        if (lrcBaseName.isEmpty()) {
            // Skip LRC files that have no base name, as they cannot be meaningfully compared.
            continue;
        }

        double score = SongPlayer::Core::lyricFileMatchScore(
            toUtf8String(audioBaseName),
            toUtf8String(lrcBaseName));

        if (score > bestScore && score >= MATCH_THRESHOLD) {
            bestScore = score;
            bestMatch = lrcFilePath;
        }
    }

    // Log the outcome of the search for debugging and monitoring purposes.
    if (!bestMatch.isEmpty()) {
        qDebug() << "LyricsService: Found best LRC match for" << audioBaseName << ":" << bestMatch << "(Score:" << bestScore << ")";
    } else {
        qDebug() << "LyricsService: No suitable LRC match found for" << audioBaseName;
    }

    return bestMatch;
}
