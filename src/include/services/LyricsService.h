#pragma once

#include "core/Lyrics.h"

#include <QHash>
#include <QObject>
#include <QString>
#include <QStringList>
#include <vector>

class LyricsService : public QObject
{
    Q_OBJECT

public:
    using LyricLine = SongPlayer::Core::LyricLine;

    explicit LyricsService(QObject *parent = nullptr);

    std::vector<LyricLine> parseLrcFile(const QString& audioFilePath);

private:
    QString findLrcFile(const QString& audioFilePath);

    QString findBestLrcMatch(const QString& audioFilePath);

    QStringList scanDirectoryForLrcFiles(const QString& dirPath);

private:
    QHash<QString, QStringList> m_directoryCache;
    QHash<QString, qint64> m_cacheTimestamp;
    static constexpr qint64 CACHE_EXPIRE_TIME = 30000;
};
