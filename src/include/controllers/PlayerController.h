#pragma once

#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

#include "models/LyricsModel.h"
#include "models/PlaylistModel.h"

class AudioInfo;
class AudioPlayer;
class ICurrentSongManager;
class IPlaylistOperations;
class IPlaylistPersistence;
class LyricsService;
class PlaylistStorageService;
class QTimer;

namespace SongPlayer {
class AudioImporter;
}

class PlayerController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool playing READ playing NOTIFY playingChanged)
    Q_PROPERTY(qint64 duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(qint64 position READ position NOTIFY positionChanged)
    Q_PROPERTY(float volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(bool muted READ isMuted WRITE setMuted NOTIFY mutedChanged)
    Q_PROPERTY(AudioInfo* currentSong READ currentSong WRITE setCurrentSong NOTIFY currentSongChanged)
    Q_PROPERTY(int playMode READ playModeInt WRITE setPlayModeInt NOTIFY playModeChanged)
    Q_PROPERTY(LyricsModel* lyricsModel READ lyricsModel CONSTANT)
    Q_PROPERTY(bool importing READ importing NOTIFY importingChanged)
    Q_PROPERTY(int importCompleted READ importCompleted NOTIFY importProgressChanged)
    Q_PROPERTY(int importTotal READ importTotal NOTIFY importProgressChanged)

public:
    explicit PlayerController(QObject *parent = nullptr);

    bool playing() const;
    qint64 duration() const;
    qint64 position() const;

    float volume() const;
    void setVolume(float newVolume);

    bool isMuted() const;
    void setMuted(bool muted);

    AudioInfo *currentSong() const;
    void setCurrentSong(AudioInfo *newCurrentSong);

    PlayMode playMode() const;
    void setPlayMode(PlayMode newMode);
    int playModeInt() const;
    void setPlayModeInt(int newMode);

    LyricsModel *lyricsModel() const;
    bool importing() const;
    int importCompleted() const;
    int importTotal() const;

    Q_INVOKABLE void playPause();
    Q_INVOKABLE void setPosition(qint64 newPosition);
    Q_INVOKABLE void switchToNextSong();
    Q_INVOKABLE void switchToPreviousSong();
    Q_INVOKABLE void switchToAudioByIndex(int index);

    Q_INVOKABLE bool addAudio(const QString &title,
                              const QString &authorName,
                              const QUrl &audioSource,
                              const QUrl &imageSource,
                              const QUrl &videoSource = QUrl());
    Q_INVOKABLE void removeAudio(int index);
    Q_INVOKABLE void clearPlaylist();
    Q_INVOKABLE QList<QObject*> getPlaylistAudioInfoList() const;

    Q_INVOKABLE bool saveCurrentPlaylist(const QString &playlistName = QString());
    Q_INVOKABLE bool loadPlaylist(const QString &playlistName);
    Q_INVOKABLE QStringList getAllPlaylistNames() const;
    Q_INVOKABLE bool deletePlaylist(const QString &playlistName);
    Q_INVOKABLE bool renamePlaylist(const QString &oldName, const QString &newName);
    Q_INVOKABLE QString currentPlaylistName() const;

    Q_INVOKABLE void importLocalAudio(const QList<QUrl> &fileUrls);
    Q_INVOKABLE void cancelAudioImport();
    Q_INVOKABLE void addNetworkAudio(const QString &title,
                                     const QString &authorName,
                                     const QUrl &audioSource,
                                     const QUrl &imageSource);
    Q_INVOKABLE PlaylistModel *playlistModel() const;

signals:
    void playingChanged();
    void durationChanged();
    void positionChanged();
    void volumeChanged();
    void mutedChanged();
    void currentSongChanged();
    void playModeChanged();
    void duplicateAudioSkipped(const QString &title, const QString &reason);
    void importingChanged();
    void importProgressChanged();
    void importStarted(int total);
    void importFinished(int imported, int failed, bool canceled);
    void importRejected(const QString &reason);
    void importFailed(const QUrl &source, const QString &reason);

private slots:
    void onAudioSourceChangeRequested(const QUrl &source);
    void onAudioImported(const QString &title,
                         const QString &authorName,
                         const QUrl &audioSource,
                         const QUrl &imageSource);
    void onCurrentSongChanged();
    void onPositionChanged();
    void onPlaylistChanged();
    void onImportStarted();
    void onImportFinished();

private:
    void loadDefaultPlaylistOnStartup();

    AudioPlayer *m_audioPlayer{nullptr};
    PlaylistModel *m_playlistModel{nullptr};
    SongPlayer::AudioImporter *m_audioImporter{nullptr};
    LyricsService *m_lyricsService{nullptr};
    LyricsModel *m_lyricsModel{nullptr};
    PlaylistStorageService *m_playlistStorageService{nullptr};
    QTimer *m_saveTimer{nullptr};

    ICurrentSongManager *m_currentSongManager{nullptr};
    IPlaylistOperations *m_playlistOperations{nullptr};
    IPlaylistPersistence *m_playlistPersistence{nullptr};
    bool m_playlistDirtyDuringImport{false};
};
