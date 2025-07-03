// Written by HanQin Chen (cqnuchq@outlook.com) 2025-07-04
#include "services/PlaylistStorageService.h"
#include "storage/PlaylistDatabase.h"
#include "models/AudioInfo.h"
#include "models/PlaylistModel.h"
#include <QDateTime>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QVariantMap>
#include <QVariantList>
#include <memory>
#include <QDebug>

namespace {
    QUrl normalizeImagePath(const QString& originalPath) {
        if (originalPath.isEmpty()) {
            return QUrl("qrc:/qt/qml/MySongPlayer/assets/icons/app_icon.png");
        }
        
        if (originalPath.startsWith("qrc:/qt/qml/MySongPlayer/")) {
            return QUrl(originalPath);
        }
        
        if (originalPath.contains("app_icon.png")) {
            return QUrl("qrc:/qt/qml/MySongPlayer/assets/icons/app_icon.png");
        }
        
        return QUrl(originalPath);
    }
}

PlaylistStorageService::PlaylistStorageService(QObject *parent)
    : QObject(parent)
    , m_database(std::make_unique<PlaylistDatabase>(this))
    , m_initialized(false)
{
    connect(m_database.get(), &PlaylistDatabase::databaseError,
            this, &PlaylistStorageService::onDatabaseError);
}

PlaylistStorageService::~PlaylistStorageService()
{
    shutdown();
}

bool PlaylistStorageService::initialize()
{
    if (m_initialized) {
        return true;
    }

    if (!m_database->initializeDatabase()) {
        m_lastError = QString("Database initialization failed: %1").arg(m_database->lastError());
        emit errorOccurred(m_lastError);
        return false;
    }

    if (!createDefaultPlaylist()) {
        m_lastError = QString("Failed to create default playlist: %1").arg(m_database->lastError());
        emit errorOccurred(m_lastError);
        return false;
    }



    m_initialized = true;
    emit initializationChanged(true);

    return true;
}

void PlaylistStorageService::shutdown()
{
    if (m_initialized) {
        m_database->closeDatabase();
        m_initialized = false;
        emit initializationChanged(false);

    }
}

bool PlaylistStorageService::isInitialized() const
{
    return m_initialized;
}

bool PlaylistStorageService::savePlaylist(const QString &playlistName,
                                          const QList<AudioInfo*> &audioItems,
                                          PlayMode playMode,
                                          int currentIndex)
{
    if (!checkInitialized()) {
        return false;
    }

    try {
        validatePlaylistName(playlistName);

        if (!m_database->beginTransaction()) {
            m_lastError = "Failed to begin transaction";
            return false;
        }

        int playlistId = -1;
        QSqlQuery query = m_database->executeQuery(
            "SELECT id FROM playlists WHERE name = ?",
            QVariantList() << playlistName
            );

        if (query.next()) {
            playlistId = query.value(0).toInt();

            if (!m_database->executeNonQuery(
                    "DELETE FROM playlist_items WHERE playlist_id = ?",
                    QVariantList() << playlistId)) {
                m_database->rollbackTransaction();
                m_lastError = "Failed to delete existing playlist items";
                return false;
            }

            if (!m_database->executeNonQuery(
                    "UPDATE playlists SET play_mode = ?, current_index = ?, updated_at = CURRENT_TIMESTAMP WHERE id = ?",
                    QVariantList() << static_cast<int>(playMode) << currentIndex << playlistId)) {
                m_database->rollbackTransaction();
                m_lastError = "Failed to update playlist information";
                return false;
            }
        } else {
            if (!m_database->executeNonQuery(
                    "INSERT INTO playlists (name, play_mode, current_index) VALUES (?, ?, ?)",
                    QVariantList() << playlistName << static_cast<int>(playMode) << currentIndex)) {
                m_database->rollbackTransaction();
                m_lastError = "Failed to create playlist";
                return false;
            }

            query = m_database->executeQuery("SELECT last_insert_rowid()");
            if (query.next()) {
                playlistId = query.value(0).toInt();
            } else {
                m_database->rollbackTransaction();
                m_lastError = "Failed to get playlist ID";
                return false;
            }
        }

        for (int i = 0; i < audioItems.size(); ++i) {
            AudioInfo *audioInfo = audioItems[i];
            if (!audioInfo) continue;

            int audioItemId = getOrCreateAudioItem(audioInfo);
            if (audioItemId == -1) {
                m_database->rollbackTransaction();
                m_lastError = QString("Failed to process audio item: %1").arg(audioInfo->title());
                return false;
            }

            if (!m_database->executeNonQuery(
                    "INSERT INTO playlist_items (playlist_id, audio_item_id, position) VALUES (?, ?, ?)",
                    QVariantList() << playlistId << audioItemId << i)) {
                m_database->rollbackTransaction();
                m_lastError = QString("Failed to add audio item to playlist: %1").arg(audioInfo->title());
                return false;
            }
        }

        if (!m_database->commitTransaction()) {
            m_lastError = "Failed to commit transaction";
            return false;
        }

        emit playlistSaved(playlistName);
        qDebug() << "Playlist saved successfully:" << playlistName << "with" << audioItems.size() << "items";
        return true;

    } catch (const std::exception &e) {
        m_database->rollbackTransaction();
        m_lastError = QString("Exception occurred while saving playlist: %1").arg(e.what());
        emit errorOccurred(m_lastError);
        return false;
    }
}

