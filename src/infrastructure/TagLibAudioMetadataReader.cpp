#include "infrastructure/TagLibAudioMetadataReader.h"

#include "core/AudioImport.h"

#include <taglib/attachedpictureframe.h>
#include <taglib/fileref.h>
#include <taglib/id3v2tag.h>
#include <taglib/mpegfile.h>
#include <taglib/tag.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>

namespace SongPlayer::Infrastructure {
namespace {

struct EmbeddedPicture {
    TagLib::ByteVector bytes;
    std::string mimeType;
};

std::string pathToUtf8(const std::filesystem::path& path)
{
    const std::u8string value = path.generic_u8string();
    return {reinterpret_cast<const char*>(value.data()), value.size()};
}

std::filesystem::path pathFromUtf8(std::string_view value)
{
    const auto* begin = reinterpret_cast<const char8_t*>(value.data());
    return std::filesystem::path(std::u8string(begin, begin + value.size()));
}

std::string tagText(const TagLib::String& value)
{
    return value.to8Bit(true);
}

std::optional<EmbeddedPicture> embeddedMpegCover(const std::filesystem::path& audioFile)
{
    std::string extension = pathToUtf8(audioFile.extension());
    std::ranges::transform(extension, extension.begin(), [](unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });
    if (extension != ".mp3") {
        return std::nullopt;
    }

    TagLib::MPEG::File file(audioFile.c_str(), false);
    if (!file.isValid()) {
        return std::nullopt;
    }

    const TagLib::ID3v2::Tag* tag = file.ID3v2Tag(false);
    if (!tag) {
        return std::nullopt;
    }

    const TagLib::ID3v2::FrameList frames = tag->frameList("APIC");
    if (frames.isEmpty()) {
        return std::nullopt;
    }

    const TagLib::ID3v2::AttachedPictureFrame* selected = nullptr;
    for (const TagLib::ID3v2::Frame* frame : frames) {
        const auto* picture = static_cast<const TagLib::ID3v2::AttachedPictureFrame*>(frame);
        if (picture && picture->type() == TagLib::ID3v2::AttachedPictureFrame::FrontCover) {
            selected = picture;
            break;
        }
    }

    if (!selected) {
        selected = static_cast<const TagLib::ID3v2::AttachedPictureFrame*>(frames.front());
    }
    if (!selected || selected->picture().isEmpty()) {
        return std::nullopt;
    }

    return EmbeddedPicture{
        .bytes = selected->picture(),
        .mimeType = tagText(selected->mimeType()),
    };
}

std::optional<std::filesystem::path> cachePicture(
    const std::filesystem::path& audioFile,
    const std::filesystem::path& cacheDirectory,
    const EmbeddedPicture& picture)
{
    std::error_code error;
    std::filesystem::create_directories(cacheDirectory, error);
    if (error) {
        return std::nullopt;
    }

    const std::string stem = pathToUtf8(audioFile.stem());
    const std::string sourceIdentity = pathToUtf8(audioFile.lexically_normal());
    const std::string_view extension = Core::imageExtensionForMimeType(picture.mimeType);
    const std::filesystem::path target = cacheDirectory / pathFromUtf8(
        Core::coverFileNameForAudio(stem, sourceIdentity, extension));

    if (std::filesystem::is_regular_file(target, error) && !error) {
        return target;
    }
    error.clear();

    std::filesystem::path temporary = target;
    temporary += ".tmp";
    {
        std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
        if (!output) {
            return std::nullopt;
        }
        output.write(picture.bytes.data(), static_cast<std::streamsize>(picture.bytes.size()));
        if (!output) {
            output.close();
            std::filesystem::remove(temporary, error);
            return std::nullopt;
        }
    }

    std::filesystem::rename(temporary, target, error);
    if (error) {
        error.clear();
        if (std::filesystem::is_regular_file(target, error) && !error) {
            std::filesystem::remove(temporary, error);
            return target;
        }
        std::filesystem::remove(temporary, error);
        return std::nullopt;
    }

    return target;
}

} // namespace

Core::AudioImportResult TagLibAudioMetadataReader::read(
    const Core::AudioImportRequest& request) const noexcept
{
    try {
        std::error_code error;
        if (!std::filesystem::is_regular_file(request.audioFile, error) || error) {
            return std::unexpected(Core::AudioImportError{
                .code = Core::AudioImportErrorCode::InvalidPath,
                .audioFile = request.audioFile,
                .message = "Audio path is not a readable regular file",
            });
        }

        Core::ImportedAudio imported{
            .title = pathToUtf8(request.audioFile.stem()),
            .artist = std::string(Core::kUnknownArtistName),
            .audioFile = request.audioFile,
            .coverFile = std::nullopt,
        };

        TagLib::FileRef file(request.audioFile.c_str(), false);
        if (!file.isNull() && file.tag()) {
            const TagLib::Tag* tag = file.tag();
            if (!tag->title().isEmpty()) {
                imported.title = tagText(tag->title());
            }
            if (!tag->artist().isEmpty()) {
                imported.artist = tagText(tag->artist());
            }
        }

        if (const auto picture = embeddedMpegCover(request.audioFile)) {
            imported.coverFile = cachePicture(
                request.audioFile, request.coverCacheDirectory, *picture);
        }

        return imported;
    } catch (const std::exception& exception) {
        return std::unexpected(Core::AudioImportError{
            .code = Core::AudioImportErrorCode::UnexpectedFailure,
            .audioFile = request.audioFile,
            .message = exception.what(),
        });
    } catch (...) {
        return std::unexpected(Core::AudioImportError{
            .code = Core::AudioImportErrorCode::UnexpectedFailure,
            .audioFile = request.audioFile,
            .message = "Unknown metadata reader failure",
        });
    }
}

} // namespace SongPlayer::Infrastructure
