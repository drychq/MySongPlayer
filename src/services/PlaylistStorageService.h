// Written by HanQin Chen (cqnuchq@outlook.com) 2025-06-23
#pragma once

#include <QObject>
#include <QList>
#include <QString>
#include <QUrl>
#include <QVariant>
#include <QDateTime>
#include <QSqlQuery>
#include <QRegularExpression>
#include <QtQml/qqmlregistration.h>

class AudioInfo;
class PlaylistDatabase;

#include "models/PlaylistModel.h"

/**
 * @brief Playlist storage structure
 */
struct PlaylistInfo {
    int id = -1;
    QString name;
    QDateTime createdAt;
    QDateTime updatedAt;
    PlayMode playMode;
    int currentIndex = -1;
    QList<AudioInfo*> audioItems;
};

/**
 * @brief Playlist storage service class
 *
 * Responsible for persistent storage operations of playlists and provides high-level data operation APIs.
 * This class focuses on the following responsibilities:
 * - Saving and loading playlists
 * - Managing multiple playlists
 * - Deduplication and management of audio items
 * - Data conversion and serialization
 * - Error handling and user feedback
 */
class PlaylistStorageService : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("PlaylistStorageService instances are created and managed by C++")

public:
    explicit PlaylistStorageService(QObject *parent = nullptr);
    virtual ~PlaylistStorageService();

    bool initialize();
    void shutdown();
    bool isInitialized() const;

    Q_INVOKABLE bool savePlaylist(const QString &playlistName,
                                  const QList<AudioInfo*> &audioItems,
                                  PlayMode playMode = PlayMode::Loop,
                                  int currentIndex = -1);

    Q_INVOKABLE PlaylistInfo loadPlaylist(const QString &playlistName);
    Q_INVOKABLE QStringList getAllPlaylistNames();
    Q_INVOKABLE bool deletePlaylist(const QString &playlistName);
    Q_INVOKABLE bool renamePlaylist(const QString &oldName, const QString &newName);

    Q_INVOKABLE bool addAudioToPlaylist(const QString &playlistName, AudioInfo *audioInfo);
    Q_INVOKABLE bool removeAudioFromPlaylist(const QString &playlistName, int position);
    Q_INVOKABLE bool moveAudioInPlaylist(const QString &playlistName, int fromPosition, int toPosition);
    Q_INVOKABLE bool updatePlaylistState(const QString &playlistName, PlayMode playMode, int currentIndex);

    Q_INVOKABLE QList<AudioInfo*> findDuplicateAudioItems();
    Q_INVOKABLE bool cleanupUnusedAudioItems();

    Q_INVOKABLE bool importPlaylistFromData(const QString &playlistName, const QVariantList &audioData);
    Q_INVOKABLE QVariantList exportPlaylistToData(const QString &playlistName);

    Q_INVOKABLE int getPlaylistCount();
    Q_INVOKABLE int getAudioItemCount();
    Q_INVOKABLE QVariantMap getStorageStatistics();

    QString lastError() const;

signals:
    void playlistSaved(const QString &playlistName);
    void playlistDeleted(const QString &playlistName);
    void playlistRenamed(const QString &oldName, const QString &newName);
    void audioItemAdded(const QString &playlistName, int position);
    void audioItemRemoved(const QString &playlistName, int position);
    void playlistStateUpdated(const QString &playlistName);
    void errorOccurred(const QString &error);
    void initializationChanged(bool initialized);

private slots:
    void onDatabaseError(const QString &error);

private:
    static constexpr const char* DEFAULT_PLAYLIST_NAME = "Default playlist";
    static constexpr const char* ERROR_EMPTY_NAME = "Playlist name cannot be empty";
    static constexpr const char* ERROR_NAME_TOO_LONG = "Playlist name is too long";
    static constexpr const char* ERROR_INVALID_CHARS = "Playlist name contains invalid characters";

    PlaylistDatabase *m_database;
    QString m_lastError;
    bool m_initialized;

    int getOrCreateAudioItem(AudioInfo *audioInfo);
    AudioInfo* audioInfoFromData(const QVariantMap &data);
    QVariantMap audioInfoToData(AudioInfo *audioInfo);
    bool createDefaultPlaylist();
    void validatePlaylistName(const QString &name);
    bool updatePlaylistTimestamp(const QString &playlistName);

    PlaylistInfo playlistInfoFromQuery(const QSqlQuery &query);
    QList<AudioInfo*> loadAudioItemsForPlaylist(int playlistId);
    bool checkInitialized();
};
