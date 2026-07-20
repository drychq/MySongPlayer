#include "services/PlaylistStorageService.h"

#include "adapters/QtAudioTrackAdapter.h"
#include "core/Playlist.h"
#include "storage/PlaylistDatabase.h"

#include <QDebug>
#include <QSqlQuery>
#include <QVariantList>

#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using std::exception;
using std::invalid_argument;
using std::make_unique;
using std::nullopt;
using std::numeric_limits;
using std::optional;
using std::size_t;
using std::span;
using std::u16string;
using std::vector;

namespace {

const QString& defaultPlaylistName()
{
    static const QString name{SongPlayer::QtAdapter::fromUtf8String(
        SongPlayer::Core::kDefaultPlaylistName)};
    return name;
}

QUrl normalizeImagePath(const QString& originalPath)
{
    if (originalPath.isEmpty() || originalPath.contains(QStringLiteral("app_icon.png"))) {
        return QUrl{QStringLiteral("qrc:/qt/qml/MySongPlayer/assets/icons/app_icon.png")};
    }

    return QUrl{originalPath};
}

u16string toU16String(const QString& value)
{
    u16string result;
    result.reserve(static_cast<size_t>(value.size()));

    for (const QChar character : value) {
        result.push_back(static_cast<char16_t>(character.unicode()));
    }

    return result;
}

int databaseIndex(optional<size_t> currentIndex)
{
    return currentIndex ? static_cast<int>(*currentIndex) : -1;
}

optional<size_t> modelIndex(int currentIndex)
{
    return currentIndex >= 0 ? optional<size_t>{static_cast<size_t>(currentIndex)} : nullopt;
}

SongPlayer::Core::PlayMode storedPlayMode(int value)
{
    using SongPlayer::Core::PlayMode;

    switch (static_cast<PlayMode>(value)) {
    case PlayMode::Loop:
    case PlayMode::Shuffle:
    case PlayMode::RepeatOne:
        return static_cast<PlayMode>(value);
    }

    return PlayMode::Loop;
}

} // namespace

PlaylistStorageService::PlaylistStorageService(QObject* parent)
    : QObject{parent}
    , m_database{make_unique<SongPlayer::PlaylistDatabase>(this)}
{
    connect(m_database.get(), &SongPlayer::PlaylistDatabase::databaseError,
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
        m_lastError = QStringLiteral("Database initialization failed: %1")
                          .arg(m_database->lastError());
        emit errorOccurred(m_lastError);
        return false;
    }

    if (!createDefaultPlaylist()) {
        m_lastError = QStringLiteral("Failed to create default playlist: %1")
                          .arg(m_database->lastError());
        emit errorOccurred(m_lastError);
        return false;
    }

    m_initialized = true;
    emit initializationChanged(true);
    return true;
}

void PlaylistStorageService::shutdown()
{
    if (!m_initialized) {
        return;
    }

    m_database->closeDatabase();
    m_initialized = false;
    emit initializationChanged(false);
}

bool PlaylistStorageService::isInitialized() const
{
    return m_initialized;
}

bool PlaylistStorageService::savePlaylist(
    const QString& playlistName,
    span<const SongPlayer::Core::AudioTrack> audioItems,
    SongPlayer::Core::PlayMode playMode,
    optional<size_t> currentIndex)
{
    if (!checkInitialized()) {
        return false;
    }

    if (audioItems.size() > static_cast<size_t>(numeric_limits<int>::max()) ||
        (currentIndex && *currentIndex >= audioItems.size())) {
        m_lastError = QStringLiteral("Playlist index or size exceeds the supported range");
        emit errorOccurred(m_lastError);
        return false;
    }

    try {
        validatePlaylistName(playlistName);

        const bool saved{m_database->runInTransaction([&]() {
            int playlistId{-1};
            QSqlQuery query{m_database->executeQuery(
                QStringLiteral("SELECT id FROM playlists WHERE name = ?"),
                QVariantList{playlistName})};

            if (query.next()) {
                playlistId = query.value(0).toInt();
                if (!m_database->executeNonQuery(
                        QStringLiteral("DELETE FROM playlist_items WHERE playlist_id = ?"),
                        QVariantList{playlistId})) {
                    m_lastError = QStringLiteral("Failed to delete existing playlist items");
                    return false;
                }

                if (!m_database->executeNonQuery(
                        QStringLiteral("UPDATE playlists SET play_mode = ?, current_index = ?, "
                                       "updated_at = CURRENT_TIMESTAMP WHERE id = ?"),
                        QVariantList{static_cast<int>(playMode), databaseIndex(currentIndex), playlistId})) {
                    m_lastError = QStringLiteral("Failed to update playlist information");
                    return false;
                }
            } else {
                if (!m_database->executeNonQuery(
                        QStringLiteral("INSERT INTO playlists (name, play_mode, current_index) "
                                       "VALUES (?, ?, ?)"),
                        QVariantList{playlistName, static_cast<int>(playMode), databaseIndex(currentIndex)})) {
                    m_lastError = QStringLiteral("Failed to create playlist");
                    return false;
                }

                query = m_database->executeQuery(QStringLiteral("SELECT last_insert_rowid()"));
                if (!query.next()) {
                    m_lastError = QStringLiteral("Failed to get playlist ID");
                    return false;
                }
                playlistId = query.value(0).toInt();
            }

            for (size_t position{0}; position < audioItems.size(); ++position) {
                const SongPlayer::Core::AudioTrack& audioInfo{audioItems[position]};
                const int audioItemId{getOrCreateAudioItem(audioInfo)};
                if (audioItemId < 0) {
                    m_lastError = QStringLiteral("Failed to process audio item: %1")
                                      .arg(SongPlayer::QtAdapter::fromUtf8String(audioInfo.title));
                    return false;
                }

                if (!m_database->executeNonQuery(
                        QStringLiteral("INSERT INTO playlist_items "
                                       "(playlist_id, audio_item_id, position) VALUES (?, ?, ?)"),
                        QVariantList{playlistId, audioItemId, static_cast<int>(position)})) {
                    m_lastError = QStringLiteral("Failed to add audio item to playlist: %1")
                                      .arg(SongPlayer::QtAdapter::fromUtf8String(audioInfo.title));
                    return false;
                }
            }

            return true;
        })};

        if (!saved) {
            return false;
        }

        emit playlistSaved(playlistName);
        qDebug() << "Playlist saved successfully:" << playlistName << "with"
                 << static_cast<qulonglong>(audioItems.size()) << "items";
        return true;
    } catch (const exception& error) {
        m_lastError = QStringLiteral("Exception occurred while saving playlist: %1")
                          .arg(error.what());
        emit errorOccurred(m_lastError);
        return false;
    }
}

