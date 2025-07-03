// Written by HanQin Chen (cqnuchq@outlook.com) 2025-07-04
#include "storage/PlaylistDatabase.h"

PlaylistDatabase::PlaylistDatabase(QObject *parent)
    : QObject{parent}
    , m_databasePath(getDatabasePath())
{}

PlaylistDatabase::~PlaylistDatabase()
{
    closeDatabase();
}

bool PlaylistDatabase::initializeDatabase()
{
    try {
        // Add a SQLite database connection with a unique name to prevent conflicts
        // if multiple database connections are used within the application.
        m_database = QSqlDatabase::addDatabase("QSQLITE", DATABASE_CONNECTION_NAME);
        // Set the database file path. This determines where the SQLite database file will be stored.
        m_database.setDatabaseName(m_databasePath);

        if (!m_database.open()) {
            // Log and store the error if the database fails to open, providing diagnostic information.
            m_lastError = QString("Unable to open database: %1").arg(m_database.lastError().text());
            qCritical() << m_lastError;
            return false;
        }

        QSqlQuery query(m_database);
        // Enable foreign key constraints. This is crucial for maintaining referential integrity
        // between tables (e.g., ensuring that playlist items always refer to existing playlists or audio items).
        if (!query.exec("PRAGMA foreign_keys = ON")) {
            logError("Enable foreign key constraints", query.lastError());
            return false;
        }

        if (!createTables()) {
            // If table creation fails, set an error and return false, indicating a critical setup issue.
            m_lastError = "Failed to create database tables";
            return false;
        }

        return true;

    } catch (const std::exception &e) {
        // Catch any unexpected exceptions during initialization to prevent crashes
        // and provide a robust error handling mechanism.
        m_lastError = QString("Database initialization exception: %1").arg(e.what());
        qCritical() << m_lastError;
        return false;
    }
}

void PlaylistDatabase::closeDatabase()
{
    if (m_database.isOpen()) {
        m_database.close();
    }

    if (QSqlDatabase::contains(DATABASE_CONNECTION_NAME)) {
        QSqlDatabase::removeDatabase(DATABASE_CONNECTION_NAME);
    }
}

bool PlaylistDatabase::isConnected() const
{
    return m_database.isOpen() && m_database.isValid();
}

bool PlaylistDatabase::createTables()
{
    // Begin a database transaction to ensure that all table creation operations are atomic.
    // If any table creation fails, the entire transaction can be rolled back,
    // leaving the database in a consistent state.
    if (!beginTransaction()) {
        return false;
    }

    try {
        if (!createPlaylistsTable()) {
            rollbackTransaction();
            return false;
        }

        if (!createAudioItemsTable()) {
            rollbackTransaction();
            return false;
        }

        if (!createPlaylistItemsTable()) {
            rollbackTransaction();
            return false;
        }

        if (!createIndexes()) {
            rollbackTransaction();
            return false;
        }

        // Commit the transaction only if all table and index creations were successful.
        return commitTransaction();

    } catch (const std::exception &e) {
        // Rollback the transaction if any exception occurs during table creation,
        // ensuring data integrity and preventing partial table structures.
        rollbackTransaction();
        m_lastError = QString("Exception occurred while creating tables: %1").arg(e.what());
        qCritical() << m_lastError;
        return false;
    }
}

bool PlaylistDatabase::checkTableExists(const QString &tableName)
{
    QSqlQuery query(m_database);
    query.prepare("SELECT name FROM sqlite_master WHERE type='table' AND name=?");
    query.addBindValue(tableName);

    if (!query.exec()) {
        logError("Check if table exists", query.lastError());
        return false;
    }

    return query.next();
}

QSqlQuery PlaylistDatabase::executeQuery(const QString &queryString, const QVariantList &values)
{
    QSqlQuery query(m_database);
    query.prepare(queryString);

    for (const QVariant &value : values) {
        query.addBindValue(value);
    }

    if (!query.exec()) {
        logError("Execute query", query.lastError());
    }

    return query;
}

bool PlaylistDatabase::executeNonQuery(const QString &queryString, const QVariantList &values)
{
    QSqlQuery query = executeQuery(queryString, values);
    return !query.lastError().isValid();
}

bool PlaylistDatabase::beginTransaction()
{
    if (!m_database.transaction()) {
        logError("Begin transaction", m_database.lastError());
        return false;
    }
    return true;
}

bool PlaylistDatabase::commitTransaction()
{
    if (!m_database.commit()) {
        logError("Commit transaction", m_database.lastError());
        return false;
    }
    return true;
}

