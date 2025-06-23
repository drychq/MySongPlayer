// Written by HanQin Chen (cqnuchq@outlook.com) 2025-06-22
#pragma once

#include <QObject>
#include <QStringList>

class PlaylistStorageService;

class IPlaylistPersistence
{
public:
    virtual ~IPlaylistPersistence() = default;
    virtual bool saveCurrentPlaylist(const QString &playlistName = QString()) = 0;
    virtual bool loadPlaylist(const QString &playlistName) = 0;
    virtual QStringList getAllPlaylistNames() const = 0;
    virtual bool deletePlaylist(const QString &playlistName) = 0;
    virtual bool renamePlaylist(const QString &oldName, const QString &newName) = 0;
    virtual QString currentPlaylistName() const = 0;
    virtual PlaylistStorageService* storageService() const = 0;
};

Q_DECLARE_INTERFACE(IPlaylistPersistence, "com.songplayer.IPlaylistPersistence")

