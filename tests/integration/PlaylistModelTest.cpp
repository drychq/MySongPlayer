#include "coordinators/PlaylistCoordinator.h"
#include "models/AudioInfo.h"
#include "models/PlaylistModel.h"
#include "models/PlaylistSearchModel.h"
#include "services/PlaylistStorageService.h"

#include <QCoreApplication>
#include <QUrl>

#include <iostream>
#include <optional>

using std::cerr;
using std::nullopt;
using std::optional;
using std::size_t;

namespace {

int failures{};

void expect(bool condition, const char *message)
{
    if (condition) {
        return;
    }
    cerr << "FAILED: " << message << '\n';
    ++failures;
}

QUrl sourceFor(const QString &name)
{
    return QUrl{QStringLiteral("file:///virtual/") + name + QStringLiteral(".mp3")};
}

bool addTrack(PlaylistModel &model, const QString &name)
{
    return model.addAudio(name, QStringLiteral("Artist"), sourceFor(name), {});
}

void verifiesInsertionAndSingleSourceSignal()
{
    PlaylistModel model;
    PlaylistStorageService storage;
    PlaylistCoordinator coordinator{&model, &storage};
    int sourceChanges{};
    QObject::connect(&coordinator, &PlaylistCoordinator::requestAudioSourceChange,
                     &model, [&](const QUrl &) { ++sourceChanges; });

    expect(addTrack(model, QStringLiteral("one")), "first track is inserted");
    expect(sourceChanges == 1, "first track requests its source exactly once");
    expect(addTrack(model, QStringLiteral("two")), "second track is inserted");
    expect(sourceChanges == 1, "non-current insertion does not request a source");

    coordinator.switchToAudioByIndex(1);
    expect(sourceChanges == 2, "switching tracks requests one source change");
    coordinator.switchToAudioByIndex(1);
    expect(sourceChanges == 2, "selecting the current track does not reload its source");
    coordinator.switchToPreviousSong();
    expect(sourceChanges == 3, "previous-track navigation requests one source change");

    expect(!addTrack(model, QStringLiteral("one")), "duplicate source is rejected");
    expect(model.currentSong() == model.getAudioInfoAtIndex(0),
           "duplicate insertion leaves the current song unchanged");
    expect(sourceChanges == 3, "duplicate insertion does not reload or switch audio");
}

void verifiesRemovalBehavior()
{
    {
        PlaylistModel model;
        addTrack(model, QStringLiteral("one"));
        addTrack(model, QStringLiteral("two"));
        addTrack(model, QStringLiteral("three"));
        AudioInfo *third{model.getAudioInfoAtIndex(2)};
        model.setCurrentSong(third);
        int changes{};
        QObject::connect(&model, &PlaylistModel::currentSongChanged,
                         &model, [&] { ++changes; });

        model.removeAudio(-1);
        model.removeAudio(3);
        expect(model.rowCount() == 3, "invalid removal indexes are ignored");
        model.removeAudio(0);
        expect(model.currentSong() == third, "removing a non-current track preserves current song");
        expect(model.currentIndex() == optional<size_t>{1},
               "removing before current song updates its index");
        expect(changes == 0, "non-current removal does not emit current-song change");
    }

    {
        PlaylistModel model;
        addTrack(model, QStringLiteral("one"));
        addTrack(model, QStringLiteral("two"));
        addTrack(model, QStringLiteral("three"));
        AudioInfo *third{model.getAudioInfoAtIndex(2)};
        model.setCurrentSong(model.getAudioInfoAtIndex(1));
        int changes{};
        QObject::connect(&model, &PlaylistModel::currentSongChanged,
                         &model, [&] { ++changes; });

        model.removeAudio(1);
        expect(model.currentSong() == third, "removing current middle track selects its successor");
        expect(model.currentIndex() == optional<size_t>{1},
               "middle successor receives the removed index");
        expect(changes == 1, "current middle removal emits one current-song change");
    }

    {
        PlaylistModel model;
        addTrack(model, QStringLiteral("one"));
        addTrack(model, QStringLiteral("two"));
        addTrack(model, QStringLiteral("three"));
        AudioInfo *second{model.getAudioInfoAtIndex(1)};
        model.setCurrentSong(model.getAudioInfoAtIndex(2));
        model.removeAudio(2);
        expect(model.currentSong() == second, "removing current tail selects its predecessor");
        expect(model.currentIndex() == optional<size_t>{1}, "tail predecessor keeps its index");
    }

    {
        PlaylistModel model;
        addTrack(model, QStringLiteral("only"));
        model.removeAudio(0);
        expect(model.rowCount() == 0, "removing the only track empties the playlist");
        expect(model.currentSong() == nullptr, "removing the only track clears current song");
        expect(model.currentIndex() == nullopt, "empty playlist has no current index");
    }
}

void verifiesInvalidPlayModeIsRejected()
{
    PlaylistModel model;
    model.setPlayMode(PlayMode::Shuffle);
    const PlayMode acceptedMode{model.playMode()};
    int changes{};
    QObject::connect(&model, &PlaylistModel::playModeChanged,
                     &model, [&] { ++changes; });

    // Deliberately exercise the model's defensive enum validation at its C++ boundary.
    model.setPlayMode(static_cast<PlayMode>(999)); // NOLINT(clang-analyzer-optin.core.EnumCastOutOfRange)
    expect(model.playMode() == acceptedMode, "invalid play mode is rejected");
    expect(changes == 0, "invalid play mode does not emit a change signal");
}

void verifiesLocalSearchPreservesZeroIndex()
{
    PlaylistSearchModel searchModel;
    AudioInfo first;
    first.setTitle(QStringLiteral("Needle"));
    first.setAuthorName(QStringLiteral("Artist"));
    first.setAudioSource(sourceFor(QStringLiteral("needle")));
    AudioInfo second;
    second.setTitle(QStringLiteral("Other"));
    second.setAuthorName(QStringLiteral("Artist"));
    second.setAudioSource(sourceFor(QStringLiteral("other")));
    const QVariantList tracks{QVariant::fromValue<QObject *>(&first),
                              QVariant::fromValue<QObject *>(&second)};

    searchModel.performSearch(tracks, QStringLiteral("Needle"));
    expect(searchModel.rowCount() == 1, "local search finds the first track");
    expect(searchModel.data(searchModel.index(0, 0), PlaylistSearchModel::OriginalIndexRole).toInt() == 0,
           "local search preserves original index zero");
}

} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication application{argc, argv};
    QCoreApplication::setApplicationName(QStringLiteral("MySongPlayerModelTests"));
    verifiesInsertionAndSingleSourceSignal();
    verifiesRemovalBehavior();
    verifiesInvalidPlayModeIsRejected();
    verifiesLocalSearchPreservesZeroIndex();
    return failures == 0 ? 0 : 1;
}
