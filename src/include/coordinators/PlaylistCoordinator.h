#pragma once

#include <QObject>

#include "interfaces/ICurrentSongManager.h"
#include "interfaces/IPlaylistOperations.h"
#include "interfaces/IPlaylistPersistence.h"

class PlaylistModel;
class AudioInfo;
class PlaylistStorageService;

class PlaylistCoordinator : public QObject, public ICurrentSongManager, public IPlaylistOperations, public IPlaylistPersistence
{
    Q_OBJECT

public:
    explicit PlaylistCoordinator(PlaylistModel *playlistModel, PlaylistStorageService *storageService, QObject *parent = nullptr);
    ~PlaylistCoordinator() override = default;

    AudioInfo* currentSong() const override;
    void setCurrentSong(AudioInfo *newCurrentSong) override;

    void switchToNextSong() override;
    void switchToPreviousSong() override;
    void switchToAudioByIndex(int index) override;
    void handlePlayFinished() override;

    bool addAudio(const QString& title,
                  const QString& authorName,
                  const QUrl& audioSource,
                  const QUrl& imageSource,
                  const QUrl& videoSource = QUrl()) override;
    void removeAudio(int index) override;
    void clearPlaylist() override;

    PlaylistModel* playlistModel() const override;
    QList<QObject*> getPlaylistAudioInfoList() const override;

    bool saveCurrentPlaylist(const QString &playlistName = QString()) override;
    bool loadPlaylist(const QString &playlistName) override;
    QStringList getAllPlaylistNames() const override;
    bool deletePlaylist(const QString &playlistName) override;
    bool renamePlaylist(const QString &oldName, const QString &newName) override;
    QString currentPlaylistName() const override;

    PlaylistStorageService* storageService() const override;

signals:
    void currentSongChanged();
    void requestAudioSourceChange(const QUrl &source);

    void playlistSaved(const QString &playlistName);
    void playlistLoaded(const QString &playlistName);
    void playlistDeleted(const QString &playlistName);
    void playlistRenamed(const QString &oldName, const QString &newName);
    void currentPlaylistChanged(const QString &playlistName);

private slots:
    void onCurrentSongChanged();
    void onPlayFinished();

private:
    PlaylistModel *m_playlistModel{nullptr};
    PlaylistStorageService *m_storageService{nullptr};
    QString m_currentPlaylistName{};
    bool m_loadingPlaylist{false};
};
