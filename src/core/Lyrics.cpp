#include "core/Lyrics.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <iterator>
#include <string_view>

using std::distance;
using std::errc;
using std::find_if;
using std::from_chars;
using std::int64_t;
using std::isspace;
using std::max;
using std::nullopt;
using std::optional;
using std::prev;
using std::size_t;
using std::span;
using std::string;
using std::string_view;
using std::tolower;
using std::vector;
namespace ranges = std::ranges;

namespace SongPlayer::Core {
namespace {

constexpr string_view kIgnoredTags[]{
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
    constexpr string_view specialCharacters{R"(()[]{}-_ .,;:!?'`~@#$%^&*+=|\/<>)"};
    return specialCharacters.find(character) != string_view::npos || character == '"';
}

string trim(string_view value)
{
    const auto isNotSpace{[](unsigned char character) {
        return isspace(character) == 0;
    }};

    const auto begin{ranges::find_if(value, isNotSpace)};
    if (begin == value.end()) {
        return {};
    }

    const auto end{find_if(value.rbegin(), value.rend(), isNotSpace).base()};
    return {begin, end};
}

vector<string_view> splitLines(string_view content)
{
    vector<string_view> lines;
    size_t start{0};

    while (start <= content.size()) {
        const size_t end{content.find('\n', start)};
        if (end == string_view::npos) {
            lines.push_back(content.substr(start));
            break;
        }

        lines.push_back(content.substr(start, end - start));
        start = end + 1;
    }

    return lines;
}

optional<int> parsePositiveInt(string_view value)
{
    if (value.empty()) {
        return nullopt;
    }

    int result{0};
    const auto [ptr, error]{from_chars(value.data(), value.data() + value.size(), result)};
    if (error != errc{} || ptr != value.data() + value.size() || result < 0) {
        return nullopt;
    }

    return result;
}

void removeAll(string& value, string_view needle)
{
    size_t position{0};
    while ((position = value.find(needle, position)) != string::npos) {
        value.erase(position, needle.size());
    }
}

vector<int64_t> parseLineTimestamps(string_view line, size_t& cursor)
{
    vector<int64_t> timestamps{};
    while (cursor < line.size() && line[cursor] == '[') {
        const size_t close{line.find(']', cursor + 1)};
        if (close == string_view::npos) {
            break;
        }

        const string_view timestampText{line.data() + cursor + 1, close - cursor - 1};
        const optional<int64_t> timestamp{parseLrcTimestamp(timestampText)};
        if (!timestamp) {
            break;
        }

        timestamps.push_back(*timestamp);
        cursor = close + 1;
    }
    return timestamps;
}

} // namespace

optional<int64_t> parseLrcTimestamp(string_view timestamp)
{
    const size_t colon{timestamp.find(':')};
    if (colon == string_view::npos) {
        return nullopt;
    }

    const string_view minutesPart{timestamp.substr(0, colon)};
    string_view secondsPart{timestamp.substr(colon + 1)};

    const size_t dot{secondsPart.find('.')};
    const bool hasFractionSeparator{dot != string_view::npos};
    string_view fractionPart;
    if (hasFractionSeparator) {
        fractionPart = secondsPart.substr(dot + 1);
        secondsPart = secondsPart.substr(0, dot);
    }

    if (secondsPart.size() != 2 || (hasFractionSeparator && fractionPart.empty()) ||
        fractionPart.size() > 3) {
        return nullopt;
    }

    const optional<int> minutes{parsePositiveInt(minutesPart)};
    const optional<int> seconds{parsePositiveInt(secondsPart)};
    if (!minutes || !seconds || *seconds > 59) {
        return nullopt;
    }

    int64_t milliseconds{(static_cast<int64_t>(*minutes) * 60 + *seconds) * 1000};
    if (!fractionPart.empty()) {
        const optional<int> fraction{parsePositiveInt(fractionPart)};
        if (!fraction) {
            return nullopt;
        }

        if (fractionPart.size() == 1) {
            milliseconds += int64_t{*fraction} * 100;
        } else if (fractionPart.size() == 2) {
            milliseconds += int64_t{*fraction} * 10;
        } else {
            milliseconds += *fraction;
        }
    }

    return milliseconds;
}

vector<LyricLine> parseLrcContent(string_view content)
{
    vector<LyricLine> lyrics;

    for (string_view rawLine : splitLines(content)) {
        const string line{trim(rawLine)};
        if (line.empty()) {
            continue;
        }

        size_t cursor{0};
        const vector<int64_t> timestamps{parseLineTimestamps(line, cursor)};

        if (timestamps.empty()) {
            continue;
        }

        const string text{trim(string_view{line}.substr(cursor))};
        if (text.empty()) {
            continue;
        }

        for (int64_t timestamp : timestamps) {
            lyrics.push_back(LyricLine{.timestampMs = timestamp, .text = text});
        }
    }

    ranges::sort(lyrics, {}, &LyricLine::timestampMs);
    return lyrics;
}

optional<size_t> lyricIndexAtPosition(span<const LyricLine> lyrics, int64_t positionMs)
{
    const auto iterator{ranges::upper_bound(
        lyrics,
        positionMs,
        {},
        &LyricLine::timestampMs)};

    if (iterator == lyrics.begin()) {
        return nullopt;
    }

    return static_cast<size_t>(distance(lyrics.begin(), prev(iterator)));
}

string normalizeLyricFileName(string_view fileName)
{
    string result;
    result.reserve(fileName.size());

    for (unsigned char character : fileName) {
        const char lowered{static_cast<char>(tolower(character))};
        if (!isSpecialNameCharacter(lowered)) {
            result.push_back(lowered);
        }
    }

    for (string_view tag : kIgnoredTags) {
        removeAll(result, tag);
    }

    return trim(result);
}

double lyricFileMatchScore(string_view audioName, string_view lrcName)
{
    const string cleanAudio{normalizeLyricFileName(audioName)};
    const string cleanLrc{normalizeLyricFileName(lrcName)};

    if (cleanAudio.empty() || cleanLrc.empty()) {
        return 0.0;
    }

    if (cleanAudio == cleanLrc) {
        return 100.0;
    }

    if (cleanLrc.find(cleanAudio) != string::npos) {
        const double lengthRatio{static_cast<double>(cleanAudio.size()) /
                                 static_cast<double>(lrcName.size())};
        return 80.0 + (lengthRatio * 15.0);
    }

    if (cleanAudio.find(cleanLrc) != string::npos) {
        const double lengthRatio{static_cast<double>(cleanLrc.size()) /
                                 static_cast<double>(cleanAudio.size())};
        return 75.0 + (lengthRatio * 10.0);
    }

    int commonCharacters{0};
    const size_t maxLength{max(cleanAudio.size(), cleanLrc.size())};
    for (char character : cleanAudio) {
        if (cleanLrc.find(character) != string::npos) {
            ++commonCharacters;
        }
    }

    return (static_cast<double>(commonCharacters) / static_cast<double>(maxLength)) * 60.0;
}

} // namespace SongPlayer::Core
