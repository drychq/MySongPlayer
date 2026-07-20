#include "models/PlaylistModel.h"
#include "adapters/QtAudioTrackAdapter.h"

#include <algorithm>
#include <limits>
#include <utility>

using std::in_range;
using std::min;
using std::numeric_limits;
using std::nullopt;
using std::optional;
using std::size_t;

namespace {

bool isValidPlayMode(PlayMode mode) noexcept
{
    switch (mode) {
    case PlayMode::Loop:
    case PlayMode::Shuffle:
    case PlayMode::RepeatOne:
        return true;
    }
    return false;
}

AudioInfo *replacementAfterRemoval(const QList<AudioInfo *> &audioList,
                                   AudioInfo *currentSong,
                                   int removedIndex)
{
    if (audioList[removedIndex] != currentSong) {
        return currentSong;
    }
    if (audioList.size() == 1) {
        return nullptr;
    }
    if (removedIndex + 1 < audioList.size()) {
        return audioList[removedIndex + 1];
    }
    return audioList[removedIndex - 1];
}

} // namespace

PlaylistModel::PlaylistModel(QObject *parent)
    : QAbstractListModel{parent}
{}

PlaylistModel::~PlaylistModel()
{
    clearPlaylist();
}

int PlaylistModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    const size_t maximum{static_cast<size_t>(numeric_limits<int>::max())};
    return static_cast<int>(min(m_playlist.size(), maximum));
}

QVariant PlaylistModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid() && index.row() >= 0 && index.row() < m_audioList.size()) {
        AudioInfo *audioInfo{m_audioList[index.row()]};

        switch (static_cast<Role>(role)) {
        case AudioTitleRole:
            return audioInfo->title();
        case AudioAuthorNameRole:
            return audioInfo->authorName();
        case AudioSourceRole:
            return audioInfo->audioSource();
        case AudioImageSourceRole:
            return audioInfo->imageSource();
        case AudioVideoSourceRole:
            return audioInfo->videoSource();
        }
    }

    return {};
}

QHash<int, QByteArray> PlaylistModel::roleNames() const
{
    QHash<int, QByteArray> result;

    result[AudioAuthorNameRole] = "audioAuthorName";
    result[AudioTitleRole] = "audioTitle";
    result[AudioSourceRole] = "audioSource";
    result[AudioImageSourceRole] = "audioImageSource";
    result[AudioVideoSourceRole] = "audioVideoSource";

    return result;
}

AudioInfo *PlaylistModel::currentSong() const
{
    return m_currentSong;
}

void PlaylistModel::setCurrentSong(AudioInfo *newCurrentSong)
{
    if (m_currentSong == newCurrentSong)
        return;

    m_currentSong = newCurrentSong;
    syncCoreCurrentSong();
    emit currentSongChanged();
}

optional<size_t> PlaylistModel::currentIndex() const noexcept
{
    return m_playlist.currentIndex();
}

int PlaylistModel::currentSongIndex() const
{
    const optional<size_t> index{m_playlist.currentIndex()};
    if (!index || !in_range<int>(*index)) {
        return -1;
    }

    return static_cast<int>(*index);
}

PlayMode PlaylistModel::playMode() const
{
    return m_playlist.playMode();
}

void PlaylistModel::setPlayMode(PlayMode newMode)
{
    if (!isValidPlayMode(newMode) || m_playlist.playMode() == newMode) {
        return;
    }

    m_playlist.setPlayMode(newMode);
    emit playModeChanged();
}

bool PlaylistModel::addAudio(const QString &title, const QString &authorName,
                             const QUrl &audioSource, const QUrl &imageSource,
                             const QUrl &videoSource)
{
    const SongPlayer::Core::AudioTrack track{SongPlayer::QtAdapter::makeCoreTrack(
        title, authorName, audioSource, imageSource, videoSource)};
    if (track.audioSource.empty() || m_playlist.containsSource(track.audioSource)) {
        emit duplicateAudioSkipped(title, QStringLiteral("Audio file already exists in playlist"));
        return false;
    }
    if (m_audioList.size() >= numeric_limits<int>::max()) {
        qWarning() << "Playlist has reached the model row limit";
        return false;
    }

    const int insertionIndex{static_cast<int>(m_audioList.size())};
    beginInsertRows(QModelIndex{}, insertionIndex, insertionIndex);
    const bool trackAdded{m_playlist.addTrack(track)};
    Q_ASSERT(trackAdded);

    auto *audioInfo{new AudioInfo{this}};

    audioInfo->setTitle(title);
    audioInfo->setAuthorName(authorName);
    audioInfo->setAudioSource(audioSource);
    audioInfo->setImageSource(imageSource);
    audioInfo->setVideoSource(videoSource);

    const bool shouldBecomeCurrent{m_audioList.isEmpty()};

    m_audioList << audioInfo;

    endInsertRows();

    if (shouldBecomeCurrent) {
        setCurrentSong(audioInfo);
    }
    return true;
}

void PlaylistModel::removeAudio(int index)
{
    if (index < 0 || index >= m_audioList.size()) {
        return;
    }

    AudioInfo *toRemove{m_audioList[index]};
    AudioInfo *newCurrentSong{replacementAfterRemoval(m_audioList, m_currentSong, index)};
    beginRemoveRows({}, index, index);
    m_audioList.removeAt(index);
    m_playlist.removeTrack(static_cast<size_t>(index));
    toRemove->deleteLater();
    endRemoveRows();

    if (m_currentSong != newCurrentSong) {
        setCurrentSong(newCurrentSong);
        return;
    }
    syncCoreCurrentSong();
}

void PlaylistModel::clearPlaylist()
{
    if (!m_audioList.isEmpty()) {
        const int lastIndex{static_cast<int>(m_audioList.size() - 1)};
        beginRemoveRows({}, 0, lastIndex);

        setCurrentSong(nullptr);
        m_playlist.clear();

        qDeleteAll(m_audioList);
        m_audioList.clear();

        endRemoveRows();
    }
}

AudioInfo* PlaylistModel::getAudioInfoAtIndex(int index) const
{
    if (index >= 0 && index < m_audioList.size()) {
        return m_audioList[index];
    }
    return nullptr;
}

bool PlaylistModel::isDuplicateAudio(const QUrl& audioSource) const
{
    return m_playlist.containsSource(SongPlayer::QtAdapter::toSourceKey(audioSource));
}

optional<size_t> PlaylistModel::nextSongIndex(optional<size_t> shuffleIndex) const noexcept
{
    return m_playlist.nextIndex(shuffleIndex);
}

optional<size_t> PlaylistModel::previousSongIndex(optional<size_t> shuffleIndex) const noexcept
{
    return m_playlist.previousIndex(shuffleIndex);
}

void PlaylistModel::syncCoreCurrentSong()
{
    if (!m_currentSong) {
        m_playlist.setCurrentIndex(nullopt);
        return;
    }

    const qsizetype index{m_audioList.indexOf(m_currentSong)};
    if (index < 0) {
        m_playlist.setCurrentIndex(nullopt);
        return;
    }

    m_playlist.setCurrentIndex(static_cast<size_t>(index));
}
