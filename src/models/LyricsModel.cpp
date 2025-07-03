#include "LyricsModel.h"
#include "models/AudioInfo.h"
#include "controllers/PlayerController.h"

LyricsModel::LyricsModel(QObject *parent)
    : QObject(parent)
    , m_currentLineIndex(-1)
    , m_hasLyrics(false)
    , m_showLyrics(false)
{
}

void LyricsModel::setShowLyrics(bool show)
{
    if (m_showLyrics != show) {
        m_showLyrics = show;
        emit showLyricsChanged();
    }
}

void LyricsModel::setLyrics(const QList<LyricsService::LyricLine>& lyrics)
{
    m_lyrics = lyrics;

    bool hasLyrics = !lyrics.isEmpty();
    if (m_hasLyrics != hasLyrics) {
        m_hasLyrics = hasLyrics;
        emit hasLyricsChanged();
    }

    updateAllLyrics();

    if (m_currentLineIndex != -1) {
        m_currentLineIndex = -1;
        emit currentLineIndexChanged();
    }

    if (!m_currentLyric.isEmpty()) {
        m_currentLyric.clear();
        emit currentLyricChanged();
    }

    // Lyrics data updated - no debug output in production
}

void LyricsModel::updatePosition(qint64 position)
{
    if (m_lyrics.isEmpty()) {
        return;
    }

    int newIndex = findLyricIndexByPosition(position);

    // Update current line index
    if (m_currentLineIndex != newIndex) {
        m_currentLineIndex = newIndex;
        emit currentLineIndexChanged();

        // Update current lyric text
        QString newCurrentLyric;
        if (newIndex >= 0 && newIndex < m_lyrics.size()) {
            newCurrentLyric = m_lyrics[newIndex].text;
        }

        if (m_currentLyric != newCurrentLyric) {
            m_currentLyric = newCurrentLyric;
            emit currentLyricChanged();
        }
    }
}

void LyricsModel::clearLyrics()
{
    m_lyrics.clear();
    m_allLyrics.clear();

    if (m_hasLyrics) {
        m_hasLyrics = false;
        emit hasLyricsChanged();
    }

    if (m_currentLineIndex != -1) {
        m_currentLineIndex = -1;
        emit currentLineIndexChanged();
    }

    if (!m_currentLyric.isEmpty()) {
        m_currentLyric.clear();
        emit currentLyricChanged();
    }

    emit allLyricsChanged();

    // Lyrics data cleared - no debug output in production
}

void LyricsModel::toggleDisplayMode()
{
    setShowLyrics(!m_showLyrics);
    // Toggle display mode called - no debug output in production
}

void LyricsModel::updateAllLyrics()
{
    m_allLyrics.clear();

    for (const auto& lyricLine : std::as_const(m_lyrics)) {
        m_allLyrics.append(lyricLine.text);
    }

    emit allLyricsChanged();
}

int LyricsModel::findLyricIndexByPosition(qint64 position)
{
    if (m_lyrics.isEmpty()) {
        return -1;
    }

    int left = 0;
    int right = m_lyrics.size() - 1;
    int result = -1;

    while (left <= right) {
        int mid = left + (right - left) / 2;

        if (m_lyrics[mid].timestamp <= position) {
            result = mid;
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }

    return result;
}


