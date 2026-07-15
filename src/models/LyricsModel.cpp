#include "models/LyricsModel.h"

#include <optional>
#include <utility>

namespace {

QString toQString(const std::string& value)
{
    return QString::fromUtf8(value.data(), static_cast<qsizetype>(value.size()));
}

} // namespace

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

void LyricsModel::setLyrics(std::vector<SongPlayer::Core::LyricLine> lyrics)
{
    m_lyrics = std::move(lyrics);

    bool hasLyrics = !m_lyrics.empty();
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
    if (m_lyrics.empty()) {
        return;
    }

    int newIndex = findLyricIndexByPosition(position);

    // Update current line index
    if (m_currentLineIndex != newIndex) {
        m_currentLineIndex = newIndex;
        emit currentLineIndexChanged();

        // Update current lyric text
        QString newCurrentLyric;
        if (newIndex >= 0 && static_cast<std::size_t>(newIndex) < m_lyrics.size()) {
            newCurrentLyric = toQString(m_lyrics[static_cast<std::size_t>(newIndex)].text);
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

    for (const auto& lyricLine : m_lyrics) {
        m_allLyrics.append(toQString(lyricLine.text));
    }

    emit allLyricsChanged();
}

int LyricsModel::findLyricIndexByPosition(qint64 position)
{
    if (m_lyrics.empty()) {
        return -1;
    }

    const std::optional<std::size_t> index = SongPlayer::Core::lyricIndexAtPosition(m_lyrics, position);
    if (!index) {
        return -1;
    }

    return static_cast<int>(*index);
}
