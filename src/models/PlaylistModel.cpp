// Written by HanQin Chen (cqnuchq@outlook.com) 2025-06-22
#include "models/PlaylistModel.h"

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
    return m_audioList.size();
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
    emit currentSongChanged();
}

PlayMode PlaylistModel::playMode() const
{
    return m_playMode;
}

void PlaylistModel::setPlayMode(PlayMode newMode)
{
    if (m_playMode == newMode)
        return;

    m_playMode = newMode;
    emit playModeChanged();

    if (m_playMode == PlayMode::Shuffle && !m_audioList.isEmpty()) {
        generateShuffleSequence();
    }
}

void PlaylistModel::addAudio(const QString &title, const QString &authorName,
                             const QUrl &audioSource, const QUrl &imageSource,
                             const QUrl &videoSource)
{
    if (isDuplicateAudio(audioSource)) {

        emit duplicateAudioSkipped(title, "Audio file already exists in playlist");
        return;
    }


    beginInsertRows(QModelIndex(), m_audioList.size(), m_audioList.size());

    AudioInfo *audioInfo = new AudioInfo(this);

    audioInfo->setTitle(title);
    audioInfo->setAuthorName(authorName);
    audioInfo->setAudioSource(audioSource);
    audioInfo->setImageSource(imageSource);
    audioInfo->setVideoSource(videoSource);

    if (m_audioList.isEmpty()) {
        setCurrentSong(audioInfo);
    }

    m_audioList << audioInfo;

    endInsertRows();

}

void PlaylistModel::removeAudio(int index)
{
    if (index >= 0 && index < m_audioList.size()) {
        AudioInfo *toRemove = m_audioList[index];

        beginRemoveRows(QModelIndex(), index, index);

        if (toRemove == m_currentSong) {
            if (m_audioList.size() > 1) {
                AudioInfo* newCurrentSong = nullptr;
                if (index < m_audioList.size() - 1) {
                    newCurrentSong = m_audioList[index + 1];
                } else {
                    newCurrentSong = m_audioList[index - 1];
                }
                setCurrentSong(newCurrentSong);
            } else {
                setCurrentSong(nullptr);
            }
        }

        m_audioList.removeAt(index);
        toRemove->deleteLater();

        endRemoveRows();

    } else {

    }
}

void PlaylistModel::clearPlaylist()
{
    if (!m_audioList.isEmpty()) {
        beginRemoveRows(QModelIndex(), 0, m_audioList.size() - 1);

        setCurrentSong(nullptr);

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
    for (const AudioInfo* info : m_audioList) {
        if (info && info->audioSource() == audioSource) {
            return true;
        }
    }
    return false;
}

void PlaylistModel::generateShuffleSequence()
{
    m_shuffleIndices.clear();

    for (int i = 0; i < m_audioList.size(); ++i) {
        m_shuffleIndices.append(i);
    }

    auto* generator = QRandomGenerator::global();
    for (int i = m_shuffleIndices.size() - 1; i > 0; --i) {
        int j = generator->bounded(i + 1);
        m_shuffleIndices.swapItemsAt(i, j);
    }

    if (m_currentSong) {
        int currentIndex = m_audioList.indexOf(m_currentSong);
        m_currentShuffleIndex = m_shuffleIndices.indexOf(currentIndex);
    } else {
        m_currentShuffleIndex = -1;
    }
}
