// Written by HanQin Chen (cqnuchq@outlook.com) 2025-06-22
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
    , m_lyricsService{new LyricsService(this)}
    , m_lyricsModel{new LyricsModel(this)}
    , m_playlistStorageService(new PlaylistStorageService(this))
{
    if (!m_playlistStorageService->initialize()) {
        qCritical() << "Playlist storage service initialization failed:" << m_playlistStorageService->lastError();
    }

    auto* playlistCoordinator = new PlaylistCoordinator(m_playlistModel, m_playlistStorageService, this);
    m_currentSongManager = qobject_cast<ICurrentSongManager*>(playlistCoordinator);
    m_playlistOperations = qobject_cast<IPlaylistOperations*>(playlistCoordinator);
    m_playlistPersistence = qobject_cast<IPlaylistPersistence*>(playlistCoordinator);

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

    connect(m_playlistModel, &PlaylistModel::playModeChanged,
            this, &PlayerController::playModeChanged);

    connect(m_audioImporter, &AudioImporter::audioImported,
            this, &PlayerController::onAudioImported);

    connect(m_playlistModel, &PlaylistModel::duplicateAudioSkipped,
            this, &PlayerController::duplicateAudioSkipped);

    connect(m_playlistModel, &PlaylistModel::rowsInserted,
            this, &PlayerController::onPlaylistChanged);
    connect(m_playlistModel, &PlaylistModel::rowsRemoved,
            this, &PlayerController::onPlaylistChanged);
    connect(m_playlistModel, &PlaylistModel::currentSongChanged,
            this, &PlayerController::onPlaylistChanged);
    connect(m_playlistModel, &PlaylistModel::playModeChanged,
            this, &PlayerController::onPlaylistChanged);

    loadDefaultPlaylistOnStartup();
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

PlayMode PlayerController::playMode() const
{
    return m_playlistModel->playMode();
}

void PlayerController::setPlayMode(PlayMode newMode)
{
    m_playlistModel->setPlayMode(newMode);
}

int PlayerController::playModeInt() const
{
    return static_cast<int>(m_playlistModel->playMode());
}

void PlayerController::setPlayModeInt(int newMode)
{
    m_playlistModel->setPlayMode(static_cast<PlayMode>(newMode));
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

bool PlayerController::saveCurrentPlaylist(const QString &playlistName)
{
    return m_playlistPersistence->saveCurrentPlaylist(playlistName);
}

bool PlayerController::loadPlaylist(const QString &playlistName)
{
    return m_playlistPersistence->loadPlaylist(playlistName);
}

QStringList PlayerController::getAllPlaylistNames() const
{
    return m_playlistPersistence->getAllPlaylistNames();
}

bool PlayerController::deletePlaylist(const QString &playlistName)
{
    return m_playlistPersistence->deletePlaylist(playlistName);
}

bool PlayerController::renamePlaylist(const QString &oldName, const QString &newName)
{
    return m_playlistPersistence->renamePlaylist(oldName, newName);
}

QString PlayerController::currentPlaylistName() const
{
    return m_playlistPersistence->currentPlaylistName();
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

        if (!audioFilePath.isEmpty()) {
            QList<LyricsService::LyricLine> lyrics = m_lyricsService->parseLrcFile(audioFilePath);
            m_lyricsModel->setLyrics(lyrics);

            qDebug() << "PlayerController: Song switched, loading lyrics:" << currentSong->title()
                     << "lyrics lines:" << lyrics.size()
                     << "showLyrics initial state:" << m_lyricsModel->showLyrics();
        }
    }
}

void PlayerController::onPositionChanged()
{
    qint64 currentPosition = m_audioPlayer->position();
    m_lyricsModel->updateCurrentPosition(currentPosition);
}

LyricsModel* PlayerController::lyricsModel() const
{
    return m_lyricsModel;
}

void PlayerController::onPlaylistChanged()
{
    static QTimer* saveTimer = nullptr;
    if (!saveTimer) {
        saveTimer = new QTimer(this);
        saveTimer->setSingleShot(true);
        saveTimer->setInterval(2000);
        connect(saveTimer, &QTimer::timeout, this, [this]() {
            bool success = m_playlistPersistence->saveCurrentPlaylist("Default Playlist");
            if (success) {
                qDebug() << "PlayerController: Playlist auto-save successful";
            } else {
                qWarning() << "PlayerController: Playlist auto-save failed:" << m_playlistPersistence->storageService()->lastError();
            }
        });
    }

    saveTimer->start();
}

void PlayerController::loadDefaultPlaylistOnStartup()
{
    QTimer::singleShot(100, this, [this]() {
        bool success = m_playlistPersistence->loadPlaylist("Default Playlist");
        if (success) {
            qInfo() << "Default playlist loaded successfully";
        } else {
            qDebug() << "Default playlist not found or empty, will create new default playlist";
        }
    });
}