PlaylistInfo PlaylistStorageService::loadPlaylist(const QString &playlistName)
{
    PlaylistInfo result;

    if (!checkInitialized()) {
        return result;
    }

    try {
        QSqlQuery query = m_database->executeQuery(
            "SELECT id, name, created_at, updated_at, play_mode, current_index FROM playlists WHERE name = ?",
            QVariantList() << playlistName
            );

        if (!query.next()) {
            m_lastError = QString("Playlist does not exist: %1").arg(playlistName);
            return result;
        }

        result = playlistInfoFromQuery(query);
        result.audioItems = loadAudioItemsForPlaylist(result.id);

        qDebug() << "Playlist loaded successfully:" << playlistName << "with" << result.audioItems.size() << "items";
        return result;

    } catch (const std::exception &e) {
        m_lastError = QString("Exception occurred while loading playlist: %1").arg(e.what());
        emit errorOccurred(m_lastError);
        return result;
    }
}

QStringList PlaylistStorageService::getAllPlaylistNames()
{
    QStringList result;

    if (!checkInitialized()) {
        return result;
    }

    QSqlQuery query = m_database->executeQuery("SELECT name FROM playlists ORDER BY name");

    while (query.next()) {
        result.append(query.value(0).toString());
    }

    return result;
}

bool PlaylistStorageService::deletePlaylist(const QString &playlistName)
{
    if (!checkInitialized()) {
        return false;
    }

    if (playlistName == DEFAULT_PLAYLIST_NAME) {
        m_lastError = "Cannot delete default playlist";
        emit errorOccurred(m_lastError);
        return false;
    }

    if (!m_database->executeNonQuery(
            "DELETE FROM playlists WHERE name = ?",
            QVariantList() << playlistName)) {
        m_lastError = "Failed to delete playlist";
        return false;
    }

    emit playlistDeleted(playlistName);
    qDebug() << "Playlist deleted successfully:" << playlistName;
    return true;
}

bool PlaylistStorageService::renamePlaylist(const QString &oldName, const QString &newName)
{
    if (!checkInitialized()) {
        return false;
    }

    try {
        validatePlaylistName(newName);

        if (!m_database->executeNonQuery(
                "UPDATE playlists SET name = ?, updated_at = CURRENT_TIMESTAMP WHERE name = ?",
                QVariantList() << newName << oldName)) {
            m_lastError = "Failed to rename playlist";
            return false;
        }

        emit playlistRenamed(oldName, newName);
        qDebug() << "Playlist renamed successfully:" << oldName << "->" << newName;
        return true;

    } catch (const std::exception &e) {
        m_lastError = QString("Exception occurred while renaming playlist: %1").arg(e.what());
        emit errorOccurred(m_lastError);
        return false;
    }
}

