#include "core/Playlist.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <string>
#include <unordered_set>
#include <utility>

namespace SongPlayer::Core {
namespace {

std::string normalized(std::string_view text)
{
    std::string result;
    result.reserve(text.size());

    for (unsigned char character : text) {
        result.push_back(static_cast<char>(std::tolower(character)));
    }

    return result;
}

bool containsCaseInsensitive(std::string_view value, std::string_view needle)
{
    if (needle.empty()) {
        return true;
    }

    const std::string normalizedValue = normalized(value);
    const std::string normalizedNeedle = normalized(needle);
    return normalizedValue.find(normalizedNeedle) != std::string::npos;
}

} // namespace

bool Playlist::addTrack(AudioTrack track)
{
    if (track.audioSource.empty() || containsSource(track.audioSource)) {
        return false;
    }

    m_tracks.push_back(std::move(track));
    if (!m_currentIndex) {
        m_currentIndex = 0;
    }

    return true;
}

bool Playlist::removeTrack(std::size_t index)
{
    if (index >= m_tracks.size()) {
        return false;
    }

    m_tracks.erase(m_tracks.begin() + static_cast<std::ptrdiff_t>(index));

    if (m_tracks.empty()) {
        m_currentIndex.reset();
    } else if (m_currentIndex == index) {
        m_currentIndex = std::min(index, m_tracks.size() - 1);
    } else if (m_currentIndex && *m_currentIndex > index) {
        --(*m_currentIndex);
    }

    return true;
}

void Playlist::clear()
{
    m_tracks.clear();
    m_currentIndex.reset();
}

std::size_t Playlist::size() const noexcept
{
    return m_tracks.size();
}

bool Playlist::empty() const noexcept
{
    return m_tracks.empty();
}

const AudioTrack* Playlist::trackAt(std::size_t index) const noexcept
{
    if (index >= m_tracks.size()) {
        return nullptr;
    }

    return &m_tracks[index];
}

std::span<const AudioTrack> Playlist::tracks() const noexcept
{
    return m_tracks;
}

bool Playlist::containsSource(std::string_view audioSource) const
{
    return std::ranges::any_of(m_tracks, [audioSource](const AudioTrack& track) {
        return track.audioSource == audioSource;
    });
}

PlayMode Playlist::playMode() const noexcept
{
    return m_playMode;
}

void Playlist::setPlayMode(PlayMode mode) noexcept
{
    m_playMode = mode;
}

std::optional<std::size_t> Playlist::currentIndex() const noexcept
{
    return m_currentIndex;
}

void Playlist::setCurrentIndex(std::optional<std::size_t> index) noexcept
{
    if (index && *index >= m_tracks.size()) {
        m_currentIndex.reset();
        return;
    }

    m_currentIndex = index;
}

std::optional<std::size_t> Playlist::nextIndex(std::optional<std::size_t> shuffleIndex) const noexcept
{
    if (m_tracks.empty()) {
        return std::nullopt;
    }

    if (m_playMode == PlayMode::Shuffle) {
        if (shuffleIndex && *shuffleIndex < m_tracks.size()) {
            return shuffleIndex;
        }
        return m_currentIndex.value_or(0);
    }

    if (!m_currentIndex) {
        return 0;
    }

    return (*m_currentIndex + 1) % m_tracks.size();
}

std::optional<std::size_t> Playlist::previousIndex(std::optional<std::size_t> shuffleIndex) const noexcept
{
    if (m_tracks.empty()) {
        return std::nullopt;
    }

    if (m_playMode == PlayMode::Shuffle) {
        if (shuffleIndex && *shuffleIndex < m_tracks.size()) {
            return shuffleIndex;
        }
        return m_currentIndex.value_or(0);
    }

    if (!m_currentIndex) {
        return m_tracks.size() - 1;
    }

    return (*m_currentIndex + m_tracks.size() - 1) % m_tracks.size();
}

bool matchesSearch(const AudioTrack& track, std::string_view searchText)
{
    if (searchText.empty()) {
        return false;
    }

    return containsCaseInsensitive(track.title, searchText)
        || containsCaseInsensitive(track.authorName, searchText);
}

std::vector<PlaylistSearchResult> searchTracks(
    std::span<const AudioTrack> tracks,
    std::string_view searchText)
{
    std::vector<PlaylistSearchResult> results;
    std::unordered_set<std::string> seenSources;

    if (searchText.empty()) {
        return results;
    }

    for (std::size_t index = 0; index < tracks.size(); ++index) {
        const AudioTrack& track = tracks[index];
        if (track.audioSource.empty() || !matchesSearch(track, searchText)) {
            continue;
        }

        if (!seenSources.insert(track.audioSource).second) {
            continue;
        }

        results.push_back(PlaylistSearchResult{.originalIndex = index});
    }

    return results;
}

PlaylistNameValidationError validatePlaylistName(std::u16string_view name, std::size_t maxLength) noexcept
{
    if (name.empty()) {
        return PlaylistNameValidationError::Empty;
    }

    if (name.size() > maxLength) {
        return PlaylistNameValidationError::TooLong;
    }

    constexpr std::u16string_view invalidCharacters = u"<>:\"/\\|?*";
    for (char16_t character : name) {
        if (invalidCharacters.find(character) != std::u16string_view::npos) {
            return PlaylistNameValidationError::InvalidCharacter;
        }
    }

    return PlaylistNameValidationError::None;
}

std::optional<std::size_t> playlistCurrentIndexAfterRemoval(
    std::size_t itemCount,
    std::optional<std::size_t> currentIndex,
    std::size_t removedIndex) noexcept
{
    if (itemCount == 0 || removedIndex >= itemCount || !currentIndex || *currentIndex >= itemCount) {
        return std::nullopt;
    }

    const std::size_t itemCountAfterRemoval = itemCount - 1;
    if (itemCountAfterRemoval == 0) {
        return std::nullopt;
    }

    if (*currentIndex == removedIndex) {
        return std::min(removedIndex, itemCountAfterRemoval - 1);
    }

    if (*currentIndex > removedIndex) {
        return *currentIndex - 1;
    }

    return currentIndex;
}

} // namespace SongPlayer::Core
