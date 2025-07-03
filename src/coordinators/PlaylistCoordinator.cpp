// Written by HanQin Chen (cqnuchq@outlook.com) 2025-06-23
#include "coordinators/PlaylistCoordinator.h"
#include "services/PlaylistStorageService.h"
#include "models/PlaylistModel.h"
#include <QRandomGenerator>

PlaylistCoordinator::PlaylistCoordinator(PlaylistModel *playlistModel, QObject *parent)
    : QObject(parent)
    , m_playlistModel(playlistModel)
    , m_storageService(nullptr) // This constructor is for scenarios where playlist persistence is not required.
    , m_currentPlaylistName("Default Playlist")
{
    // Connect signals from the playlist model to handle changes in the current song or playlist structure.
    // These connections are fundamental for updating the UI and managing playback flow.
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
    // Connect signals from the playlist model to handle changes in the current song or playlist structure.
    // These connections are fundamental for updating the UI and managing playback flow.
    connect(m_playlistModel, &PlaylistModel::currentSongChanged,
            this, &PlaylistCoordinator::onCurrentSongChanged);
    connect(m_playlistModel, &PlaylistModel::rowsInserted,
            this, &PlaylistCoordinator::onRowsInserted);

    if (m_storageService) {
        // Connect signals from the storage service to react to playlist save, delete, and rename events.
        // This ensures that the coordinator's internal state and UI are synchronized with persistent storage changes.
        connect(m_storageService, &PlaylistStorageService::playlistSaved,
                this, &PlaylistCoordinator::playlistSaved);
        connect(m_storageService, &PlaylistStorageService::playlistDeleted,
                this, &PlaylistCoordinator::playlistDeleted);
        connect(m_storageService, &PlaylistStorageService::playlistRenamed,
                this, [this](const QString &oldName, const QString &newName) {
                    // Update the current playlist name if the renamed playlist was the active one.
                    // This maintains consistency between the UI and the underlying data.
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
    // Manages the transition to the next song in the playlist based on the current playback mode.
    // This centralizes the logic for sequential, looping, and shuffled playback.
    if (!m_playlistModel || m_playlistModel->rowCount() == 0) {
        // If the playlist is empty, there's no song to switch to.
        return;
    }

    PlayMode playMode = m_playlistModel->playMode();
    AudioInfo* currentSong = m_playlistModel->currentSong();

    switch (playMode) {
    case PlayMode::RepeatOne:
    case PlayMode::Loop: {
        // For RepeatOne and Loop modes, if no song is currently playing, start from the first song.
        // This ensures playback begins predictably when the playlist is initiated or reset.
        if (!currentSong) {
            AudioInfo* firstSong = m_playlistModel->getAudioInfoAtIndex(0);
            m_playlistModel->setCurrentSong(firstSong);
            if (firstSong) {
                emit requestAudioSourceChange(firstSong->audioSource());
            }
            return;
        }

        // Find the index of the current song to determine the next song in sequence.
        // This linear search is acceptable given typical playlist sizes.
        int currentIndex = -1;
        for (int i = 0; i < m_playlistModel->rowCount(); ++i) {
            if (m_playlistModel->getAudioInfoAtIndex(i) == currentSong) {
                currentIndex = i;
                break;
            }
        }

        // Calculate the index of the next song, wrapping around to the beginning of the playlist
        // if the current song is the last one. This implements the looping behavior.
        int nextIndex = (currentIndex + 1) % m_playlistModel->rowCount();
        AudioInfo* nextSong = m_playlistModel->getAudioInfoAtIndex(nextIndex);
        m_playlistModel->setCurrentSong(nextSong);
        if (nextSong) {
            emit requestAudioSourceChange(nextSong->audioSource());
        }
        break;
    }

    case PlayMode::Shuffle: {
        // For Shuffle mode, a random song is selected from the entire playlist.
        // This provides a non-sequential playback experience.
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
    // Manages the transition to the previous song in the playlist based on the current playback mode.
    // This centralizes the logic for sequential, looping, and shuffled playback in reverse.
    if (!m_playlistModel || m_playlistModel->rowCount() == 0) {
        // If the playlist is empty, there's no song to switch to.
        return;
    }

    PlayMode playMode = m_playlistModel->playMode();
    AudioInfo* currentSong = m_playlistModel->currentSong();

    switch (playMode) {
    case PlayMode::RepeatOne:
    case PlayMode::Loop: {
        // For RepeatOne and Loop modes, if no song is currently playing, start from the last song.
        // This ensures playback begins predictably when navigating backward from an empty state.
        if (!currentSong) {
            AudioInfo* lastSong = m_playlistModel->getAudioInfoAtIndex(m_playlistModel->rowCount() - 1);
            m_playlistModel->setCurrentSong(lastSong);
            if (lastSong) {
                emit requestAudioSourceChange(lastSong->audioSource());
            }
            return;
        }

        // Find the index of the current song to determine the previous song in sequence.
        // This linear search is acceptable given typical playlist sizes.
        int currentIndex = -1;
        for (int i = 0; i < m_playlistModel->rowCount(); ++i) {
            if (m_playlistModel->getAudioInfoAtIndex(i) == currentSong) {
                currentIndex = i;
                break;
            }
        }

        // Calculate the index of the previous song, wrapping around to the end of the playlist
        // if the current song is the first one. This implements the looping behavior.
        int prevIndex = (currentIndex - 1 + m_playlistModel->rowCount()) % m_playlistModel->rowCount();
        AudioInfo* prevSong = m_playlistModel->getAudioInfoAtIndex(prevIndex);
        m_playlistModel->setCurrentSong(prevSong);
        if (prevSong) {
            emit requestAudioSourceChange(prevSong->audioSource());
        }
        break;
    }

    case PlayMode::Shuffle: {
        // For Shuffle mode, a random song is selected from the entire playlist.
        // This provides a non-sequential playback experience when navigating backward.
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
    // Ensures that a storage service is available before attempting to save the playlist.
    // This prevents crashes and provides clear feedback if persistence is not configured.
    if (!m_storageService) {
        qWarning() << "Storage service not initialized, cannot save playlist";
        return false;
    }

    // Determine the target playlist name. If an explicit name is provided, use it;
    // otherwise, default to the currently active playlist name.
    QString targetPlaylistName = playlistName.isEmpty() ? m_currentPlaylistName : playlistName;

    // Collect all AudioInfo objects from the current playlist model.
    // This prepares the data for serialization and storage.
    QList<AudioInfo*> audioItems;
    for (int i = 0; i < m_playlistModel->rowCount(); ++i) {
        AudioInfo* audioInfo = m_playlistModel->getAudioInfoAtIndex(i);
        if (audioInfo) {
            audioItems.append(audioInfo);
        }
    }

    // Determine the current playback mode and the index of the currently playing song.
    // This metadata is saved along with the playlist to restore the exact playback state.
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

    // Delegate the actual saving operation to the PlaylistStorageService.
    // This decouples the playlist management logic from the persistence mechanism.
    bool success = m_storageService->savePlaylist(targetPlaylistName, audioItems, playMode, currentIndex);
    if (success && playlistName.isEmpty()) {
        // If the playlist was saved successfully using the default name, update the internal
        // current playlist name to reflect this, ensuring consistency.
        m_currentPlaylistName = targetPlaylistName;
    }

    return success;
}

bool PlaylistCoordinator::loadPlaylist(const QString &playlistName)
{
    // Ensures that a storage service is available before attempting to load a playlist.
    // This prevents crashes and provides clear feedback if persistence is not configured.
    if (!m_storageService) {
        qWarning() << "Storage service not initialized, cannot load playlist";
        return false;
    }

    // Attempt to load the playlist data from the storage service.
    // If the playlist does not exist or is empty, log a warning and return false.
    PlaylistInfo playlistInfo = m_storageService->loadPlaylist(playlistName);
    if (playlistInfo.id == -1 || playlistInfo.audioItems.isEmpty()) {
        qWarning() << "Playlist does not exist or is empty:" << playlistName;
        return false;
    }

    // Clear the current playlist model before loading new items.
    // This ensures that the UI reflects the newly loaded playlist accurately.
    m_playlistModel->clearPlaylist();

    // Add each audio item from the loaded playlist data to the playlist model.
    // This populates the UI with the songs from the loaded playlist.
    for (AudioInfo* audioInfo : std::as_const(playlistInfo.audioItems)) {
        if (audioInfo) {
            m_playlistModel->addAudio(audioInfo->title(), audioInfo->authorName(),
                                      audioInfo->audioSource(), audioInfo->imageSource(),
                                      audioInfo->videoSource());
        }
    }

    // Restore the saved play mode to ensure consistent playback behavior.
    m_playlistModel->setPlayMode(playlistInfo.playMode);

    // Restore the currently playing song and its audio source.
    // This allows the application to resume playback from where it left off in the loaded playlist.
    if (playlistInfo.currentIndex >= 0 && playlistInfo.currentIndex < m_playlistModel->rowCount()) {
        AudioInfo* currentSong = m_playlistModel->getAudioInfoAtIndex(playlistInfo.currentIndex);
        m_playlistModel->setCurrentSong(currentSong);

        if (currentSong && !currentSong->audioSource().isEmpty()) {
            qDebug() << "PlaylistCoordinator: Set audio source when restoring playlist:" << currentSong->title();
            emit requestAudioSourceChange(currentSong->audioSource());
        }
    }

    // Update the internal current playlist name and emit signals to notify other components.
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
    // Ensures that a storage service is available before attempting to delete a playlist.
    // This prevents crashes and provides clear feedback if persistence is not configured.
    if (!m_storageService) {
        qWarning() << "Storage service not initialized, cannot delete playlist";
        return false;
    }

    bool success = m_storageService->deletePlaylist(playlistName);
    if (success) {
        // If the currently active playlist is deleted, switch back to the default playlist.
        // This prevents the application from being in an inconsistent state with a non-existent active playlist.
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
    // This method is invoked when the currently playing audio track finishes.
    // It determines the next action based on the active playback mode (RepeatOne, Loop, Shuffle).
    if (!m_playlistModel) {
        return;
    }

    PlayMode playMode = m_playlistModel->playMode();
    switch (playMode) {
    case PlayMode::RepeatOne:
        // If in 'Repeat One' mode, the current song is re-requested to play again.
        // This ensures continuous playback of the same track.
        if (m_playlistModel->currentSong()) {
            emit requestAudioSourceChange(m_playlistModel->currentSong()->audioSource());
        }
        break;

    case PlayMode::Loop:
    case PlayMode::Shuffle:
        // For 'Loop' and 'Shuffle' modes, the system attempts to switch to the next song.
        // In 'Loop' mode, this will be the next sequential song (wrapping around if at the end).
        // In 'Shuffle' mode, this will be a randomly selected song.
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
