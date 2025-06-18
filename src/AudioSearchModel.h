#pragma once

#include <QAbstractListModel>
#include <QNetworkAccessManager>
#include <QtQml/qqmlregistration.h>
#include <QGuiApplication>
#include <QQmlEngine>
#include <QJSEngine>

class AudioInfo;

class AudioSearchModel : public QAbstractListModel
{
    Q_OBJECT
    QML_SINGLETON
    QML_NAMED_ELEMENT(AudioSearchModel)
    Q_PROPERTY(bool isSearching READ isSearching NOTIFY isSearchingChanged)

public:
    static QObject* provider(QQmlEngine *engine, QJSEngine *scriptEngine) {
        Q_UNUSED(engine); Q_UNUSED(scriptEngine);
        auto model = new AudioSearchModel(QGuiApplication::instance());
        return model;
    }

    enum Role {
        AudioNameRole = Qt::UserRole + 1,
        AudioAuthorRole,
        AudioImageSourceRole,
        AudioSourceRole
    };

    explicit AudioSearchModel(QObject *parent = nullptr);

    virtual int rowCount(const QModelIndex &parent) const override;
    virtual QVariant data(const QModelIndex &index, int role) const override;
    virtual QHash<int, QByteArray> roleNames() const override;

    bool isSearching() const;
    void setIsSearching(bool newIsSearching);

public slots:
    void searchSong(const QString &name);
    void parseData();

signals:
    void isSearchingChanged();

private:
    QList<AudioInfo*> m_audioList;
    QNetworkAccessManager m_networkManager;
    QNetworkReply *m_reply = nullptr;
    bool m_isSearching;
};


