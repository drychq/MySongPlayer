#pragma once

#include <string>

namespace SongPlayer::Core {

struct AudioTrack {
    int songIndex{-1};
    std::string title;
    std::string authorName;
    std::string audioSource;
    std::string imageSource;
    std::string videoSource;
};

} // namespace SongPlayer::Core
