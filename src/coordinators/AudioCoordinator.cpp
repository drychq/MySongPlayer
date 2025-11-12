#include "coordinators/AudioCoordinator.h"
#include "controllers/AudioPlayer.h"
#include "models/AudioInfo.h"
#include <QDebug>

AudioCoordinator::AudioCoordinator(AudioPlayer* audioPlayer, QObject* parent)
    : QObject(parent)
    , m_audioPlayer(audioPlayer)
    , m_currentSong(nullptr)
    , m_playMode(SongPlayer::SafePlayMode{SongPlayer::PlayMode::Loop})
{
    Q_ASSERT(m_audioPlayer != nullptr);
    connectAudioPlayerSignals();
    clearError();
}

void AudioCoordinator::playPause()
{
    if (!m_audioPlayer) {
        setError("AudioPlayer is not available");
        return;
    }
    
    clearError();
    m_audioPlayer->playPause();
}

void AudioCoordinator::play()
{
    if (!m_audioPlayer) {
        setError("AudioPlayer is not available");
        return;
    }
    
    if (!isPlaying()) {
        clearError();
        m_audioPlayer->playPause();
    }
}

void AudioCoordinator::pause()
{
    if (!m_audioPlayer) {
        setError("AudioPlayer is not available");
        return;
    }
    
    if (isPlaying()) {
        clearError();
        m_audioPlayer->playPause();
    }
}

void AudioCoordinator::stop()
{
    if (!m_audioPlayer) {
        setError("AudioPlayer is not available");
        return;
    }
    
    clearError();
    m_audioPlayer->stop();
    emit playStopped();
}

void AudioCoordinator::setPosition(qint64 position)
{
    if (!m_audioPlayer) {
        setError("AudioPlayer is not available");
        return;
    }
    
    if (position < 0) {
        setError("Invalid position: position cannot be negative");
        return;
    }
    
    clearError();
    m_audioPlayer->setPosition(position);
}

void AudioCoordinator::setVolume(float volume)
{
    if (!m_audioPlayer) {
        setError("AudioPlayer is not available");
        return;
    }
    
    if (volume < 0.0f || volume > 1.0f) {
        setError("Invalid volume: volume must be between 0.0 and 1.0");
        return;
    }
    
    clearError();
    m_audioPlayer->setVolume(volume);
}

void AudioCoordinator::setMuted(bool muted)
{
    if (!m_audioPlayer) {
        setError("AudioPlayer is not available");
        return;
    }
    
    clearError();
    m_audioPlayer->setMuted(muted);
}

void AudioCoordinator::setSource(const QUrl& source)
{
    if (!m_audioPlayer) {
        setError("AudioPlayer is not available");
        return;
    }
    
    if (!source.isValid() && !source.isEmpty()) {
        setError("Invalid audio source URL");
        return;
    }
    
    clearError();
    m_currentSource = source;
    m_audioPlayer->setSource(source);
    emit sourceChanged(source);
}

SongPlayer::SafePlayMode AudioCoordinator::playMode() const
{
    return m_playMode;
}

void AudioCoordinator::setPlayMode(SongPlayer::SafePlayMode mode)
{
    if (m_playMode != mode) {
        m_playMode = mode;
        emit playModeChanged(m_playMode);
    }
}

void AudioCoordinator::setCurrentSong(AudioInfo* song)
{
    if (m_currentSong != song) {
        m_currentSong = song;
        
        if (m_currentSong) {
            setSource(m_currentSong->audioSource());
        } else {
            setSource(QUrl()); 
        
        emit currentSongChanged(m_currentSong);
        }
    }
}

AudioInfo* AudioCoordinator::currentSong() const
{
    return m_currentSong;
}

bool AudioCoordinator::isPlaying() const
{
    return m_audioPlayer ? m_audioPlayer->playing() : false;
}

qint64 AudioCoordinator::duration() const
{
    return m_audioPlayer ? m_audioPlayer->duration() : 0;
}

qint64 AudioCoordinator::position() const
{
    return m_audioPlayer ? m_audioPlayer->position() : 0;
}

float AudioCoordinator::volume() const
{
    return m_audioPlayer ? m_audioPlayer->volume() : 0.0f;
}

bool AudioCoordinator::isMuted() const
{
    return m_audioPlayer ? m_audioPlayer->isMuted() : false;
}

QUrl AudioCoordinator::currentSource() const
{
    return m_currentSource;
}

bool AudioCoordinator::canPlay() const
{
    return m_audioPlayer && !m_currentSource.isEmpty() && !isPlaying();
}

bool AudioCoordinator::canPause() const
{
    return m_audioPlayer && isPlaying();
}

void AudioCoordinator::requestNextSong()
{
    clearError();
    emit nextSongRequested();
}

void AudioCoordinator::requestPreviousSong()
{
    clearError();
    emit previousSongRequested();
}

void AudioCoordinator::handleAudioError(const QString& error)
{
    setError(error);
    qWarning() << "AudioCoordinator error:" << error;
}

void AudioCoordinator::onAudioPlayerPlayingChanged()
{
    bool playing = isPlaying();
    emit playingChanged(playing);
    
    if (playing) {
        emit playStarted();
    } else {
        emit playPaused();
    }
    
    updatePlayingState();
}

void AudioCoordinator::onAudioPlayerPositionChanged()
{
    emit positionChanged(position());
}

void AudioCoordinator::onAudioPlayerDurationChanged()
{
    emit durationChanged(duration());
}

void AudioCoordinator::onAudioPlayerVolumeChanged()
{
    emit volumeChanged(volume());
}

void AudioCoordinator::onAudioPlayerMutedChanged()
{
    emit mutedChanged(isMuted());
}

void AudioCoordinator::onAudioPlayerPlayFinished()
{
    emit playFinished();
    emit playStopped();
}

void AudioCoordinator::connectAudioPlayerSignals()
{
    if (!m_audioPlayer) return;
    
    connect(m_audioPlayer, &AudioPlayer::playingChanged,
            this, &AudioCoordinator::onAudioPlayerPlayingChanged);
    
    connect(m_audioPlayer, &AudioPlayer::positionChanged,
            this, &AudioCoordinator::onAudioPlayerPositionChanged);
    
    connect(m_audioPlayer, &AudioPlayer::durationChanged,
            this, &AudioCoordinator::onAudioPlayerDurationChanged);
    
    connect(m_audioPlayer, &AudioPlayer::volumeChanged,
            this, &AudioCoordinator::onAudioPlayerVolumeChanged);
    
    connect(m_audioPlayer, &AudioPlayer::mutedChanged,
            this, &AudioCoordinator::onAudioPlayerMutedChanged);
    
    connect(m_audioPlayer, &AudioPlayer::playFinished,
            this, &AudioCoordinator::onAudioPlayerPlayFinished);
}

void AudioCoordinator::updatePlayingState()
{
    
}

void AudioCoordinator::setError(const QString& error)
{
    if (m_lastError != error) {
        m_lastError = error;
        emit errorOccurred(error);
    }
}

void AudioCoordinator::clearError()
{
    if (!m_lastError.isEmpty()) {
        m_lastError.clear();
    }
}