QString PlaylistStorageService::lastError() const
{
    return m_lastError;
}

void PlaylistStorageService::onDatabaseError(const QString &error)
{
    m_lastError = error;
    emit errorOccurred(error);
}

int PlaylistStorageService::getOrCreateAudioItem(AudioInfo *audioInfo)
{
    if (!audioInfo) return -1;

    QSqlQuery query = m_database->executeQuery(
        "SELECT id FROM audio_items WHERE audio_source = ?",
        QVariantList() << audioInfo->audioSource().toString()
        );

    if (query.next()) {
        return query.value(0).toInt();
    }

    if (!m_database->executeNonQuery(
            "INSERT INTO audio_items (title, author_name, audio_source, image_source, video_source) VALUES (?, ?, ?, ?, ?)",
            QVariantList() << audioInfo->title() << audioInfo->authorName()
                           << audioInfo->audioSource().toString()
                           << audioInfo->imageSource().toString()
                           << audioInfo->videoSource().toString())) {
        return -1;
    }

    query = m_database->executeQuery("SELECT last_insert_rowid()");
    if (query.next()) {
        return query.value(0).toInt();
    }

    return -1;
}

bool PlaylistStorageService::createDefaultPlaylist()
{
    QSqlQuery query = m_database->executeQuery(
        "SELECT COUNT(*) FROM playlists WHERE name = ?",
        QVariantList() << DEFAULT_PLAYLIST_NAME
        );

    if (query.next() && query.value(0).toInt() > 0) {
        return true;
    }

    return m_database->executeNonQuery(
        "INSERT INTO playlists (name, play_mode, current_index) VALUES (?, ?, ?)",
        QVariantList() << DEFAULT_PLAYLIST_NAME << static_cast<int>(PlayMode::Loop) << -1
        );
}

void PlaylistStorageService::validatePlaylistName(const QString &name)
{
    if (name.isEmpty()) {
        throw std::invalid_argument(ERROR_EMPTY_NAME);
    }

    if (name.length() > 100) {
        throw std::invalid_argument(ERROR_NAME_TOO_LONG);
    }

    static const QRegularExpression invalidCharsRegex("[<>:\"/\\\\|?*]");
    if (name.contains(invalidCharsRegex)) {
        throw std::invalid_argument(ERROR_INVALID_CHARS);
    }
}

PlaylistInfo PlaylistStorageService::playlistInfoFromQuery(const QSqlQuery &query)
{
    PlaylistInfo info;
    info.id = query.value("id").toInt();
    info.name = query.value("name").toString();
    info.createdAt = query.value("created_at").toDateTime();
    info.updatedAt = query.value("updated_at").toDateTime();
    info.playMode = static_cast<PlayMode>(query.value("play_mode").toInt());
    info.currentIndex = query.value("current_index").toInt();
    return info;
}

QList<AudioInfo*> PlaylistStorageService::loadAudioItemsForPlaylist(int playlistId)
{
    QList<AudioInfo*> result;

    QSqlQuery query = m_database->executeQuery(
        R"(SELECT a.title, a.author_name, a.audio_source, a.image_source, a.video_source, p.position
           FROM audio_items a
           JOIN playlist_items p ON a.id = p.audio_item_id
           WHERE p.playlist_id = ?
           ORDER BY p.position)",
        QVariantList() << playlistId
        );

    while (query.next()) {
        std::unique_ptr<AudioInfo> audioInfo = std::make_unique<AudioInfo>(this);
        audioInfo->setTitle(query.value("title").toString());
        audioInfo->setAuthorName(query.value("author_name").toString());
        audioInfo->setAudioSource(QUrl(query.value("audio_source").toString()));
        audioInfo->setImageSource(normalizeImagePath(query.value("image_source").toString()));
        audioInfo->setVideoSource(QUrl(query.value("video_source").toString()));
        audioInfo->setSongIndex(query.value("position").toInt());

        result.append(audioInfo.release());
    }

    return result;
}


