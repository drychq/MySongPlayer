#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace SongPlayer::Core {

struct LyricLine {
    std::int64_t timestampMs = 0;
    std::string text;

    [[nodiscard]] bool operator<(const LyricLine& other) const noexcept
    {
        return timestampMs < other.timestampMs;
    }
};

[[nodiscard]] std::vector<LyricLine> parseLrcContent(std::string_view content);

[[nodiscard]] std::optional<std::int64_t> parseLrcTimestamp(std::string_view timestamp);

[[nodiscard]] std::optional<std::size_t> lyricIndexAtPosition(
    std::span<const LyricLine> lyrics,
    std::int64_t positionMs);

[[nodiscard]] std::string normalizeLyricFileName(std::string_view fileName);

[[nodiscard]] double lyricFileMatchScore(std::string_view audioName, std::string_view lrcName);

} // namespace SongPlayer::Core
