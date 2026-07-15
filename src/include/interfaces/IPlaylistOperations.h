#pragma once

#include <QList>
#include <QString>
#include <QUrl>

class QObject;
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
