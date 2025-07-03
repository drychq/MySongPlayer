#pragma once

#include <QAbstractListModel>
#include <QtQml/qqmlregistration.h>
#include <QGuiApplication>
#include <QQmlEngine>
#include <QJSEngine>

#include "models/AudioInfo.h"

class PlaylistSearchModel : public QAbstractListModel
{
    Q_OBJECT
    QML_SINGLETON
    QML_NAMED_ELEMENT(PlaylistSearchModel)
    Q_PROPERTY(bool isSearching READ isSearching NOTIFY isSearchingChanged)

public:
    enum Role {
        AudioTitleRole = Qt::UserRole + 1,
        AudioAuthorNameRole,
        AudioImageSourceRole,
        AudioSourceRole,
        OriginalIndexRole
    };
    Q_ENUM(Role)

    static QObject* provider(QQmlEngine *engine, QJSEngine *scriptEngine) {
        Q_UNUSED(engine); Q_UNUSED(scriptEngine);
        auto model = new PlaylistSearchModel(QGuiApplication::instance());
        return model;
    }

    explicit PlaylistSearchModel(QObject *parent = nullptr);
    virtual ~PlaylistSearchModel();

    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    virtual QVariant data(const QModelIndex &index, int role) const override;
    virtual QHash<int, QByteArray> roleNames() const override;

    bool isSearching() const;
    void setIsSearching(bool newIsSearching);

public slots:
    void searchInPlaylist(const QString &searchText);

    void clearSearch();

    void performSearch(const QVariantList &audioInfoList, const QString &searchText);

signals:
    void isSearchingChanged();

private:
    struct SearchResult {
        AudioInfo* audioInfo;
        int originalIndex;

        SearchResult() : audioInfo(nullptr), originalIndex(-1) {}
        SearchResult(AudioInfo* info, int index) : audioInfo(info), originalIndex(index) {}
    };

    bool matchesSearchCriteria(AudioInfo* audioInfo, const QString &searchText) const;

    void removeDuplicateResults();

    QList<SearchResult> m_searchResults;
    bool m_isSearching;
    QString m_currentSearchText;
};
