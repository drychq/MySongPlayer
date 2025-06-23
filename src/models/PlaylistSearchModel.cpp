#include "models/PlaylistSearchModel.h"
#include <QSet>

PlaylistSearchModel::PlaylistSearchModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_isSearching(false)
{}

PlaylistSearchModel::~PlaylistSearchModel()
{}

int PlaylistSearchModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_searchResults.size();
}

QVariant PlaylistSearchModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_searchResults.size()) {
        return {};
    }

    const SearchResult &result = m_searchResults[index.row()];
    AudioInfo *audioInfo = result.audioInfo;

    if (!audioInfo) {
        return {};
    }

    switch(static_cast<Role>(role)) {
    case AudioTitleRole:
        return audioInfo->title();
    case AudioAuthorNameRole:
        return audioInfo->authorName();
    case AudioImageSourceRole:
        return audioInfo->imageSource();
    case AudioSourceRole:
        return audioInfo->audioSource();
    case OriginalIndexRole:
        return result.originalIndex;
    }

    return {};
}

QHash<int, QByteArray> PlaylistSearchModel::roleNames() const
{
    QHash<int, QByteArray> names;
    names[AudioTitleRole] = "audioTitle";
    names[AudioAuthorNameRole] = "audioAuthorName";
    names[AudioImageSourceRole] = "audioImageSource";
    names[AudioSourceRole] = "audioSource";
    names[OriginalIndexRole] = "originalIndex";
    return names;
}

bool PlaylistSearchModel::isSearching() const
{
    return m_isSearching;
}

void PlaylistSearchModel::setIsSearching(bool newIsSearching)
{
    if (m_isSearching == newIsSearching)
        return;

    m_isSearching = newIsSearching;
    emit isSearchingChanged();
}

void PlaylistSearchModel::searchInPlaylist(const QString &searchText)
{
    m_currentSearchText = searchText.trimmed();
}

void PlaylistSearchModel::performSearch(const QVariantList &audioInfoList, const QString &searchText)
{
    if (audioInfoList.isEmpty() || searchText.isEmpty()) {
        clearSearch();
        return;
    }

    setIsSearching(true);
    m_currentSearchText = searchText.trimmed();

    beginResetModel();
    m_searchResults.clear();

    if (m_currentSearchText.isEmpty()) {
        endResetModel();
        setIsSearching(false);
        return;
    }

    for (int i = 0; i < audioInfoList.size(); ++i) {
        QObject* obj = qvariant_cast<QObject*>(audioInfoList[i]);
        AudioInfo* audioInfo = qobject_cast<AudioInfo*>(obj);

        if (audioInfo && matchesSearchCriteria(audioInfo, m_currentSearchText)) {
            SearchResult result(audioInfo, i);
            m_searchResults.append(result);
        }
    }

    removeDuplicateResults();

    endResetModel();
    setIsSearching(false);
}

void PlaylistSearchModel::clearSearch()
{
    m_currentSearchText.clear();
    setIsSearching(false);

    beginResetModel();
    m_searchResults.clear();
    endResetModel();
}

bool PlaylistSearchModel::matchesSearchCriteria(AudioInfo* audioInfo, const QString &searchText) const
{
    if (!audioInfo || searchText.isEmpty()) {
        return false;
    }

    QString searchLower = searchText.toLower();

    if (audioInfo->title().toLower().contains(searchLower)) {
        return true;
    }

    if (audioInfo->authorName().toLower().contains(searchLower)) {
        return true;
    }

    return false;
}

void PlaylistSearchModel::removeDuplicateResults()
{
    if (m_searchResults.isEmpty()) {
        return;
    }

    QSet<QUrl> seenSources;
    auto it = m_searchResults.begin();
    int originalCount = m_searchResults.size();

    while (it != m_searchResults.end()) {
        if (it->audioInfo && seenSources.contains(it->audioInfo->audioSource())) {
            it = m_searchResults.erase(it);
        } else if (it->audioInfo) {
            seenSources.insert(it->audioInfo->audioSource());
            ++it;
        } else {
            it = m_searchResults.erase(it);
        }
    }

    int finalCount = m_searchResults.size();
    if (originalCount != finalCount) {

    }
}
