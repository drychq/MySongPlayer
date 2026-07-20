#include "core/Playlist.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <string>
#include <unordered_set>
#include <utility>

using std::min;
using std::nullopt;
using std::optional;
using std::ptrdiff_t;
using std::size_t;
using std::span;
using std::string;
using std::string_view;
using std::tolower;
using std::u16string_view;
using std::unordered_set;
using std::vector;
namespace ranges = std::ranges;

namespace SongPlayer::Core {
namespace {

string normalized(string_view text)
{
    string result;
    result.reserve(text.size());

    for (unsigned char character : text) {
        result.push_back(static_cast<char>(tolower(character)));
    }

    return result;
}

bool containsCaseInsensitive(string_view value, string_view needle)
{
    if (needle.empty()) {
        return true;
    }

    const string normalizedValue{normalized(value)};
    const string normalizedNeedle{normalized(needle)};
    return normalizedValue.find(normalizedNeedle) != string::npos;
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

bool Playlist::removeTrack(size_t index)
{
    if (index >= m_tracks.size()) {
        return false;
    }

    m_tracks.erase(m_tracks.begin() + static_cast<ptrdiff_t>(index));

    if (m_tracks.empty()) {
        m_currentIndex.reset();
    } else if (m_currentIndex == index) {
        m_currentIndex = min(index, m_tracks.size() - 1);
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

size_t Playlist::size() const noexcept
{
    return m_tracks.size();
}

bool Playlist::empty() const noexcept
{
    return m_tracks.empty();
}

const AudioTrack* Playlist::trackAt(size_t index) const noexcept
{
    if (index >= m_tracks.size()) {
        return nullptr;
    }

    return &m_tracks[index];
}

span<const AudioTrack> Playlist::tracks() const noexcept
{
    return m_tracks;
}

bool Playlist::containsSource(string_view audioSource) const
{
    return ranges::any_of(m_tracks, [audioSource](const AudioTrack& track) {
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

optional<size_t> Playlist::currentIndex() const noexcept
{
    return m_currentIndex;
}

void Playlist::setCurrentIndex(optional<size_t> index) noexcept
{
    if (index && *index >= m_tracks.size()) {
        m_currentIndex.reset();
        return;
    }

    m_currentIndex = index;
}

optional<size_t> Playlist::nextIndex(optional<size_t> shuffleIndex) const noexcept
{
    if (m_tracks.empty()) {
        return nullopt;
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

optional<size_t> Playlist::previousIndex(optional<size_t> shuffleIndex) const noexcept
{
    if (m_tracks.empty()) {
        return nullopt;
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

bool matchesSearch(const AudioTrack& track, string_view searchText)
{
    if (searchText.empty()) {
        return false;
    }

    return containsCaseInsensitive(track.title, searchText)
        || containsCaseInsensitive(track.authorName, searchText);
}

vector<PlaylistSearchResult> searchTracks(
    span<const AudioTrack> tracks,
    string_view searchText)
{
    vector<PlaylistSearchResult> results;
    unordered_set<string> seenSources;

    if (searchText.empty()) {
        return results;
    }

    for (size_t index{0}; index < tracks.size(); ++index) {
        const AudioTrack& track{tracks[index]};
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

PlaylistNameValidationError validatePlaylistName(u16string_view name, size_t maxLength) noexcept
{
    if (name.empty()) {
        return PlaylistNameValidationError::Empty;
    }

    if (name.size() > maxLength) {
        return PlaylistNameValidationError::TooLong;
    }

    constexpr u16string_view invalidCharacters{u"<>:\"/\\|?*"};
    for (char16_t character : name) {
        if (invalidCharacters.find(character) != u16string_view::npos) {
            return PlaylistNameValidationError::InvalidCharacter;
        }
    }

    return PlaylistNameValidationError::None;
}

} // namespace SongPlayer::Core
