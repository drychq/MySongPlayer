#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include <QUrl>
#include <QtQml/qqmlregistration.h>
#include <QHash>


class LyricsService : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    struct LyricLine {
        qint64 timestamp;
        QString text;

        bool operator<(const LyricLine& other) const {
            return timestamp < other.timestamp;
        }
    };

    explicit LyricsService(QObject *parent = nullptr);

    Q_INVOKABLE QList<LyricLine> parseLrcFile(const QString& audioFilePath);

    Q_INVOKABLE bool hasLyricsFile(const QString& audioFilePath);

private:
    QString findLrcFile(const QString& audioFilePath);

    QString findBestLrcMatch(const QString& audioFilePath);

    QString preprocessFileName(const QString& fileName);

    double calculateMatchScore(const QString& audioName, const QString& lrcName);

    QStringList scanDirectoryForLrcFiles(const QString& dirPath);

    QList<LyricLine> parseLrcContent(const QString& content);

    qint64 parseTimestamp(const QString& timeString);

private:
    QHash<QString, QStringList> m_directoryCache;
    QHash<QString, qint64> m_cacheTimestamp;
    static constexpr qint64 CACHE_EXPIRE_TIME = 30000;
};
