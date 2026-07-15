#pragma once

#include "core/AudioImport.h"

namespace SongPlayer::Infrastructure {

class TagLibAudioMetadataReader final : public Core::IAudioMetadataReader {
public:
    [[nodiscard]] Core::AudioImportResult read(
        const Core::AudioImportRequest& request) const noexcept override;
};

} // namespace SongPlayer::Infrastructure