bool PlaylistDatabase::rollbackTransaction()
{
    if (!m_database.rollback()) {
        logError("Rollback transaction", m_database.lastError());
        return false;
    }
    return true;
}

QString PlaylistDatabase::lastError() const
{
    return m_lastError;
}

void PlaylistDatabase::logError(const QString &operation, const QSqlError &error)
{
    if (error.isValid()) {
        m_lastError = QString("%1 failed: %2").arg(operation, error.text());
        emit databaseError(m_lastError);
    }
}



QString PlaylistDatabase::getDatabasePath()
{
    // Determine the standard writable location for application data.
    // This ensures that the database file is stored in a user-appropriate and system-compliant location.
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir appDir(appDataPath);

    if (!appDir.exists()) {
        // If the application data directory does not exist, attempt to create it.
        // If creation fails, fall back to the system's temporary directory to ensure the application can still function.
        if (!appDir.mkpath(".")) {
            appDataPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
        }
    }

    // Construct the full path to the database file within the determined application data directory.
    return appDir.filePath("playlists.db");
}

bool PlaylistDatabase::createPlaylistsTable()
{
    // Defines the schema for the 'playlists' table.
    // This table stores metadata about each playlist, such as its name, creation/update timestamps,
    // the current playback mode, and the index of the currently playing song within that playlist.
    // The 'name' field is set to be UNIQUE to ensure that each playlist has a distinct identifier.
    const QString createTableQuery = R"(
        CREATE TABLE IF NOT EXISTS playlists (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL UNIQUE,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            play_mode INTEGER DEFAULT 0,
            current_index INTEGER DEFAULT -1
        )
    )";

    QSqlQuery query(m_database);
    if (!query.exec(createTableQuery)) {
        logError("Create playlists table", query.lastError());
        return false;
    }

    return true;
}

bool PlaylistDatabase::createAudioItemsTable()
{
    // Defines the schema for the 'audio_items' table.
    // This table stores unique audio track information, including title, artist, and various source URLs.
    // The 'audio_source' field is marked as UNIQUE to prevent duplicate entries for the same audio file,
    // ensuring data integrity and efficient lookups.
    const QString createTableQuery = R"(
        CREATE TABLE IF NOT EXISTS audio_items (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            title TEXT NOT NULL,
            author_name TEXT,
            audio_source TEXT NOT NULL UNIQUE,
            image_source TEXT,
            video_source TEXT,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        )
    )";

    QSqlQuery query(m_database);
    if (!query.exec(createTableQuery)) {
        logError("Create audio items table", query.lastError());
        return false;
    }

    return true;
}

bool PlaylistDatabase::createPlaylistItemsTable()
{
    // Defines the schema for the 'playlist_items' table, which acts as a junction table
    // to link audio items to playlists. This many-to-many relationship allows a single audio item
    // to appear in multiple playlists and a playlist to contain multiple audio items.
    // The 'position' field maintains the order of songs within a specific playlist.
    const QString createTableQuery = R"(
        CREATE TABLE IF NOT EXISTS playlist_items (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            playlist_id INTEGER NOT NULL,
            audio_item_id INTEGER NOT NULL,
            position INTEGER NOT NULL,
            FOREIGN KEY (playlist_id) REFERENCES playlists(id) ON DELETE CASCADE,
            FOREIGN KEY (audio_item_id) REFERENCES audio_items(id) ON DELETE CASCADE,
            UNIQUE(playlist_id, position) -- Ensures that each position within a playlist is unique.
        )
    )";

    QSqlQuery query(m_database);
    if (!query.exec(createTableQuery)) {
        logError("Create playlist items table", query.lastError());
        return false;
    }

    return true;
}

bool PlaylistDatabase::createIndexes()
{
    // Defines a list of SQL queries to create indexes on frequently queried columns.
    // Indexes significantly improve the performance of read operations (SELECT statements)
    // by allowing the database to quickly locate rows without scanning the entire table.
    // This is crucial for responsive playlist management and audio item lookups.
    QStringList indexQueries = {
        "CREATE INDEX IF NOT EXISTS idx_audio_source ON audio_items(audio_source)",
        "CREATE INDEX IF NOT EXISTS idx_playlist_items_playlist ON playlist_items(playlist_id)",
        "CREATE INDEX IF NOT EXISTS idx_playlist_items_position ON playlist_items(playlist_id, position)",
        "CREATE INDEX IF NOT EXISTS idx_playlists_name ON playlists(name)"
    };

    for (const QString &indexQuery : indexQueries) {
        QSqlQuery query(m_database);
        if (!query.exec(indexQuery)) {
            logError("Create index", query.lastError());
            return false;
        }
    }

    return true;
}


