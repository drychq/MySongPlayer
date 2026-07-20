#pragma once

#include "core/AudioTrack.h"
#include "core/PlayMode.h"

#include <QDateTime>
#include <QObject>
#include <QSqlQuery>
#include <QString>
#include <QStringList>

#include <cstddef>
#include <memory>
#include <optional>
#include <span>
#include <vector>

namespace SongPlayer {
class PlaylistDatabase;
}

/**
 * @brief Playlist storage structure
 */
struct PlaylistInfo {
    int id{-1};
    QString name{};
    QDateTime createdAt{};
    QDateTime updatedAt{};
    SongPlayer::Core::PlayMode playMode{SongPlayer::Core::PlayMode::Loop};
    std::optional<std::size_t> currentIndex{};
    std::vector<SongPlayer::Core::AudioTrack> audioItems{};
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

public:
    explicit PlaylistStorageService(QObject *parent = nullptr);
    ~PlaylistStorageService() override;

    bool initialize();
    void shutdown();
    bool isInitialized() const;

    bool savePlaylist(
        const QString& playlistName,
        std::span<const SongPlayer::Core::AudioTrack> audioItems,
        SongPlayer::Core::PlayMode playMode = SongPlayer::Core::PlayMode::Loop,
        std::optional<std::size_t> currentIndex = std::nullopt);

    PlaylistInfo loadPlaylist(const QString& playlistName);
    QStringList getAllPlaylistNames();
    bool deletePlaylist(const QString& playlistName);
    bool renamePlaylist(const QString& oldName, const QString& newName);

    QString lastError() const;

signals:
    void playlistSaved(const QString &playlistName);
    void playlistDeleted(const QString &playlistName);
    void playlistRenamed(const QString &oldName, const QString &newName);
    void errorOccurred(const QString &error);
    void initializationChanged(bool initialized);

private slots:
    void onDatabaseError(const QString &error);

private:
    static constexpr const char *ERROR_EMPTY_NAME{"Playlist name cannot be empty"};
    static constexpr const char *ERROR_NAME_TOO_LONG{"Playlist name is too long"};
    static constexpr const char *ERROR_INVALID_CHARS{"Playlist name contains invalid characters"};

    std::unique_ptr<SongPlayer::PlaylistDatabase> m_database{};
    QString m_lastError{};
    bool m_initialized{false};

    int getOrCreateAudioItem(const SongPlayer::Core::AudioTrack& audioInfo);
    bool createDefaultPlaylist();
    void validatePlaylistName(const QString &name);

    PlaylistInfo playlistInfoFromQuery(const QSqlQuery &query);
    std::vector<SongPlayer::Core::AudioTrack> loadAudioItemsForPlaylist(int playlistId);
    bool checkInitialized();

};
