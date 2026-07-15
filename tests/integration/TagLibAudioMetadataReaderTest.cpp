#include "core/AudioImport.h"
#include "infrastructure/TagLibAudioMetadataReader.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace {

int failures = 0;

void expect(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        ++failures;
    }
}

class TemporaryDirectory {
public:
    TemporaryDirectory()
        : m_path(std::filesystem::temp_directory_path() /
                 ("mysongplayer-metadata-reader-test-" + std::to_string(
                     std::chrono::steady_clock::now().time_since_epoch().count())))
    {
        std::error_code error;
        std::filesystem::remove_all(m_path, error);
        std::filesystem::create_directories(m_path);
    }

    ~TemporaryDirectory()
    {
        std::error_code error;
        std::filesystem::remove_all(m_path, error);
    }

    [[nodiscard]] const std::filesystem::path& path() const noexcept
    {
        return m_path;
    }

private:
    std::filesystem::path m_path;
};

} // namespace

int main()
{
    TemporaryDirectory temporary;
    const std::filesystem::path audioFile = temporary.path() / "fallback-title.mp3";
    std::ofstream(audioFile, std::ios::binary).put('\0');

    SongPlayer::Infrastructure::TagLibAudioMetadataReader reader;
    const SongPlayer::Core::AudioImportResult imported = reader.read({
        .audioFile = audioFile,
        .coverCacheDirectory = temporary.path() / "covers",
    });

    expect(imported.has_value(), "a regular file gets a deterministic fallback track");
    if (imported) {
        expect(imported->title == "fallback-title", "the file stem is used when metadata is absent");
        expect(imported->artist == SongPlayer::Core::kUnknownArtistName,
               "missing artist metadata uses the core fallback value");
        expect(imported->audioFile == audioFile, "the source path round-trips unchanged");
        expect(!imported->coverFile, "a file without APIC data has no cached cover");
    }

    const auto missing = reader.read({
        .audioFile = temporary.path() / "missing.mp3",
        .coverCacheDirectory = temporary.path() / "covers",
    });
    expect(!missing.has_value(), "a missing path is reported as an error");
    if (!missing) {
        expect(missing.error().code == SongPlayer::Core::AudioImportErrorCode::InvalidPath,
               "a missing path has the InvalidPath error code");
    }

    return failures == 0 ? 0 : 1;
}
