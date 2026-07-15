// Written by HanQin Chen (cqnuchq@outlook.com) 2025-06-22
#include "models/PlaylistModel.h"
#include "adapters/QtAudioTrackAdapter.h"
#include <QDebug>

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
    return static_cast<int>(m_playlist.size());
}

QVariant PlaylistModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid() && index.row() >= 0 && index.row() < m_audioList.size()) {
        AudioInfo * audioInfo = m_audioList[index.row()];

        switch((Role) role) {
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

std::optional<std::size_t> PlaylistModel::currentIndex() const noexcept
{
    return m_playlist.currentIndex();
}

int PlaylistModel::currentSongIndex() const
{
    const std::optional<std::size_t> index = m_playlist.currentIndex();
    if (!index) {
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
    if (m_playlist.playMode() == newMode)
        return;

    m_playlist.setPlayMode(newMode);
    emit playModeChanged();
}

void PlaylistModel::addAudio(const QString &title, const QString &authorName,
                             const QUrl &audioSource, const QUrl &imageSource,
                             const QUrl &videoSource)
{
    const SongPlayer::Core::AudioTrack track = SongPlayer::QtAdapter::makeCoreTrack(
        title, authorName, audioSource, imageSource, videoSource);
    if (track.audioSource.empty() || m_playlist.containsSource(track.audioSource)) {
        emit duplicateAudioSkipped(title, "Audio file already exists in playlist");
        return;
    }

    beginInsertRows(QModelIndex(), m_audioList.size(), m_audioList.size());
    const bool trackAdded = m_playlist.addTrack(track);
    Q_ASSERT(trackAdded);

    AudioInfo *audioInfo = new AudioInfo(this);

    audioInfo->setTitle(title);
    audioInfo->setAuthorName(authorName);
    audioInfo->setAudioSource(audioSource);
    audioInfo->setImageSource(imageSource);
    audioInfo->setVideoSource(videoSource);

    const bool shouldBecomeCurrent = m_audioList.isEmpty();

    m_audioList << audioInfo;

    endInsertRows();

    if (shouldBecomeCurrent) {
        setCurrentSong(audioInfo);
    }
}

void PlaylistModel::removeAudio(int index)
{
    if (index >= 0 && index < m_audioList.size()) {
        AudioInfo *toRemove = m_audioList[index];
        AudioInfo *newCurrentSong = m_currentSong;

        if (toRemove == m_currentSong) {
            if (m_audioList.size() > 1) {
                if (index < m_audioList.size() - 1) {
                    newCurrentSong = m_audioList[index + 1];
                } else {
                    newCurrentSong = m_audioList[index - 1];
                }
            } else {
                newCurrentSong = nullptr;
            }
        }

        beginRemoveRows(QModelIndex(), index, index);

        m_audioList.removeAt(index);
        m_playlist.removeTrack(static_cast<std::size_t>(index));
        toRemove->deleteLater();

        endRemoveRows();

        if (m_currentSong != newCurrentSong) {
            setCurrentSong(newCurrentSong);
        } else {
            syncCoreCurrentSong();
        }
    } else {

    }
}

void PlaylistModel::clearPlaylist()
{
    if (!m_audioList.isEmpty()) {
        beginRemoveRows(QModelIndex(), 0, m_audioList.size() - 1);

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

std::optional<std::size_t> PlaylistModel::nextSongIndex(std::optional<std::size_t> shuffleIndex) const noexcept
{
    return m_playlist.nextIndex(shuffleIndex);
}

std::optional<std::size_t> PlaylistModel::previousSongIndex(std::optional<std::size_t> shuffleIndex) const noexcept
{
    return m_playlist.previousIndex(shuffleIndex);
}

void PlaylistModel::forceRefresh()
{
    if (m_audioList.isEmpty()) {
        return;
    }

    emit dataChanged(index(0, 0), index(m_audioList.size() - 1, 0));
    emit currentSongChanged();
}

bool PlaylistModel::validateModelState() const
{
    if (m_playlist.size() != static_cast<std::size_t>(m_audioList.size())) {
        return false;
    }

    if (m_currentSong && !m_audioList.contains(m_currentSong)) {
        return false;
    }

    for (const AudioInfo *audioInfo : m_audioList) {
        if (!audioInfo || audioInfo->audioSource().isEmpty()) {
            return false;
        }
    }

    return true;
}

QStringList PlaylistModel::getModelDiagnostics() const
{
    QStringList diagnostics;
    diagnostics << QString("itemCount=%1").arg(m_playlist.size());
    diagnostics << QString("hasCurrentSong=%1").arg(m_currentSong != nullptr);
    diagnostics << QString("playMode=%1").arg(static_cast<int>(m_playlist.playMode()));
    diagnostics << QString("valid=%1").arg(validateModelState());
    return diagnostics;
}

void PlaylistModel::performModelDiagnostic() const
{
    const QStringList diagnostics = getModelDiagnostics();
    for (const QString &line : diagnostics) {
        qDebug() << "PlaylistModel:" << line;
    }
}

void PlaylistModel::syncCoreCurrentSong()
{
    if (!m_currentSong) {
        m_playlist.setCurrentIndex(std::nullopt);
        return;
    }

    const int index = m_audioList.indexOf(m_currentSong);
    if (index < 0) {
        m_playlist.setCurrentIndex(std::nullopt);
        return;
    }

    m_playlist.setCurrentIndex(static_cast<std::size_t>(index));
}
