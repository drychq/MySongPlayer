// Written by HanQin Chen (cqnuchq@outlook.com) 2025-06-22
#pragma once

#include <QObject>
#include <QUrl>
#include <QList>

class PlaylistModel;

class IPlaylistOperations
{
public:
    virtual ~IPlaylistOperations() = default;
    virtual void addAudio(const QString& title,
                          const QString& authorName,
                          const QUrl& audioSource,
                          const QUrl& imageSource,
                          const QUrl& videoSource = QUrl()) = 0;
    virtual void removeAudio(int index) = 0;
    virtual void clearPlaylist() = 0;
    virtual PlaylistModel* playlistModel() const = 0;
    virtual QList<QObject*> getPlaylistAudioInfoList() const = 0;
};

Q_DECLARE_INTERFACE(IPlaylistOperations, "com.songplayer.IPlaylistOperations")
