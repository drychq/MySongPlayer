// Written by HanQin Chen (cqnuchq@outlook.com) 2025-07-04
#pragma once

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QString>
#include <QUrl>
#include <QVariant>
#include <QVariantList>
#include <QStandardPaths>
#include <QDir>

class PlaylistDatabase : public QObject
{
    Q_OBJECT
public:
    explicit PlaylistDatabase(QObject *parent = nullptr);
    virtual ~PlaylistDatabase();

    bool initializeDatabase();
    void closeDatabase();
    bool isConnected() const;

    bool createTables();
    bool checkTableExists(const QString &tableName);

    QSqlQuery executeQuery(const QString &queryString, const QVariantList &values = QVariantList());
    bool executeNonQuery(const QString &queryString, const QVariantList &values = QVariantList());


    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();


    QString lastError() const;
    void logError(const QString &operation, const QSqlError &error);

signals:
    void databaseError(const QString &error);

private:
    static constexpr const char* DATABASE_CONNECTION_NAME = "playlist_connection";

    QSqlDatabase m_database;
    QString m_databasePath;
    QString m_lastError;

    QString getDatabasePath();
    bool createPlaylistsTable();
    bool createAudioItemsTable();
    bool createPlaylistItemsTable();
    bool createIndexes();
};
