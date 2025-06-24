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

    static const QRegularExpression timeTagRegex(R"(\[(\d{1,2}):(\d{2})(?:\.(\d{1,3}))?\])");
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
                continue;
            }

            QRegularExpressionMatchIterator timeIterator = timeTagRegex.globalMatch(trimmedLine);
            while (timeIterator.hasNext()) {
                QRegularExpressionMatch timeMatch = timeIterator.next();

                int minutes = timeMatch.captured(1).toInt();
                int seconds = timeMatch.captured(2).toInt();
                int fractionalPart = timeMatch.captured(3).isEmpty() ? 0 : timeMatch.captured(3).toInt();

                qint64 timestamp;
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

    std::sort(lyrics.begin(), lyrics.end());

    return lyrics;
}

qint64 LyricsService::parseTimestamp(const QString& timeString)
{
    // 使用静态局部变量避免重复创建QRegularExpression对象
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
    QString result = fileName.toLower();

    // 使用静态局部变量避免重复创建QRegularExpression对象
    static const QRegularExpression specialChars(R"([\(\)\[\]\{\}\-_\s\.\,\;\:\!\?\'\"\`\~\@\#\$\%\^\&\*\+\=\|\\\/<>])");
    result = result.replace(specialChars, "");

    QStringList commonTags = {"lyrics", "lrc", "karaoke", "vocal", "instrumental", "remix", "edit", "version"};
    for (const QString& tag : commonTags) {
        result = result.replace(tag, "");
    }

    return result.trimmed();
}

double LyricsService::calculateMatchScore(const QString& audioName, const QString& lrcName)
{

    QString cleanAudio = preprocessFileName(audioName);
    QString cleanLrc = preprocessFileName(lrcName);


    if (cleanAudio.isEmpty() || cleanLrc.isEmpty()) {
        return 0.0;
    }


    if (cleanAudio == cleanLrc) {
        return 100.0;
    }


    if (cleanLrc.contains(cleanAudio)) {
        double lengthRatio = static_cast<double>(cleanAudio.length()) / cleanLrc.length();
        return 80.0 + (lengthRatio * 15.0);
    }

    if (cleanAudio.contains(cleanLrc)) {
        double lengthRatio = static_cast<double>(cleanLrc.length()) / cleanAudio.length();
        return 75.0 + (lengthRatio * 10.0);
    }


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
    if (m_directoryCache.contains(dirPath) && m_cacheTimestamp.contains(dirPath)) {
        qint64 cacheTime = m_cacheTimestamp.value(dirPath);
        if (currentTime - cacheTime < CACHE_EXPIRE_TIME) {

            return m_directoryCache.value(dirPath);
        }
    }

    QStringList lrcFiles;

    QDir directory(dirPath);
    if (!directory.exists()) {

        return lrcFiles;
    }

    QStringList nameFilters;
    nameFilters << "*.lrc";
    directory.setNameFilters(nameFilters);
    directory.setFilter(QDir::Files | QDir::Readable);

    QFileInfoList fileList = directory.entryInfoList();
    for (const QFileInfo& fileInfo : std::as_const(fileList)) {
        lrcFiles.append(fileInfo.absoluteFilePath());
    }

    m_directoryCache[dirPath] = lrcFiles;
    m_cacheTimestamp[dirPath] = currentTime;


    return lrcFiles;
}

QString LyricsService::findBestLrcMatch(const QString& audioFilePath)
{
    if (audioFilePath.isEmpty()) {

        return QString();
    }

    QFileInfo audioInfo(audioFilePath);
    if (!audioInfo.exists()) {

        return QString();
    }

    QString audioBaseName = audioInfo.completeBaseName();
    QString dirPath = audioInfo.absolutePath();

    if (audioBaseName.isEmpty()) {

        return QString();
    }




    QStringList lrcFiles = scanDirectoryForLrcFiles(dirPath);
    if (lrcFiles.isEmpty()) {

        return QString();
    }

    QString bestMatch;
    double bestScore = 0.0;
    const double MATCH_THRESHOLD = 60.0;



    for (const QString& lrcFilePath : std::as_const(lrcFiles)) {
        QFileInfo lrcInfo(lrcFilePath);
        QString lrcBaseName = lrcInfo.completeBaseName();

        if (lrcBaseName.isEmpty()) {

            continue;
        }

        double score = calculateMatchScore(audioBaseName, lrcBaseName);



        if (score > bestScore && score >= MATCH_THRESHOLD) {
            bestScore = score;
            bestMatch = lrcFilePath;

        }
    }

    if (!bestMatch.isEmpty()) {

    } else {

    }

    return bestMatch;
}
