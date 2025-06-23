#pragma once

#include <QObject>

#include "interfaces/ICurrentSongManager.h"
#include "interfaces/IPlaylistOperations.h"

class PlaylistModel;
class AudioInfo;

class PlaylistCoordinator : public QObject, public ICurrentSongManager, public IPlaylistOperations
{
    Q_OBJECT
    Q_INTERFACES(ICurrentSongManager IPlaylistOperations)

public:
    explicit PlaylistCoordinator(PlaylistModel *playlistModel, QObject *parent = nullptr);
    virtual ~PlaylistCoordinator() = default;

    AudioInfo* currentSong() const override;
    void setCurrentSong(AudioInfo *newCurrentSong) override;

    void switchToNextSong() override;
    void switchToPreviousSong() override;
    void switchToAudioByIndex(int index) override;
    void handlePlayFinished() override;

    void addAudio(const QString& title,
                  const QString& authorName,
                  const QUrl& audioSource,
                  const QUrl& imageSource,
                  const QUrl& videoSource = QUrl()) override;
    void removeAudio(int index) override;
    void clearPlaylist() override;

    PlaylistModel* playlistModel() const override;
    QList<QObject*> getPlaylistAudioInfoList() const ;

    QString currentPlaylistName() const;

signals:
    void currentSongChanged();
    void requestAudioSourceChange(const QUrl &source);

    void playlistRenamed(const QString &oldName, const QString &newName);
    void currentPlaylistChanged(const QString &playlistName);

private slots:
    void onCurrentSongChanged();
    void onRequestAudioSourceChange(const QUrl &source);
    void onPlayFinished();
    void onRowsInserted(const QModelIndex &parent, int first, int last);

private:
    PlaylistModel *m_playlistModel;
    QString m_currentPlaylistName;
};
