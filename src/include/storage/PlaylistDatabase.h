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

#include <functional>

namespace SongPlayer {

class PlaylistDatabase : public QObject
{
    Q_OBJECT

public:
    explicit PlaylistDatabase(QObject *parent = nullptr);
    ~PlaylistDatabase() override;

    bool initializeDatabase();
    void closeDatabase();
    bool createTables();

    QSqlQuery executeQuery(const QString &queryString, const QVariantList &values = QVariantList());
    bool executeNonQuery(const QString &queryString, const QVariantList &values = QVariantList());


    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();
    bool runInTransaction(const std::function<bool()>& operation);


    QString lastError() const;
    void logError(const QString &operation, const QSqlError &error);

signals:
    void databaseError(const QString &error);

private:
    static constexpr const char *DATABASE_CONNECTION_NAME{"playlist_connection"};

    QSqlDatabase m_database{};
    QString m_databasePath{};
    QString m_lastError{};

    QString getDatabasePath();
    bool createPlaylistsTable();
    bool createAudioItemsTable();
    bool createPlaylistItemsTable();
    bool createIndexes();
};

}
