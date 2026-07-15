#include "core/Lyrics.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <iterator>
#include <string_view>

namespace SongPlayer::Core {
namespace {

constexpr std::string_view kIgnoredTags[] = {
    "lyrics",
    "lrc",
    "karaoke",
    "vocal",
    "instrumental",
    "remix",
    "edit",
    "version",
};

bool isSpecialNameCharacter(char character)
{
    constexpr std::string_view specialCharacters = R"(()[]{}-_ .,;:!?'`~@#$%^&*+=|\/<>)";
    return specialCharacters.find(character) != std::string_view::npos || character == '"';
}

std::string trim(std::string_view value)
{
    const auto isNotSpace = [](unsigned char character) {
        return std::isspace(character) == 0;
    };

    const auto begin = std::ranges::find_if(value, isNotSpace);
    if (begin == value.end()) {
        return {};
    }

    const auto end = std::find_if(value.rbegin(), value.rend(), isNotSpace).base();
    return {begin, end};
}

std::vector<std::string_view> splitLines(std::string_view content)
{
    std::vector<std::string_view> lines;
    std::size_t start = 0;

    while (start <= content.size()) {
        const std::size_t end = content.find('\n', start);
        if (end == std::string_view::npos) {
            lines.push_back(content.substr(start));
            break;
        }

        lines.push_back(content.substr(start, end - start));
        start = end + 1;
    }

    return lines;
}

std::optional<int> parsePositiveInt(std::string_view value)
{
    if (value.empty()) {
        return std::nullopt;
    }

    int result = 0;
    const auto [ptr, error] = std::from_chars(value.data(), value.data() + value.size(), result);
    if (error != std::errc{} || ptr != value.data() + value.size()) {
        return std::nullopt;
    }

    return result;
}

void removeAll(std::string& value, std::string_view needle)
{
    std::size_t position = 0;
    while ((position = value.find(needle, position)) != std::string::npos) {
        value.erase(position, needle.size());
    }
}

} // namespace

std::optional<std::int64_t> parseLrcTimestamp(std::string_view timestamp)
{
    const std::size_t colon = timestamp.find(':');
    if (colon == std::string_view::npos) {
        return std::nullopt;
    }

    const std::string_view minutesPart = timestamp.substr(0, colon);
    std::string_view secondsPart = timestamp.substr(colon + 1);

    const std::size_t dot = secondsPart.find('.');
    std::string_view fractionPart;
    if (dot != std::string_view::npos) {
        fractionPart = secondsPart.substr(dot + 1);
        secondsPart = secondsPart.substr(0, dot);
    }

    if (secondsPart.size() != 2 || (!fractionPart.empty() && fractionPart.size() > 3)) {
        return std::nullopt;
    }

    const std::optional<int> minutes = parsePositiveInt(minutesPart);
    const std::optional<int> seconds = parsePositiveInt(secondsPart);
    if (!minutes || !seconds || *seconds > 59) {
        return std::nullopt;
    }

    std::int64_t milliseconds = (static_cast<std::int64_t>(*minutes) * 60 + *seconds) * 1000;
    if (!fractionPart.empty()) {
        const std::optional<int> fraction = parsePositiveInt(fractionPart);
        if (!fraction) {
            return std::nullopt;
        }

        if (fractionPart.size() == 1) {
            milliseconds += *fraction * 100;
        } else if (fractionPart.size() == 2) {
            milliseconds += *fraction * 10;
        } else {
            milliseconds += *fraction;
        }
    }

    return milliseconds;
}

std::vector<LyricLine> parseLrcContent(std::string_view content)
{
    std::vector<LyricLine> lyrics;

    for (std::string_view rawLine : splitLines(content)) {
        const std::string line = trim(rawLine);
        if (line.empty()) {
            continue;
        }

        std::vector<std::int64_t> timestamps;
        std::size_t cursor = 0;
        while (cursor < line.size() && line[cursor] == '[') {
            const std::size_t close = line.find(']', cursor + 1);
            if (close == std::string::npos) {
                break;
            }

            const std::string_view timestampText(line.data() + cursor + 1, close - cursor - 1);
            const std::optional<std::int64_t> timestamp = parseLrcTimestamp(timestampText);
            if (!timestamp) {
                break;
            }

            timestamps.push_back(*timestamp);
            cursor = close + 1;
        }

        if (timestamps.empty()) {
            continue;
        }

        const std::string text = trim(std::string_view(line).substr(cursor));
        if (text.empty()) {
            continue;
        }

        for (std::int64_t timestamp : timestamps) {
            lyrics.push_back(LyricLine{.timestampMs = timestamp, .text = text});
        }
    }

    std::ranges::sort(lyrics, {}, &LyricLine::timestampMs);
    return lyrics;
}

std::optional<std::size_t> lyricIndexAtPosition(
    std::span<const LyricLine> lyrics,
    std::int64_t positionMs)
{
    const auto iterator = std::ranges::upper_bound(
        lyrics,
        positionMs,
        {},
        &LyricLine::timestampMs);

    if (iterator == lyrics.begin()) {
        return std::nullopt;
    }

    return static_cast<std::size_t>(std::distance(lyrics.begin(), std::prev(iterator)));
}

std::string normalizeLyricFileName(std::string_view fileName)
{
    std::string result;
    result.reserve(fileName.size());

    for (unsigned char character : fileName) {
        const char lowered = static_cast<char>(std::tolower(character));
        if (!isSpecialNameCharacter(lowered)) {
            result.push_back(lowered);
        }
    }

    for (std::string_view tag : kIgnoredTags) {
        removeAll(result, tag);
    }

    return trim(result);
}

double lyricFileMatchScore(std::string_view audioName, std::string_view lrcName)
{
    const std::string cleanAudio = normalizeLyricFileName(audioName);
    const std::string cleanLrc = normalizeLyricFileName(lrcName);

    if (cleanAudio.empty() || cleanLrc.empty()) {
        return 0.0;
    }

    if (cleanAudio == cleanLrc) {
        return 100.0;
    }

    if (cleanLrc.find(cleanAudio) != std::string::npos) {
        const double lengthRatio = static_cast<double>(cleanAudio.size()) / static_cast<double>(lrcName.size());
        return 80.0 + (lengthRatio * 15.0);
    }

    if (cleanAudio.find(cleanLrc) != std::string::npos) {
        const double lengthRatio = static_cast<double>(cleanLrc.size()) / static_cast<double>(cleanAudio.size());
        return 75.0 + (lengthRatio * 10.0);
    }

    int commonCharacters = 0;
    const std::size_t maxLength = std::max(cleanAudio.size(), cleanLrc.size());
    for (char character : cleanAudio) {
        if (cleanLrc.find(character) != std::string::npos) {
            ++commonCharacters;
        }
    }

    return (static_cast<double>(commonCharacters) / static_cast<double>(maxLength)) * 60.0;
}

} // namespace SongPlayer::Core
