// Written by HanQin Chen (cqnuchq@outlook.com) 2025-06-23
#include "coordinators/PlaylistCoordinator.h"
#include "services/PlaylistStorageService.h"
#include "models/PlaylistModel.h"
#include <QRandomGenerator>

PlaylistCoordinator::PlaylistCoordinator(PlaylistModel *playlistModel, QObject *parent)
    : QObject(parent)
    , m_playlistModel(playlistModel)
    , m_storageService(nullptr)
    , m_currentPlaylistName("Default Playlist")
{
    connect(m_playlistModel, &PlaylistModel::currentSongChanged,
            this, &PlaylistCoordinator::onCurrentSongChanged);
    connect(m_playlistModel, &PlaylistModel::rowsInserted,
            this, &PlaylistCoordinator::onRowsInserted);
}

PlaylistCoordinator::PlaylistCoordinator(PlaylistModel *playlistModel, PlaylistStorageService *storageService, QObject *parent)
    : QObject(parent)
    , m_playlistModel(playlistModel)
    , m_storageService(storageService)
    , m_currentPlaylistName("Default Playlist")
{
    connect(m_playlistModel, &PlaylistModel::currentSongChanged,
            this, &PlaylistCoordinator::onCurrentSongChanged);
    connect(m_playlistModel, &PlaylistModel::rowsInserted,
            this, &PlaylistCoordinator::onRowsInserted);

    if (m_storageService) {
        connect(m_storageService, &PlaylistStorageService::playlistSaved,
                this, &PlaylistCoordinator::playlistSaved);
        connect(m_storageService, &PlaylistStorageService::playlistDeleted,
                this, &PlaylistCoordinator::playlistDeleted);
        connect(m_storageService, &PlaylistStorageService::playlistRenamed,
                this, [this](const QString &oldName, const QString &newName) {
                    if (m_currentPlaylistName == oldName) {
                        m_currentPlaylistName = newName;
                        emit currentPlaylistChanged(newName);
                    }
                    emit playlistRenamed(oldName, newName);
                });
    }
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

bool PlaylistCoordinator::saveCurrentPlaylist(const QString &playlistName)
{
    if (!m_storageService) {
        qWarning() << "Storage service not initialized, cannot save playlist";
        return false;
    }

    QString targetPlaylistName = playlistName.isEmpty() ? m_currentPlaylistName : playlistName;

    // Get current playlist data
    QList<AudioInfo*> audioItems;
    for (int i = 0; i < m_playlistModel->rowCount(); ++i) {
        AudioInfo* audioInfo = m_playlistModel->getAudioInfoAtIndex(i);
        if (audioInfo) {
            audioItems.append(audioInfo);
        }
    }

    // Get current play mode and index
    PlayMode playMode = m_playlistModel->playMode();
    int currentIndex = -1;
    if (m_playlistModel->currentSong()) {
        for (int i = 0; i < audioItems.size(); ++i) {
            if (audioItems[i] == m_playlistModel->currentSong()) {
                currentIndex = i;
                break;
            }
        }
    }

    bool success = m_storageService->savePlaylist(targetPlaylistName, audioItems, playMode, currentIndex);
    if (success && playlistName.isEmpty()) {
        // Update current playlist name if saved with default name successfully
        m_currentPlaylistName = targetPlaylistName;
    }

    return success;
}

bool PlaylistCoordinator::loadPlaylist(const QString &playlistName)
{
    if (!m_storageService) {
        qWarning() << "Storage service not initialized, cannot load playlist";
        return false;
    }

    PlaylistInfo playlistInfo = m_storageService->loadPlaylist(playlistName);
    if (playlistInfo.id == -1 || playlistInfo.audioItems.isEmpty()) {
        qWarning() << "Playlist does not exist or is empty:" << playlistName;
        return false;
    }

    // Clear current playlist
    m_playlistModel->clearPlaylist();

    // Add audio items
    for (AudioInfo* audioInfo : std::as_const(playlistInfo.audioItems)) {
        if (audioInfo) {
            m_playlistModel->addAudio(audioInfo->title(), audioInfo->authorName(),
                                      audioInfo->audioSource(), audioInfo->imageSource(),
                                      audioInfo->videoSource());
        }
    }

    // Restore play mode
    m_playlistModel->setPlayMode(playlistInfo.playMode);

    // Restore current song
    if (playlistInfo.currentIndex >= 0 && playlistInfo.currentIndex < m_playlistModel->rowCount()) {
        AudioInfo* currentSong = m_playlistModel->getAudioInfoAtIndex(playlistInfo.currentIndex);
        m_playlistModel->setCurrentSong(currentSong);

        if (currentSong && !currentSong->audioSource().isEmpty()) {
            qDebug() << "PlaylistCoordinator: Set audio source when restoring playlist:" << currentSong->title();
            emit requestAudioSourceChange(currentSong->audioSource());
        }
    }

    // Update current playlist name
    m_currentPlaylistName = playlistName;

    emit currentPlaylistChanged(playlistName);
    emit playlistLoaded(playlistName);
    qDebug() << "PlaylistCoordinator: Playlist loaded successfully:" << playlistName;
    return true;
}

QStringList PlaylistCoordinator::getAllPlaylistNames() const
{
    if (!m_storageService) {
        return QStringList();
    }

    return m_storageService->getAllPlaylistNames();
}

bool PlaylistCoordinator::deletePlaylist(const QString &playlistName)
{
    if (!m_storageService) {
        qWarning() << "Storage service not initialized, cannot delete playlist";
        return false;
    }

    bool success = m_storageService->deletePlaylist(playlistName);
    if (success) {
        // Switch to default playlist if deleting current playlist
        if (m_currentPlaylistName == playlistName) {
            m_currentPlaylistName = "Default Playlist";
            emit currentPlaylistChanged(m_currentPlaylistName);
        }
    }

    return success;
}

bool PlaylistCoordinator::renamePlaylist(const QString &oldName, const QString &newName)
{
    if (!m_storageService) {
        qWarning() << "Storage service not initialized, cannot rename playlist";
        return false;
    }

    return m_storageService->renamePlaylist(oldName, newName);
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

PlaylistStorageService *PlaylistCoordinator::storageService() const
{
    return m_storageService;
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
