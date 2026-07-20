#pragma once

#include <QAbstractListModel>
#include <QtQml/qqmlregistration.h>

#include "models/AudioInfo.h"

class PlaylistSearchModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
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

    explicit PlaylistSearchModel(QObject *parent = nullptr);
    ~PlaylistSearchModel() override = default;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    bool isSearching() const;
    void setIsSearching(bool newIsSearching);

public slots:
    void clearSearch();

    void performSearch(const QVariantList &audioInfoList, const QString &searchText);

signals:
    void isSearchingChanged();

private:
    struct SearchResult {
        AudioInfo *audioInfo{nullptr};
        int originalIndex{-1};

        SearchResult() = default;
        SearchResult(AudioInfo *info, int index) : audioInfo{info}, originalIndex{index} {}
    };

    QList<SearchResult> m_searchResults{};
    bool m_isSearching{false};
    QString m_currentSearchText{};
};