PlaylistInfo PlaylistStorageService::loadPlaylist(const QString& playlistName)
{
    PlaylistInfo result;
    if (!checkInitialized()) {
        return result;
    }

    try {
        QSqlQuery query{m_database->executeQuery(
            QStringLiteral("SELECT id, name, created_at, updated_at, play_mode, current_index "
                           "FROM playlists WHERE name = ?"),
            QVariantList{playlistName})};
        if (!query.next()) {
            m_lastError = QStringLiteral("Playlist does not exist: %1").arg(playlistName);
            return result;
        }

        result = playlistInfoFromQuery(query);
        result.audioItems = loadAudioItemsForPlaylist(result.id);
        if (result.currentIndex && *result.currentIndex >= result.audioItems.size()) {
            result.currentIndex.reset();
        }

        qDebug() << "Playlist loaded successfully:" << playlistName << "with"
                 << static_cast<qulonglong>(result.audioItems.size()) << "items";
        return result;
    } catch (const exception& error) {
        m_lastError = QStringLiteral("Exception occurred while loading playlist: %1")
                          .arg(error.what());
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

    QSqlQuery query{m_database->executeQuery(
        QStringLiteral("SELECT name FROM playlists ORDER BY name"))};
    while (query.next()) {
        result.append(query.value(0).toString());
    }
    return result;
}

bool PlaylistStorageService::deletePlaylist(const QString& playlistName)
{
    if (!checkInitialized()) {
        return false;
    }

    if (playlistName == defaultPlaylistName()) {
        m_lastError = QStringLiteral("Cannot delete default playlist");
        emit errorOccurred(m_lastError);
        return false;
    }

    QSqlQuery existing{m_database->executeQuery(
        QStringLiteral("SELECT 1 FROM playlists WHERE name = ?"), QVariantList{playlistName})};
    if (!existing.next()) {
        m_lastError = QStringLiteral("Playlist does not exist: %1").arg(playlistName);
        return false;
    }

    if (!m_database->executeNonQuery(
            QStringLiteral("DELETE FROM playlists WHERE name = ?"), QVariantList{playlistName})) {
        m_lastError = QStringLiteral("Failed to delete playlist");
        return false;
    }

    emit playlistDeleted(playlistName);
    return true;
}

bool PlaylistStorageService::renamePlaylist(const QString& oldName, const QString& newName)
{
    if (!checkInitialized()) {
        return false;
    }

    if (oldName == defaultPlaylistName() || newName == defaultPlaylistName()) {
        m_lastError = QStringLiteral("The default playlist name is reserved");
        emit errorOccurred(m_lastError);
        return false;
    }

    try {
        validatePlaylistName(newName);
        QSqlQuery existing{m_database->executeQuery(
            QStringLiteral("SELECT 1 FROM playlists WHERE name = ?"), QVariantList{oldName})};
        if (!existing.next()) {
            m_lastError = QStringLiteral("Playlist does not exist: %1").arg(oldName);
            return false;
        }

        if (oldName == newName) {
            return true;
        }

        if (!m_database->executeNonQuery(
                QStringLiteral("UPDATE playlists SET name = ?, updated_at = CURRENT_TIMESTAMP "
                               "WHERE name = ?"),
                QVariantList{newName, oldName})) {
            m_lastError = QStringLiteral("Failed to rename playlist");
            return false;
        }

        emit playlistRenamed(oldName, newName);
        return true;
    } catch (const exception& error) {
        m_lastError = QStringLiteral("Exception occurred while renaming playlist: %1")
                          .arg(error.what());
        emit errorOccurred(m_lastError);
        return false;
    }
}

QString PlaylistStorageService::lastError() const
{
    return m_lastError;
}

void PlaylistStorageService::onDatabaseError(const QString& error)
{
    m_lastError = error;
    emit errorOccurred(error);
}

int PlaylistStorageService::getOrCreateAudioItem(const SongPlayer::Core::AudioTrack& audioInfo)
{
    const QString source{SongPlayer::QtAdapter::fromUtf8String(audioInfo.audioSource)};
    if (source.isEmpty()) {
        return -1;
    }

    QSqlQuery query{m_database->executeQuery(
        QStringLiteral("SELECT id FROM audio_items WHERE audio_source = ?"), QVariantList{source})};
    if (query.next()) {
        return query.value(0).toInt();
    }

    if (!m_database->executeNonQuery(
            QStringLiteral("INSERT INTO audio_items "
                           "(title, author_name, audio_source, image_source, video_source) "
                           "VALUES (?, ?, ?, ?, ?)"),
            QVariantList{
                SongPlayer::QtAdapter::fromUtf8String(audioInfo.title),
                SongPlayer::QtAdapter::fromUtf8String(audioInfo.authorName),
                source,
                SongPlayer::QtAdapter::fromUtf8String(audioInfo.imageSource),
                SongPlayer::QtAdapter::fromUtf8String(audioInfo.videoSource),
            })) {
        return -1;
    }

    query = m_database->executeQuery(QStringLiteral("SELECT last_insert_rowid()"));
    return query.next() ? query.value(0).toInt() : -1;
}

bool PlaylistStorageService::createDefaultPlaylist()
{
    QSqlQuery query{m_database->executeQuery(
        QStringLiteral("SELECT COUNT(*) FROM playlists WHERE name = ?"),
        QVariantList{defaultPlaylistName()})};
    if (query.next() && query.value(0).toInt() > 0) {
        return true;
    }

    return m_database->executeNonQuery(
        QStringLiteral("INSERT INTO playlists (name, play_mode, current_index) VALUES (?, ?, ?)"),
        QVariantList{defaultPlaylistName(),
                     static_cast<int>(SongPlayer::Core::PlayMode::Loop),
                     -1});
}

void PlaylistStorageService::validatePlaylistName(const QString& name)
{
    const u16string nameText{toU16String(name)};
    switch (SongPlayer::Core::validatePlaylistName(nameText)) {
    case SongPlayer::Core::PlaylistNameValidationError::None:
        return;
    case SongPlayer::Core::PlaylistNameValidationError::Empty:
        throw invalid_argument{ERROR_EMPTY_NAME};
    case SongPlayer::Core::PlaylistNameValidationError::TooLong:
        throw invalid_argument{ERROR_NAME_TOO_LONG};
    case SongPlayer::Core::PlaylistNameValidationError::InvalidCharacter:
        throw invalid_argument{ERROR_INVALID_CHARS};
    }
}

PlaylistInfo PlaylistStorageService::playlistInfoFromQuery(const QSqlQuery& query)
{
    PlaylistInfo info;
    info.id = query.value(QStringLiteral("id")).toInt();
    info.name = query.value(QStringLiteral("name")).toString();
    info.createdAt = query.value(QStringLiteral("created_at")).toDateTime();
    info.updatedAt = query.value(QStringLiteral("updated_at")).toDateTime();
    info.playMode = storedPlayMode(query.value(QStringLiteral("play_mode")).toInt());
    info.currentIndex = modelIndex(query.value(QStringLiteral("current_index")).toInt());
    return info;
}

vector<SongPlayer::Core::AudioTrack> PlaylistStorageService::loadAudioItemsForPlaylist(int playlistId)
{
    vector<SongPlayer::Core::AudioTrack> result;
    QSqlQuery query{m_database->executeQuery(
        QStringLiteral(
            "SELECT a.title, a.author_name, a.audio_source, a.image_source, a.video_source, p.position "
            "FROM audio_items a JOIN playlist_items p ON a.id = p.audio_item_id "
            "WHERE p.playlist_id = ? ORDER BY p.position"),
        QVariantList{playlistId})};

    while (query.next()) {
        SongPlayer::Core::AudioTrack track{SongPlayer::QtAdapter::makeCoreTrack(
            query.value(QStringLiteral("title")).toString(),
            query.value(QStringLiteral("author_name")).toString(),
            QUrl{query.value(QStringLiteral("audio_source")).toString()},
            normalizeImagePath(query.value(QStringLiteral("image_source")).toString()),
            QUrl{query.value(QStringLiteral("video_source")).toString()})};
        track.songIndex = query.value(QStringLiteral("position")).toInt();
        result.push_back(std::move(track));
    }

    return result;
}

bool PlaylistStorageService::checkInitialized()
{
    if (m_initialized) {
        return true;
    }

    m_lastError = QStringLiteral("Storage service not initialized");
    emit errorOccurred(m_lastError);
    return false;
}
