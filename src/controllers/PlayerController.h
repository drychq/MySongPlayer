// Written by HanQin Chen (cqnuchq@outlook.com) 2025-06-23
#pragma once

#include <QObject>
#include <QtQml/qqmlregistration.h>
#include <QtGui/QGuiApplication>
#include <QtQml/QQmlEngine>
#include <QtQml/QJSEngine>
#include "models/PlaylistModel.h"
#include "interfaces/ICurrentSongManager.h"
#include "interfaces/IPlaylistOperations.h"
#include "interfaces/IPlaylistPersistence.h"
#include "services/PlaylistStorageService.h"

enum class PlayMode;

class AudioPlayer;
class AudioImporter;

class PlayerController : public QObject
{
    Q_OBJECT
    QML_SINGLETON
    QML_NAMED_ELEMENT(PlayerController)

    Q_PROPERTY(bool playing READ playing NOTIFY playingChanged)
    Q_PROPERTY(qint64 duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(qint64 position READ position NOTIFY positionChanged)
    Q_PROPERTY(float volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(bool muted READ isMuted WRITE setMuted NOTIFY mutedChanged)

    Q_PROPERTY(AudioInfo* currentSong READ currentSong WRITE setCurrentSong NOTIFY currentSongChanged)
    Q_PROPERTY(int playMode READ playModeInt WRITE setPlayModeInt NOTIFY playModeChanged)

public:
    static QObject* provider(QQmlEngine *engine, QJSEngine *scriptEngine) {
        Q_UNUSED(engine); Q_UNUSED(scriptEngine);
        auto controller = new PlayerController(QGuiApplication::instance());
        return controller;
    }

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

    Q_INVOKABLE void playPause();
    Q_INVOKABLE void setPosition(qint64 newPosition);
    Q_INVOKABLE void switchToNextSong();
    Q_INVOKABLE void switchToPreviousSong();
    Q_INVOKABLE void switchToAudioByIndex(int index);

    Q_INVOKABLE void addAudio(const QString& title,
                              const QString& authorName,
                              const QUrl& audioSource,
                              const QUrl& imageSource,
                              const QUrl& videoSource = QUrl());
    Q_INVOKABLE void removeAudio(int index);
    Q_INVOKABLE void clearPlaylist();

    Q_INVOKABLE void importLocalAudio(const QList<QUrl>& fileUrls);

    Q_INVOKABLE void addNetworkAudio(const QString& title,
                                     const QString& authorName,
                                     const QUrl& audioSource,
                                     const QUrl& imageSource);

    Q_INVOKABLE PlaylistModel* playlistModel() const;
    Q_INVOKABLE QList<QObject*> getPlaylistAudioInfoList() const;

    Q_INVOKABLE bool saveCurrentPlaylist(const QString &playlistName = QString());
    Q_INVOKABLE bool loadPlaylist(const QString &playlistName);
    Q_INVOKABLE QStringList getAllPlaylistNames() const;
    Q_INVOKABLE bool deletePlaylist(const QString &playlistName);
    Q_INVOKABLE bool renamePlaylist(const QString &oldName, const QString &newName);
    Q_INVOKABLE QString currentPlaylistName() const;

signals:
    void playingChanged();
    void currentSongChanged();
    void positionChanged();
    void durationChanged();
    void volumeChanged();
    void mutedChanged();
    void duplicateAudioSkipped(const QString& title, const QString& reason);
    void playModeChanged();

private slots:
    void onAudioSourceChangeRequested(const QUrl &source);
    void onAudioImported(const QString& title,
                         const QString& authorName,
                         const QUrl& audioSource,
                         const QUrl& imageSource);
    void onCurrentSongChanged();
    void onPositionChanged();

    void onPlaylistChanged();
    void loadDefaultPlaylistOnStartup();

private:
    AudioPlayer *m_audioPlayer;
    PlaylistModel *m_playlistModel;
    AudioImporter *m_audioImporter;

    ICurrentSongManager *m_currentSongManager;
    IPlaylistOperations *m_playlistOperations;
    IPlaylistPersistence *m_playlistPersistence;

    PlaylistStorageService *m_playlistStorageService;
};
