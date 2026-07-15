#pragma once

#include "core/AudioTrack.h"
#include "core/PlayMode.h"

#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace SongPlayer::Core {

inline constexpr std::string_view kDefaultPlaylistName = "Default Playlist";

struct PlaylistSearchResult {
    std::size_t originalIndex = 0;
};

enum class PlaylistNameValidationError {
    None,
    Empty,
    TooLong,
    InvalidCharacter
};

class Playlist {
public:
    bool addTrack(AudioTrack track);
    bool removeTrack(std::size_t index);
    void clear();

    [[nodiscard]] std::size_t size() const noexcept;
    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] const AudioTrack* trackAt(std::size_t index) const noexcept;
    [[nodiscard]] std::span<const AudioTrack> tracks() const noexcept;

    [[nodiscard]] bool containsSource(std::string_view audioSource) const;

    [[nodiscard]] PlayMode playMode() const noexcept;
    void setPlayMode(PlayMode mode) noexcept;

    [[nodiscard]] std::optional<std::size_t> currentIndex() const noexcept;
    void setCurrentIndex(std::optional<std::size_t> index) noexcept;

    [[nodiscard]] std::optional<std::size_t> nextIndex(
        std::optional<std::size_t> shuffleIndex = std::nullopt) const noexcept;
    [[nodiscard]] std::optional<std::size_t> previousIndex(
        std::optional<std::size_t> shuffleIndex = std::nullopt) const noexcept;

private:
    std::vector<AudioTrack> m_tracks;
    PlayMode m_playMode = PlayMode::Loop;
    std::optional<std::size_t> m_currentIndex;
};

[[nodiscard]] bool matchesSearch(const AudioTrack& track, std::string_view searchText);

[[nodiscard]] std::vector<PlaylistSearchResult> searchTracks(
    std::span<const AudioTrack> tracks,
    std::string_view searchText);

[[nodiscard]] PlaylistNameValidationError validatePlaylistName(
    std::u16string_view name,
    std::size_t maxLength = 100) noexcept;

[[nodiscard]] std::optional<std::size_t> playlistCurrentIndexAfterRemoval(
    std::size_t itemCount,
    std::optional<std::size_t> currentIndex,
    std::size_t removedIndex) noexcept;

} // namespace SongPlayer::Core
