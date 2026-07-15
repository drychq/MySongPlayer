#include "adapters/QtAudioTrackAdapter.h"
#include "core/Playlist.h"
#include "services/PlaylistStorageService.h"

#include <QCoreApplication>
#include <QStringList>

#include <iostream>
#include <string_view>

namespace {

int failures = 0;

void expect(bool condition, std::string_view message)
{
    if (condition) {
        return;
    }

    std::cerr << "FAILED: " << message << '\n';
    ++failures;
}

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication application(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("MySongPlayerTests"));
    QCoreApplication::setApplicationName(QStringLiteral("PlaylistStorage"));

    PlaylistStorageService storage;
    expect(storage.initialize(), "storage initializes");

    const QString defaultName = SongPlayer::QtAdapter::fromUtf8String(
        SongPlayer::Core::kDefaultPlaylistName);
    const QStringList playlistNames = storage.getAllPlaylistNames();

    expect(playlistNames.count(defaultName) == 1, "default playlist exists exactly once");
    expect(!playlistNames.contains(QStringLiteral("Default playlist")),
           "legacy inconsistent default name is not created");
    expect(!storage.deletePlaylist(defaultName), "default playlist cannot be deleted");

    storage.shutdown();
    expect(!storage.isInitialized(), "storage shuts down cleanly");
    return failures == 0 ? 0 : 1;
}
