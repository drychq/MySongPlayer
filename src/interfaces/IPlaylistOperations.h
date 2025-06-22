#pragma once

#include <QObject>
#include <QUrl>
#include <QList>

class PlaylistModel;

/**
 * @brief Playlist Operations Interface
 *
 * Focuses on data management functions for the playlist.
 * This interface follows the Interface Segregation Principle and only includes responsibilities related to playlist operations:
 * - Playlist modification operations (add, remove, clear)
 * - Playlist data access
 * - Playlist model retrieval
 */
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
