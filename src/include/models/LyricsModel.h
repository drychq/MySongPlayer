#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QtQml/qqmlregistration.h>
#include "services/LyricsService.h"

class LyricsModel : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool hasLyrics READ hasLyrics NOTIFY hasLyricsChanged)
    Q_PROPERTY(QString currentLyric READ currentLyric NOTIFY currentLyricChanged)
    Q_PROPERTY(bool showLyrics READ showLyrics WRITE setShowLyrics NOTIFY showLyricsChanged)
    Q_PROPERTY(QStringList allLyrics READ allLyrics NOTIFY allLyricsChanged)
    Q_PROPERTY(int currentLineIndex READ currentLineIndex NOTIFY currentLineIndexChanged)

public:
    explicit LyricsModel(QObject *parent = nullptr);

    bool hasLyrics() const { return m_hasLyrics; }
    QString currentLyric() const { return m_currentLyric; }
    bool showLyrics() const { return m_showLyrics; }
    QStringList allLyrics() const { return m_allLyrics; }
    int currentLineIndex() const { return m_currentLineIndex; }

    void setShowLyrics(bool show);
    void setLyrics(const QList<LyricsService::LyricLine>& lyrics);

    void updatePosition(qint64 position);
    void clearLyrics();
    Q_INVOKABLE void toggleDisplayMode();



signals:
    void hasLyricsChanged();
    void currentLyricChanged();
    void showLyricsChanged();
    void allLyricsChanged();
    void currentLineIndexChanged();

private:
    QList<LyricsService::LyricLine> m_lyrics;
    QStringList m_allLyrics;

    int m_currentLineIndex;
    QString m_currentLyric;
    bool m_hasLyrics;
    bool m_showLyrics;

    void updateAllLyrics();

    int findLyricIndexByPosition(qint64 position);

};
