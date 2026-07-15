#include "core/AudioImport.h"

#include <array>
#include <cctype>
#include <charconv>
#include <cstdint>

namespace SongPlayer::Core {
namespace {

constexpr std::string_view kCoverSuffix = "_cover.jpg";
constexpr std::string_view kMimeJpeg = "image/jpeg";
constexpr std::string_view kMimePng = "image/png";
constexpr std::string_view kMimeUnknown = "image/unknown";
constexpr std::string_view kExtensionJpeg = "jpg";
constexpr std::string_view kExtensionPng = "png";
constexpr std::string_view kExtensionUnknown = "img";

constexpr std::uint64_t fnv1a(std::string_view value) noexcept
{
    constexpr std::uint64_t offsetBasis = 14695981039346656037ULL;
    constexpr std::uint64_t prime = 1099511628211ULL;

    std::uint64_t hash = offsetBasis;
    for (const unsigned char byte : value) {
        hash ^= byte;
        hash *= prime;
    }
    return hash;
}

bool equalsIgnoreCase(std::string_view lhs, std::string_view rhs) noexcept
{
    if (lhs.size() != rhs.size()) {
        return false;
    }

    for (std::size_t index = 0; index < lhs.size(); ++index) {
        const auto lhsChar = static_cast<unsigned char>(lhs[index]);
        const auto rhsChar = static_cast<unsigned char>(rhs[index]);
        if (std::tolower(lhsChar) != std::tolower(rhsChar)) {
            return false;
        }
    }

    return true;
}

} // namespace

std::string coverFileNameForAudioStem(std::string_view audioStem)
{
    std::string result;
    result.reserve(audioStem.size() + kCoverSuffix.size());
    result.append(audioStem);
    result.append(kCoverSuffix);
    return result;
}

std::string coverFileNameForAudio(
    std::string_view audioStem,
    std::string_view sourceIdentity,
    std::string_view extension)
{
    std::array<char, 16> hashBuffer{};
    const auto [hashEnd, error] = std::to_chars(
        hashBuffer.data(), hashBuffer.data() + hashBuffer.size(), fnv1a(sourceIdentity), 16);

    std::string result;
    result.reserve(audioStem.size() + hashBuffer.size() + extension.size() + 2);
    result.append(audioStem);
    result.push_back('_');
    if (error == std::errc{}) {
        result.append(hashBuffer.data(), hashEnd);
    }
    result.push_back('.');
    result.append(extension.empty() ? kExtensionJpeg : extension);
    return result;
}

std::string_view imageMimeTypeForExtension(std::string_view extension) noexcept
{
    if (equalsIgnoreCase(extension, "jpg") || equalsIgnoreCase(extension, "jpeg")) {
        return kMimeJpeg;
    }

    if (equalsIgnoreCase(extension, "png")) {
        return kMimePng;
    }

    return kMimeUnknown;
}

std::string_view imageExtensionForMimeType(std::string_view mimeType) noexcept
{
    if (equalsIgnoreCase(mimeType, kMimePng)) {
        return kExtensionPng;
    }
    if (equalsIgnoreCase(mimeType, kMimeJpeg) ||
        equalsIgnoreCase(mimeType, "image/jpg")) {
        return kExtensionJpeg;
    }
    return kExtensionUnknown;
}

} // namespace SongPlayer::Core
