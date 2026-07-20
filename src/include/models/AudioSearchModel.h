#pragma once

#include <QAbstractListModel>
#include <QNetworkAccessManager>
#include <QPointer>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

#include "models/AudioInfo.h"

class AudioSearchModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(bool isSearching READ isSearching NOTIFY isSearchingChanged)

public:
    enum Role {
        AudioNameRole = Qt::UserRole + 1,
        AudioAuthorRole,
        AudioImageSourceRole,
        AudioSourceRole
    };
    Q_ENUM(Role)

    explicit AudioSearchModel(QObject *parent = nullptr);
    // Test seam: controlled network tests inject deterministic replies and an isolated URL.
    // Production QML construction continues to use the QObject-only constructor above.
    AudioSearchModel(QNetworkAccessManager *networkManager,
                     QUrl requestUrl,
                     QObject *parent = nullptr);
    ~AudioSearchModel() override;

    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    bool isSearching() const;
    void setIsSearching(bool newIsSearching);

public slots:
    void searchSong(const QString &name);

signals:
    void isSearchingChanged();

private:
    void cancelActiveRequest();
    void handleReply(const QPointer<QNetworkReply> &reply);

    QList<AudioInfo *> m_audioList{};
    QNetworkAccessManager m_ownedNetworkManager{};
    QPointer<QNetworkAccessManager> m_networkManager{};
    QPointer<QNetworkReply> m_reply{};
    QUrl m_requestUrl{};
    bool m_isSearching{false};
};