bool PlaylistStorageService::addAudioToPlaylist(const QString &playlistName, AudioInfo *audioInfo)
{
    if (!checkInitialized() || !audioInfo) {
        return false;
    }

    PlaylistInfo playlist = loadPlaylist(playlistName);
    if (playlist.id == -1) {
        return false;
    }

    playlist.audioItems.append(audioInfo);
    return savePlaylist(playlistName, playlist.audioItems, playlist.playMode, playlist.currentIndex);
}

bool PlaylistStorageService::removeAudioFromPlaylist(const QString &playlistName, int position)
{
    if (!checkInitialized()) {
        return false;
    }

    PlaylistInfo playlist = loadPlaylist(playlistName);
    if (playlist.id == -1 || position < 0 || position >= playlist.audioItems.size()) {
        return false;
    }

    playlist.audioItems.removeAt(position);
    if (playlist.currentIndex >= position && playlist.currentIndex > 0) {
        playlist.currentIndex--;
    }

    return savePlaylist(playlistName, playlist.audioItems, playlist.playMode, playlist.currentIndex);
}

bool PlaylistStorageService::moveAudioInPlaylist(const QString &playlistName, int fromPosition, int toPosition)
{
    if (!checkInitialized()) {
        return false;
    }

    PlaylistInfo playlist = loadPlaylist(playlistName);
    if (playlist.id == -1 || fromPosition < 0 || fromPosition >= playlist.audioItems.size() ||
        toPosition < 0 || toPosition >= playlist.audioItems.size()) {
        return false;
    }

    playlist.audioItems.move(fromPosition, toPosition);
    return savePlaylist(playlistName, playlist.audioItems, playlist.playMode, playlist.currentIndex);
}

bool PlaylistStorageService::updatePlaylistState(const QString &playlistName, PlayMode playMode, int currentIndex)
{
    if (!checkInitialized()) {
        return false;
    }

    return m_database->executeNonQuery(
        "UPDATE playlists SET play_mode = ?, current_index = ?, updated_at = CURRENT_TIMESTAMP WHERE name = ?",
        QVariantList() << static_cast<int>(playMode) << currentIndex << playlistName
        );
}

QList<AudioInfo*> PlaylistStorageService::findDuplicateAudioItems()
{
    return QList<AudioInfo*>();
}

bool PlaylistStorageService::cleanupUnusedAudioItems()
{
    if (!checkInitialized()) {
        return false;
    }

    return m_database->executeNonQuery(
        "DELETE FROM audio_items WHERE id NOT IN (SELECT DISTINCT audio_item_id FROM playlist_items)"
        );
}

bool PlaylistStorageService::importPlaylistFromData(const QString &playlistName, const QVariantList &audioData)
{
    Q_UNUSED(playlistName)
    Q_UNUSED(audioData)
    return false;
}

QVariantList PlaylistStorageService::exportPlaylistToData(const QString &playlistName)
{
    Q_UNUSED(playlistName)
    return QVariantList();
}

int PlaylistStorageService::getPlaylistCount()
{
    if (!checkInitialized()) {
        return 0;
    }

    QSqlQuery query = m_database->executeQuery("SELECT COUNT(*) FROM playlists");
    if (query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

int PlaylistStorageService::getAudioItemCount()
{
    if (!checkInitialized()) {
        return 0;
    }

    QSqlQuery query = m_database->executeQuery("SELECT COUNT(*) FROM audio_items");
    if (query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

QVariantMap PlaylistStorageService::getStorageStatistics()
{
    QVariantMap stats;
    stats["playlistCount"] = getPlaylistCount();
    stats["audioItemCount"] = getAudioItemCount();
    stats["initialized"] = m_initialized;
    return stats;
}

bool PlaylistStorageService::checkInitialized()
{
    if (!m_initialized) {
        m_lastError = "Storage service not initialized";
        emit errorOccurred(m_lastError);
        return false;
    }
    return true;
}


