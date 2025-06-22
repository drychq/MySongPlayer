#include "PlaylistCoordinator.h"
#include "models/PlaylistModel.h"
#include <QRandomGenerator>
PlaylistCoordinator::PlaylistCoordinator(PlaylistModel *playlistModel, QObject *parent)
    : QObject(parent)
    , m_playlistModel(playlistModel)
    , m_currentPlaylistName("Default Playlist")
{
    connect(m_playlistModel, &PlaylistModel::currentSongChanged,
            this, &PlaylistCoordinator::onCurrentSongChanged);
    connect(m_playlistModel, &PlaylistModel::rowsInserted,
            this, &PlaylistCoordinator::onRowsInserted);
}

AudioInfo* PlaylistCoordinator::currentSong() const
{
    return m_playlistModel->currentSong();
}

void PlaylistCoordinator::setCurrentSong(AudioInfo *newCurrentSong)
{
    m_playlistModel->setCurrentSong(newCurrentSong);
}
void PlaylistCoordinator::switchToNextSong()
{
    if (!m_playlistModel || m_playlistModel->rowCount() == 0) {
        return;
    }

    PlayMode playMode = m_playlistModel->playMode();
    AudioInfo* currentSong = m_playlistModel->currentSong();

    switch (playMode) {
    case PlayMode::RepeatOne:
    case PlayMode::Loop: {
        if (!currentSong) {
            AudioInfo* firstSong = m_playlistModel->getAudioInfoAtIndex(0);
            m_playlistModel->setCurrentSong(firstSong);
            if (firstSong) {
                emit requestAudioSourceChange(firstSong->audioSource());
            }
            return;
        }

        // Find current song index and switch to next
        int currentIndex = -1;
        for (int i = 0; i < m_playlistModel->rowCount(); ++i) {
            if (m_playlistModel->getAudioInfoAtIndex(i) == currentSong) {
                currentIndex = i;
                break;
            }
        }

        int nextIndex = (currentIndex + 1) % m_playlistModel->rowCount();
        AudioInfo* nextSong = m_playlistModel->getAudioInfoAtIndex(nextIndex);
        m_playlistModel->setCurrentSong(nextSong);
        if (nextSong) {
            emit requestAudioSourceChange(nextSong->audioSource());
        }
        break;
    }

    case PlayMode::Shuffle: {
        // Simplified shuffle: randomly select a song
        int randomIndex = QRandomGenerator::global()->bounded(m_playlistModel->rowCount());
        AudioInfo* randomSong = m_playlistModel->getAudioInfoAtIndex(randomIndex);
        m_playlistModel->setCurrentSong(randomSong);
        if (randomSong) {
            emit requestAudioSourceChange(randomSong->audioSource());
        }
        break;
    }
    }
}


void PlaylistCoordinator::switchToPreviousSong()
{
    if (!m_playlistModel || m_playlistModel->rowCount() == 0) {
        return;
    }

    PlayMode playMode = m_playlistModel->playMode();
    AudioInfo* currentSong = m_playlistModel->currentSong();

    switch (playMode) {
    case PlayMode::RepeatOne:
    case PlayMode::Loop: {
        if (!currentSong) {
            AudioInfo* lastSong = m_playlistModel->getAudioInfoAtIndex(m_playlistModel->rowCount() - 1);
            m_playlistModel->setCurrentSong(lastSong);
            if (lastSong) {
                emit requestAudioSourceChange(lastSong->audioSource());
            }
            return;
        }

        int currentIndex = -1;
        for (int i = 0; i < m_playlistModel->rowCount(); ++i) {
            if (m_playlistModel->getAudioInfoAtIndex(i) == currentSong) {
                currentIndex = i;
                break;
            }
        }

        int prevIndex = (currentIndex - 1 + m_playlistModel->rowCount()) % m_playlistModel->rowCount();
        AudioInfo* prevSong = m_playlistModel->getAudioInfoAtIndex(prevIndex);
        m_playlistModel->setCurrentSong(prevSong);
        if (prevSong) {
            emit requestAudioSourceChange(prevSong->audioSource());
        }
        break;
    }

    case PlayMode::Shuffle: {
        int randomIndex = QRandomGenerator::global()->bounded(m_playlistModel->rowCount());
        AudioInfo* randomSong = m_playlistModel->getAudioInfoAtIndex(randomIndex);
        m_playlistModel->setCurrentSong(randomSong);
        if (randomSong) {
            emit requestAudioSourceChange(randomSong->audioSource());
        }
        break;
    }
    }
}

void PlaylistCoordinator::switchToAudioByIndex(int index)
{
    if (m_playlistModel && index >= 0 && index < m_playlistModel->rowCount()) {
        AudioInfo* targetSong = m_playlistModel->getAudioInfoAtIndex(index);
        m_playlistModel->setCurrentSong(targetSong);
        if (targetSong) {
            emit requestAudioSourceChange(targetSong->audioSource());
        }
    }
}

void PlaylistCoordinator::addAudio(const QString& title,
                                   const QString& authorName,
                                   const QUrl& audioSource,
                                   const QUrl& imageSource,
                                   const QUrl& videoSource)
{
    m_playlistModel->addAudio(title, authorName, audioSource, imageSource, videoSource);
}

void PlaylistCoordinator::removeAudio(int index)
{
    m_playlistModel->removeAudio(index);
}

void PlaylistCoordinator::clearPlaylist()
{
    m_playlistModel->clearPlaylist();
}
PlaylistModel* PlaylistCoordinator::playlistModel() const
{
    return m_playlistModel;
}

QList<QObject*> PlaylistCoordinator::getPlaylistAudioInfoList() const
{
    QList<QObject*> audioInfoList;
    if (m_playlistModel) {
        for (int i = 0; i < m_playlistModel->rowCount(); ++i) {
            AudioInfo* audioInfo = m_playlistModel->getAudioInfoAtIndex(i);
            if (audioInfo) {
                audioInfoList.append(audioInfo);
            }
        }
    }
    return audioInfoList;
}

void PlaylistCoordinator::handlePlayFinished()
{
    onPlayFinished();
}

void PlaylistCoordinator::onCurrentSongChanged()
{
    emit currentSongChanged();

    AudioInfo* currentSong = m_playlistModel->currentSong();
    if (currentSong && !currentSong->audioSource().isEmpty()) {
        emit requestAudioSourceChange(currentSong->audioSource());
    }
}

void PlaylistCoordinator::onRequestAudioSourceChange(const QUrl &source)
{
    emit requestAudioSourceChange(source);
}

void PlaylistCoordinator::onPlayFinished()
{
    if (!m_playlistModel) {
        return;
    }

    PlayMode playMode = m_playlistModel->playMode();
    switch (playMode) {
    case PlayMode::RepeatOne:
        if (m_playlistModel->currentSong()) {
            emit requestAudioSourceChange(m_playlistModel->currentSong()->audioSource());
        }
        break;

    case PlayMode::Loop:
    case PlayMode::Shuffle:
        switchToNextSong();
        break;
    }

}


QString PlaylistCoordinator::currentPlaylistName() const
{
    return m_currentPlaylistName;
}


void PlaylistCoordinator::onRowsInserted(const QModelIndex &parent, int first, int last)
{
    Q_UNUSED(parent);
    Q_UNUSED(last);

    if (first == 0 && m_playlistModel && m_playlistModel->rowCount() == 1) {
        AudioInfo* currentSong = m_playlistModel->currentSong();
        if (currentSong && !currentSong->audioSource().isEmpty()) {
            emit requestAudioSourceChange(currentSong->audioSource());
        }
    }
}
