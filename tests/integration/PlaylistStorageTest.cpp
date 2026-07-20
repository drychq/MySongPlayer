#include "adapters/QtAudioTrackAdapter.h"
#include "core/Playlist.h"
#include "services/PlaylistStorageService.h"
#include "storage/PlaylistDatabase.h"

#include <QCoreApplication>
#include <QTemporaryDir>

#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

using std::cerr;
using std::move;
using std::nullopt;
using std::optional;
using std::size_t;
using std::string;
using std::string_view;
using std::vector;

namespace {

int failures{0};

void expect(bool condition, string_view message)
{
    if (condition) {
        return;
    }

    cerr << "FAILED: " << message << '\n';
    ++failures;
}

SongPlayer::Core::AudioTrack track(string title, string source)
{
    return SongPlayer::Core::AudioTrack{
        .title = move(title),
        .authorName = "Test Artist",
        .audioSource = move(source),
        .imageSource = "qrc:/qt/qml/MySongPlayer/assets/icons/app_icon.png",
        .videoSource = {},
    };
}

void verifyStorageBehavior(PlaylistStorageService& storage)
{
    const QString defaultName{SongPlayer::QtAdapter::fromUtf8String(
        SongPlayer::Core::kDefaultPlaylistName)};
    const QStringList playlistNames{storage.getAllPlaylistNames()};
    expect(playlistNames.count(defaultName) == 1, "default playlist exists exactly once");
    expect(!storage.deletePlaylist(defaultName), "default playlist cannot be deleted");
    expect(!storage.renamePlaylist(defaultName, QStringLiteral("Renamed")),
           "default playlist cannot be renamed");
    expect(!storage.renamePlaylist(QStringLiteral("Missing"), defaultName),
           "the default playlist name is reserved");
    expect(!storage.deletePlaylist(QStringLiteral("Missing")),
           "deleting a missing playlist reports failure");
    expect(!storage.renamePlaylist(QStringLiteral("Missing"), QStringLiteral("Other")),
           "renaming a missing playlist reports failure");

    const vector<SongPlayer::Core::AudioTrack> emptyPlaylist;
    expect(storage.savePlaylist(
               QStringLiteral("Empty"), emptyPlaylist, SongPlayer::Core::PlayMode::Shuffle, nullopt),
           "an empty playlist can be saved");
    const PlaylistInfo emptyRoundTrip{storage.loadPlaylist(QStringLiteral("Empty"))};
    expect(emptyRoundTrip.id >= 0, "an empty playlist can be loaded");
    expect(emptyRoundTrip.audioItems.empty(), "empty playlist remains empty");
    expect(!emptyRoundTrip.currentIndex, "empty playlist has no current index");
    expect(emptyRoundTrip.playMode == SongPlayer::Core::PlayMode::Shuffle,
           "empty playlist play mode round-trips");

    const vector<SongPlayer::Core::AudioTrack> tracks{
        track("Alpha", "file:///alpha.mp3"),
        track("Beta", "file:///beta.mp3"),
        track("Gamma", "file:///gamma.mp3"),
    };
    expect(storage.savePlaylist(
               QStringLiteral("Ordered"), tracks, SongPlayer::Core::PlayMode::Loop, optional<size_t>{1}),
           "ordered playlist saves");

    const qsizetype childCount{storage.children().size()};
    for (int iteration{0}; iteration < 10; ++iteration) {
        const PlaylistInfo loaded{storage.loadPlaylist(QStringLiteral("Ordered"))};
        expect(loaded.audioItems.size() == 3, "loaded playlist keeps all tracks");
        expect(loaded.currentIndex == optional<size_t>{1}, "current index round-trips");
        expect(loaded.audioItems[1].title == "Beta", "track order round-trips");
    }
    expect(storage.children().size() == childCount,
           "repeated loads do not accumulate temporary QObjects");

    expect(storage.renamePlaylist(QStringLiteral("Ordered"), QStringLiteral("Renamed")),
           "ordinary playlist can be renamed");
    expect(storage.loadPlaylist(QStringLiteral("Ordered")).id < 0,
           "old playlist name no longer exists after rename");
    expect(storage.loadPlaylist(QStringLiteral("Renamed")).id >= 0,
           "new playlist name loads after rename");
}

void verifyCommitFailureRollsBack()
{
    SongPlayer::PlaylistDatabase database;
    expect(database.initializeDatabase(), "database reopens for transaction test");
    expect(database.executeNonQuery(QStringLiteral(
               "CREATE TABLE IF NOT EXISTS tx_parent (id INTEGER PRIMARY KEY)")),
           "deferred transaction parent table is created");
    expect(database.executeNonQuery(QStringLiteral(
               "CREATE TABLE IF NOT EXISTS tx_child ("
               "parent_id INTEGER, FOREIGN KEY(parent_id) REFERENCES tx_parent(id) "
               "DEFERRABLE INITIALLY DEFERRED)")),
           "deferred transaction child table is created");

    const bool invalidCommit{database.runInTransaction([&database]() {
        return database.executeNonQuery(
            QStringLiteral("INSERT INTO tx_child(parent_id) VALUES (99)"));
    })};
    expect(!invalidCommit, "deferred foreign-key failure makes commit fail");

    const bool validCommit{database.runInTransaction([&database]() {
        return database.executeNonQuery(QStringLiteral("INSERT INTO tx_parent(id) VALUES (99)")) &&
               database.executeNonQuery(
                   QStringLiteral("INSERT INTO tx_child(parent_id) VALUES (99)"));
    })};
    expect(validCommit, "rollback after failed commit leaves the next transaction usable");
    database.closeDatabase();
}

} // namespace

int main(int argc, char* argv[])
{
    QTemporaryDir dataDirectory;
    expect(dataDirectory.isValid(), "temporary data directory is available");
    qputenv("XDG_DATA_HOME", dataDirectory.path().toUtf8());

    QCoreApplication application{argc, argv};
    QCoreApplication::setOrganizationName(QStringLiteral("MySongPlayerTests"));
    QCoreApplication::setApplicationName(QStringLiteral("PlaylistStorage"));

    PlaylistStorageService storage;
    expect(storage.initialize(), "storage initializes");
    verifyStorageBehavior(storage);
    storage.shutdown();
    expect(!storage.isInitialized(), "storage shuts down cleanly");

    verifyCommitFailureRollsBack();
    return failures == 0 ? 0 : 1;
}
