#include "core/AudioImport.h"
#include "core/Playlist.h"
#include "core/Lyrics.h"

#include <iostream>
#include <optional>
#include <string>
#include <utility>
#include <vector>

using std::cerr;
using std::move;
using std::nullopt;
using std::string;
using std::u16string;
using std::vector;

namespace {

#define CHECK(condition)                                                        \
    do {                                                                        \
        if (!(condition)) {                                                     \
            cerr << "FAILED: " #condition << " at line " << __LINE__           \
                      << '\n';                                                  \
            return 1;                                                           \
        }                                                                       \
    } while (false)

SongPlayer::Core::AudioTrack track(
    string title,
    string author,
    string source)
{
    return SongPlayer::Core::AudioTrack{
        .title = move(title),
        .authorName = move(author),
        .audioSource = move(source),
        .imageSource = {},
        .videoSource = {},
    };
}

} // namespace

int main()
{
    SongPlayer::Core::Playlist playlist;

    CHECK(playlist.addTrack(track("Alpha Song", "Alice", "file:///alpha.mp3")));
    CHECK(!playlist.addTrack(track("Duplicate", "Someone", "file:///alpha.mp3")));
    CHECK(playlist.addTrack(track("Beta Song", "Bob", "file:///beta.mp3")));
    CHECK(playlist.size() == 2);
    CHECK(playlist.currentIndex() == 0);
    CHECK(playlist.containsSource("file:///beta.mp3"));
    CHECK(playlist.nextIndex() == 1);
    CHECK(playlist.previousIndex() == 1);

    playlist.setCurrentIndex(1);
    CHECK(playlist.nextIndex() == 0);
    CHECK(playlist.previousIndex() == 0);

    playlist.setPlayMode(SongPlayer::Core::PlayMode::Shuffle);
    CHECK(playlist.playMode() == SongPlayer::Core::PlayMode::Shuffle);
    CHECK(playlist.nextIndex(0) == 0);
    CHECK(playlist.previousIndex(1) == 1);

    playlist.setCurrentIndex(1);
    CHECK(playlist.removeTrack(0));
    CHECK(playlist.size() == 1);
    CHECK(playlist.currentIndex() == 0);

    playlist.setCurrentIndex(nullopt);
    playlist.setPlayMode(SongPlayer::Core::PlayMode::Loop);
    CHECK(playlist.nextIndex() == 0);
    CHECK(playlist.previousIndex() == 0);

    using NameError = SongPlayer::Core::PlaylistNameValidationError;
    CHECK(SongPlayer::Core::kDefaultPlaylistName == "Default Playlist");
    CHECK(SongPlayer::Core::validatePlaylistName(u"Favorites") == NameError::None);
    CHECK(SongPlayer::Core::validatePlaylistName(u"") == NameError::Empty);
    CHECK(SongPlayer::Core::validatePlaylistName(u"Bad/Name") == NameError::InvalidCharacter);
    CHECK(SongPlayer::Core::validatePlaylistName(u16string(101, u'a')) == NameError::TooLong);

    CHECK(SongPlayer::Core::kUnknownArtistName == "Unknown Artist");
    CHECK(SongPlayer::Core::coverFileNameForAudioStem("song") == "song_cover.jpg");
    CHECK(SongPlayer::Core::coverFileNameForAudio("song", "/music/a/song.mp3", "png") !=
          SongPlayer::Core::coverFileNameForAudio("song", "/music/b/song.mp3", "png"));
    CHECK(SongPlayer::Core::imageMimeTypeForExtension("jpg") == "image/jpeg");
    CHECK(SongPlayer::Core::imageMimeTypeForExtension("JPEG") == "image/jpeg");
    CHECK(SongPlayer::Core::imageMimeTypeForExtension("png") == "image/png");
    CHECK(SongPlayer::Core::imageMimeTypeForExtension("webp") == "image/unknown");
    CHECK(SongPlayer::Core::imageExtensionForMimeType("image/png") == "png");
    CHECK(SongPlayer::Core::imageExtensionForMimeType("image/webp") == "img");

    const vector<SongPlayer::Core::AudioTrack> tracks{
        track("Morning Light", "Composer", "file:///morning.mp3"),
        track("morning light live", "Composer", "file:///morning-live.mp3"),
        track("Other", "Artist", "file:///other.mp3"),
        track("Morning Duplicate", "Composer", "file:///morning.mp3"),
    };

    const auto results{SongPlayer::Core::searchTracks(tracks, "MORNING")};
    CHECK(results.size() == 2);
    CHECK(results[0].originalIndex == 0);
    CHECK(results[1].originalIndex == 1);

    const auto lyrics{SongPlayer::Core::parseLrcContent(
        "[00:10.00][00:20.500]Hello\n"
        "[00:15.25]Middle\n"
        "[metadata:ignored]\n"
        "[00:30.5]Later\n")};
    CHECK(lyrics.size() == 4);
    CHECK(lyrics[0].timestampMs == 10000);
    CHECK(lyrics[0].text == "Hello");
    CHECK(lyrics[1].timestampMs == 15250);
    CHECK(lyrics[2].timestampMs == 20500);
    CHECK(lyrics[3].timestampMs == 30500);
    CHECK(SongPlayer::Core::parseLrcTimestamp("00:01.1") == 1100);
    CHECK(SongPlayer::Core::parseLrcTimestamp("00:01.12") == 1120);
    CHECK(SongPlayer::Core::parseLrcTimestamp("00:01.123") == 1123);
    CHECK(SongPlayer::Core::parseLrcTimestamp("999999:59.999") == 59999999999LL);
    CHECK(!SongPlayer::Core::parseLrcTimestamp("-1:00"));
    CHECK(!SongPlayer::Core::parseLrcTimestamp("00:00."));

    CHECK(!SongPlayer::Core::lyricIndexAtPosition(lyrics, 9999));
    CHECK(SongPlayer::Core::lyricIndexAtPosition(lyrics, 10000) == 0);
    CHECK(SongPlayer::Core::lyricIndexAtPosition(lyrics, 16000) == 1);
    CHECK(SongPlayer::Core::lyricIndexAtPosition(lyrics, 21000) == 2);

    CHECK(SongPlayer::Core::normalizeLyricFileName("Song Title - Lyrics.lrc") == "songtitle");
    CHECK(SongPlayer::Core::lyricFileMatchScore("Song Title", "Song Title Lyrics") >= 80.0);

    return 0;
}
