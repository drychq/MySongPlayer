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
        m_database = QSqlDatabase::addDatabase("QSQLITE", DATABASE_CONNECTION_NAME);
        m_database.setDatabaseName(m_databasePath);

        if (!m_database.open()) {
            m_lastError = QString("Unable to open database: %1").arg(m_database.lastError().text());
            qCritical() << m_lastError;
            return false;
        }

        QSqlQuery query(m_database);
        if (!query.exec("PRAGMA foreign_keys = ON")) {
            logError("Enable foreign key constraints", query.lastError());
            return false;
        }

        if (!createTables()) {
            m_lastError = "Failed to create database tables";
            return false;
        }

        return true;

    } catch (const std::exception &e) {
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

        return commitTransaction();

    } catch (const std::exception &e) {
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
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir appDir(appDataPath);

    if (!appDir.exists()) {
        if (!appDir.mkpath(".")) {
            appDataPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
        }
    }

    return appDir.filePath("playlists.db");
}

bool PlaylistDatabase::createPlaylistsTable()
{
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
    const QString createTableQuery = R"(
        CREATE TABLE IF NOT EXISTS playlist_items (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            playlist_id INTEGER NOT NULL,
            audio_item_id INTEGER NOT NULL,
            position INTEGER NOT NULL,
            FOREIGN KEY (playlist_id) REFERENCES playlists(id) ON DELETE CASCADE,
            FOREIGN KEY (audio_item_id) REFERENCES audio_items(id) ON DELETE CASCADE,
            UNIQUE(playlist_id, position)
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


