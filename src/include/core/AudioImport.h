#pragma once

#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace SongPlayer::Core {

inline constexpr std::string_view kUnknownArtistName = "Unknown Artist";
inline constexpr std::string_view kCoverCacheDirectoryName = "covers";

enum class AudioImportErrorCode {
    InvalidPath,
    UnexpectedFailure,
};

struct ImportedAudio {
    std::string title;
    std::string artist;
    std::filesystem::path audioFile;
    std::optional<std::filesystem::path> coverFile;
};

struct AudioImportError {
    AudioImportErrorCode code = AudioImportErrorCode::UnexpectedFailure;
    std::filesystem::path audioFile;
    std::string message;
};

struct AudioImportRequest {
    std::filesystem::path audioFile;
    std::filesystem::path coverCacheDirectory;
};

using AudioImportResult = std::expected<ImportedAudio, AudioImportError>;

class IAudioMetadataReader {
public:
    virtual ~IAudioMetadataReader() = default;

    [[nodiscard]] virtual AudioImportResult read(
        const AudioImportRequest& request) const noexcept = 0;
};

[[nodiscard]] std::string coverFileNameForAudioStem(std::string_view audioStem);

[[nodiscard]] std::string coverFileNameForAudio(
    std::string_view audioStem,
    std::string_view sourceIdentity,
    std::string_view extension);

[[nodiscard]] std::string_view imageMimeTypeForExtension(std::string_view extension) noexcept;

[[nodiscard]] std::string_view imageExtensionForMimeType(std::string_view mimeType) noexcept;

} // namespace SongPlayer::Core
