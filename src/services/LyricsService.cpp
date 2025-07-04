#include "LyricsService.h"
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QRegularExpression>
#include <algorithm>
#include <QDir>
#include <QDateTime>

LyricsService::LyricsService(QObject *parent)
    : QObject(parent)
{
}

QList<LyricsService::LyricLine> LyricsService::parseLrcFile(const QString& audioFilePath)
{
    QList<LyricLine> lyrics;

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

    lyrics = parseLrcContent(content);


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

QList<LyricsService::LyricLine> LyricsService::parseLrcContent(const QString& content)
{
    QList<LyricLine> lyrics;

    // Regular expression to capture time tags in the format [mm:ss.zzz] or [mm:ss.zz].
    // This allows for flexible parsing of different millisecond precisions in LRC files.
    static const QRegularExpression timeTagRegex(R"(\[(\d{1,2}):(\d{2})(?:\.(\d{1,3}))?\])");
    // Regular expression to match a full LRC line, ensuring it starts with one or more time tags
    // followed by the actual lyric text. This helps in robustly separating timestamps from content.
    static const QRegularExpression fullLineRegex(R"(^(\[(?:\d{1,2}:\d{2}(?:\.\d{1,3})?)\])+(.*)$)");

    QStringList lines = content.split('\n');

    for (const QString& line : std::as_const(lines)) {
        QString trimmedLine = line.trimmed();
        if (trimmedLine.isEmpty()) {
            continue;
        }

        QRegularExpressionMatch fullMatch = fullLineRegex.match(trimmedLine);
        if (fullMatch.hasMatch()) {
            QString text = fullMatch.captured(2).trimmed();

            if (text.isEmpty()) {
                // Skip lines that only contain time tags but no actual lyric text.
                continue;
            }

            // Iterate through all time tags on a single line. Some LRC formats allow multiple timestamps
            // for the same lyric line, indicating it should appear at different points in time.
            QRegularExpressionMatchIterator timeIterator = timeTagRegex.globalMatch(trimmedLine);
            while (timeIterator.hasNext()) {
                QRegularExpressionMatch timeMatch = timeIterator.next();

                int minutes = timeMatch.captured(1).toInt();
                int seconds = timeMatch.captured(2).toInt();
                int fractionalPart = timeMatch.captured(3).isEmpty() ? 0 : timeMatch.captured(3).toInt();

                qint64 timestamp;
                // Adjust fractional part based on its length (milliseconds vs. centiseconds).
                // This handles variations in LRC file formatting where milliseconds might be 2 or 3 digits.
                if (timeMatch.captured(3).length() == 3) {
                    timestamp = (minutes * 60 + seconds) * 1000 + fractionalPart;
                } else {
                    timestamp = (minutes * 60 + seconds) * 1000 + fractionalPart * 10;
                }

                LyricLine lyricLine;
                lyricLine.timestamp = timestamp;
                lyricLine.text = text;
                lyrics.append(lyricLine);
            }
        }
    }

    // Sort the lyric lines by timestamp to ensure they are in chronological order.
    // This is crucial for correct lyric display during playback, as lines might be out of order in the raw file.
    std::sort(lyrics.begin(), lyrics.end());

    return lyrics;
}


qint64 LyricsService::parseTimestamp(const QString& timeString)
{
    static const QRegularExpression timeRegex(R"((\d{1,2}):(\d{2})(?:\.(\d{2}))?)");
    QRegularExpressionMatch match = timeRegex.match(timeString);

    if (!match.hasMatch()) {
        return -1;
    }

    int minutes = match.captured(1).toInt();
    int seconds = match.captured(2).toInt();
    int centiseconds = match.captured(3).isEmpty() ? 0 : match.captured(3).toInt();


    return (minutes * 60 + seconds) * 1000 + centiseconds * 10;
}

QString LyricsService::preprocessFileName(const QString& fileName)
{
    // Converts the file name to lowercase to ensure case-insensitive comparisons,
    // which is crucial for robust fuzzy matching of audio and LRC file names.
    QString result = fileName.toLower();

    // Removes common special characters and punctuation from the file name.
    // This normalization helps in matching files where names might differ only by these characters,
    // improving the accuracy of fuzzy matching algorithms.
    static const QRegularExpression specialChars(R"([\(\)\[\]\{\}\-_\s\.\,\;\:\!\?\'\"\`\~\@\#\$\%\^\&\*\+\=\|\\\/<>])");
    result = result.replace(specialChars, "");

    // Removes common tags or keywords that are often present in file names but are irrelevant
    // for identifying the core song title (e.g., "lyrics", "karaoke"). This further refines
    // the file name for more accurate matching.
    QStringList commonTags = {"lyrics", "lrc", "karaoke", "vocal", "instrumental", "remix", "edit", "version"};
    for (const QString& tag : commonTags) {
        result = result.replace(tag, "");
    }

    return result.trimmed();
}

double LyricsService::calculateMatchScore(const QString& audioName, const QString& lrcName)
{
    // Preprocess both audio and LRC file names to normalize them for comparison.
    // This removes irrelevant characters and converts to lowercase, ensuring more accurate matching.
    QString cleanAudio = preprocessFileName(audioName);
    QString cleanLrc = preprocessFileName(lrcName);

    if (cleanAudio.isEmpty() || cleanLrc.isEmpty()) {
        // If either name is empty after preprocessing, no meaningful comparison can be made.
        return 0.0;
    }

    // Perfect match: If the cleaned names are identical, assign the highest possible score.
    // This is the most reliable indicator of a correct match.
    if (cleanAudio == cleanLrc) {
        return 100.0;
    }

    // Partial match (LRC contains audio name): If the LRC name contains the audio name,
    // it's a strong indicator of a match. The score is adjusted based on the length ratio
    // to penalize longer LRC names that might contain the audio name by chance.
    if (cleanLrc.contains(cleanAudio)) {
        double lengthRatio = static_cast<double>(cleanAudio.length()) / lrcName.length();
        return 80.0 + (lengthRatio * 15.0);
    }

    // Partial match (Audio contains LRC name): If the audio name contains the LRC name,
    // it's also a good indicator. The score is slightly lower than the previous case
    // because audio names are often shorter and less descriptive than LRC names.
    if (cleanAudio.contains(cleanLrc)) {
        double lengthRatio = static_cast<double>(cleanLrc.length()) / cleanAudio.length();
        return 75.0 + (lengthRatio * 10.0);
    }

    // Fallback: Calculate a similarity score based on common characters.
    // This handles cases where there's no direct containment but still significant overlap.
    // It provides a baseline score for less obvious matches.
    int commonChars = 0;
    int maxLen = qMax(cleanAudio.length(), cleanLrc.length());

    for (int i = 0; i < cleanAudio.length(); ++i) {
        if (cleanLrc.contains(cleanAudio.at(i))) {
            commonChars++;
        }
    }

    double similarity = static_cast<double>(commonChars) / maxLen;
    return similarity * 60.0;
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

        double score = calculateMatchScore(audioBaseName, lrcBaseName);

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
