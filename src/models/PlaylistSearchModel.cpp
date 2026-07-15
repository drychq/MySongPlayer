#include "models/PlaylistSearchModel.h"
#include "adapters/QtAudioTrackAdapter.h"
#include "core/Playlist.h"

#include <utility>
#include <vector>

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

    std::vector<SongPlayer::Core::AudioTrack> tracks;
    std::vector<AudioInfo*> audioInfos;
    std::vector<int> originalIndexes;
    tracks.reserve(static_cast<std::size_t>(audioInfoList.size()));
    audioInfos.reserve(static_cast<std::size_t>(audioInfoList.size()));
    originalIndexes.reserve(static_cast<std::size_t>(audioInfoList.size()));

    for (int i = 0; i < audioInfoList.size(); ++i) {
        QObject* obj = qvariant_cast<QObject*>(audioInfoList[i]);
        if (!obj || !obj->inherits(AudioInfo::staticMetaObject.className())) {
            continue;
        }

        auto* audioInfo = static_cast<AudioInfo*>(obj);
        SongPlayer::Core::AudioTrack track = SongPlayer::QtAdapter::makeCoreTrack(
            audioInfo->title(),
            audioInfo->authorName(),
            audioInfo->audioSource(),
            audioInfo->imageSource(),
            audioInfo->videoSource());
        track.songIndex = audioInfo->songIndex();
        tracks.push_back(std::move(track));
        audioInfos.push_back(audioInfo);
        originalIndexes.push_back(i);
    }

    const auto results = SongPlayer::Core::searchTracks(
        tracks,
        SongPlayer::QtAdapter::toUtf8String(m_currentSearchText));

    for (const SongPlayer::Core::PlaylistSearchResult& result : results) {
        const std::size_t trackIndex = result.originalIndex;
        if (trackIndex >= audioInfos.size()) {
            continue;
        }

        m_searchResults.append(SearchResult(audioInfos[trackIndex], originalIndexes[trackIndex]));
    }

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
