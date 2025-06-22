#include "controllers/PlayerController.h"
#include "controllers/AudioPlayer.h"
#include "coordinators/PlaylistCoordinator.h"
#include "interfaces/IPlaylistOperations.h"
#include "services/AudioImporter.h"
#include <QTimer>

PlayerController::PlayerController(QObject *parent)
    : QObject{parent}
    , m_audioPlayer{new AudioPlayer(this)}
    , m_playlistModel{new PlaylistModel(this)}
    , m_audioImporter{new AudioImporter(this)}
{
    auto* playlistCoordinator = new PlaylistCoordinator(m_playlistModel, this);
    m_currentSongManager = qobject_cast<ICurrentSongManager*>(playlistCoordinator);
    m_playlistOperations = qobject_cast<IPlaylistOperations*>(playlistCoordinator);

    connect(playlistCoordinator, &PlaylistCoordinator::requestAudioSourceChange,
            this, &PlayerController::onAudioSourceChangeRequested);
    connect(playlistCoordinator, &PlaylistCoordinator::currentSongChanged,
            this, &PlayerController::onCurrentSongChanged);
    connect(playlistCoordinator, &PlaylistCoordinator::currentSongChanged,
            this, &PlayerController::currentSongChanged);

    connect(m_audioPlayer, &AudioPlayer::playFinished,
            playlistCoordinator, &::PlaylistCoordinator::handlePlayFinished);
    connect(m_audioPlayer, &AudioPlayer::playingChanged,
            this, &PlayerController::playingChanged);
    connect(m_audioPlayer, &AudioPlayer::positionChanged,
            this, &PlayerController::positionChanged);
    connect(m_audioPlayer, &AudioPlayer::durationChanged,
            this, &PlayerController::durationChanged);
    connect(m_audioPlayer, &AudioPlayer::volumeChanged,
            this, &PlayerController::volumeChanged);
    connect(m_audioPlayer, &AudioPlayer::mutedChanged,
            this, &PlayerController::mutedChanged);
    connect(m_audioPlayer, &AudioPlayer::positionChanged,
            this, &PlayerController::onPositionChanged);

    connect(m_audioImporter, &AudioImporter::audioImported,
            this, &PlayerController::onAudioImported);

    connect(m_playlistModel, &PlaylistModel::duplicateAudioSkipped,
            this, &PlayerController::duplicateAudioSkipped);

}

bool PlayerController::playing() const
{
    return m_audioPlayer->playing();
}

qint64 PlayerController::duration() const
{
    return m_audioPlayer->duration();
}

qint64 PlayerController::position() const
{
    return m_audioPlayer->position();
}

float PlayerController::volume() const
{
    return m_audioPlayer->volume();
}

void PlayerController::setVolume(float newVolume)
{
    m_audioPlayer->setVolume(newVolume);
}

bool PlayerController::isMuted() const
{
    return m_audioPlayer->isMuted();
}

void PlayerController::setMuted(bool muted)
{
    m_audioPlayer->setMuted(muted);
}

void PlayerController::playPause()
{
    m_audioPlayer->playPause();
}

void PlayerController::setPosition(qint64 newPosition)
{
    m_audioPlayer->setPosition(newPosition);
}

AudioInfo* PlayerController::currentSong() const
{
    return m_currentSongManager->currentSong();
}

void PlayerController::setCurrentSong(AudioInfo *newCurrentSong)
{
    m_currentSongManager->setCurrentSong(newCurrentSong);
}

void PlayerController::switchToNextSong()
{
    m_currentSongManager->switchToNextSong();
}

void PlayerController::switchToPreviousSong()
{
    m_currentSongManager->switchToPreviousSong();
}

void PlayerController::switchToAudioByIndex(int index)
{
    m_currentSongManager->switchToAudioByIndex(index);
}

void PlayerController::addAudio(const QString &title, const QString &authorName,
                                const QUrl &audioSource, const QUrl &imageSource,
                                const QUrl &videoSource)
{
    m_playlistOperations->addAudio(title, authorName, audioSource, imageSource, videoSource);
}

void PlayerController::removeAudio(int index)
{
    m_playlistOperations->removeAudio(index);
}

void PlayerController::clearPlaylist()
{
    m_playlistOperations->clearPlaylist();
}

QList<QObject*> PlayerController::getPlaylistAudioInfoList() const
{
    return m_playlistOperations->getPlaylistAudioInfoList();
}

void PlayerController::importLocalAudio(const QList<QUrl> &fileUrls)
{
    qDebug() << "PlayerController: Starting local audio import for" << fileUrls.size() << "files";
    m_audioImporter->importLocalAudio(fileUrls);
}

void PlayerController::addNetworkAudio(const QString &title, const QString &authorName,
                                       const QUrl &audioSource, const QUrl &imageSource)
{
    m_playlistOperations->addAudio(title, authorName, audioSource, imageSource);
    int lastIndex = m_playlistModel->rowCount() - 1;
    switchToAudioByIndex(lastIndex);
    qDebug() << "PlayerController: Added network audio to playlist:" << title << "by" << authorName;
}

PlaylistModel *PlayerController::playlistModel() const
{
    return m_playlistOperations->playlistModel();
}

void PlayerController::onAudioSourceChangeRequested(const QUrl &source)
{
    qDebug() << "PlayerController: Setting audio source to:" << source.fileName();
    m_audioPlayer->setSource(source);
    emit positionChanged();
}

void PlayerController::onAudioImported(const QString &title, const QString &authorName,
                                       const QUrl &audioSource, const QUrl &imageSource)
{
    qDebug() << "PlayerController: Audio imported successfully -" << title << "by" << authorName;
    addAudio(title, authorName, audioSource, imageSource);
}

void PlayerController::onCurrentSongChanged()
{
    if (m_currentSongManager->currentSong() == nullptr) {
        m_audioPlayer->stop();
    } else {
        AudioInfo* currentSong = m_currentSongManager->currentSong();
        QString audioFilePath = currentSong->audioSource().toLocalFile();
    }
}

void PlayerController::onPositionChanged()
{
    qint64 currentPosition = m_audioPlayer->position();
}

