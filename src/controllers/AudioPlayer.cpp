#include "controllers/AudioPlayer.h"
#include <QDebug>

AudioPlayer::AudioPlayer(QObject *parent)
    : QObject{parent}
    , m_audioOutput{new QAudioOutput(this)}
{
    m_mediaPlayer.setAudioOutput(m_audioOutput);

    m_audioOutput->setVolume(m_volume);
    m_audioOutput->setMuted(m_muted);

    connect(&m_mediaPlayer, &QMediaPlayer::durationChanged, this, &AudioPlayer::durationChanged);

    connect(&m_mediaPlayer, &QMediaPlayer::positionChanged, this, [this](qint64 position) {
        emit positionChanged();
    });

    connect(&m_mediaPlayer, &QMediaPlayer::mediaStatusChanged, this, &AudioPlayer::onMediaStatusChanged);

    connect(&m_mediaPlayer, &QMediaPlayer::playbackStateChanged, this, &AudioPlayer::onPlayStateChanged);
}


bool AudioPlayer::playing() const
{
    return m_playing;
}

qint64 AudioPlayer::duration() const
{
    return m_mediaPlayer.duration();
}

qint64 AudioPlayer::position() const
{
    return m_mediaPlayer.position();
}

float AudioPlayer::volume() const
{
    return m_volume;
}

void AudioPlayer::setVolume(float newVolume)
{
    newVolume = qBound(0.0f, newVolume, 1.0f);

    if (!qFuzzyCompare(m_volume, newVolume)) {
        m_volume = newVolume;

        if (!m_muted) {
            m_audioOutput->setVolume(m_volume);
        }

        emit volumeChanged();
    }
}

bool AudioPlayer::isMuted() const
{
    return m_muted;
}

void AudioPlayer::setMuted(bool muted)
{
    if (m_muted != muted) {
        m_muted = muted;
        m_audioOutput->setMuted(m_muted);
        emit mutedChanged();
    }
}

void AudioPlayer::playPause()
{
    if (m_mediaPlayer.playbackState() == QMediaPlayer::PlayingState) {
        m_mediaPlayer.pause();
    } else {
        m_mediaPlayer.play();
    }
}

void AudioPlayer::stop()
{
    m_mediaPlayer.stop();
}

void AudioPlayer::setPosition(qint64 newPosition)
{
    m_mediaPlayer.setPosition(newPosition);
}

void AudioPlayer::setSource(const QUrl &source)
{
    m_mediaPlayer.stop();
    m_isNewMedia = true;  // Mark that we're loading a new media source
    m_mediaPlayer.setSource(source);
}

void AudioPlayer::onMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    if (status == QMediaPlayer::LoadedMedia) {
        if (m_isNewMedia) {
            initializeNewMedia();
            m_isNewMedia = false;
        }

    } else if (status == QMediaPlayer::EndOfMedia) {
        emit playFinished();

    } else if (status == QMediaPlayer::InvalidMedia) {
        qWarning() << "AudioPlayer: Invalid media source detected";
        qDebug() << "AudioPlayer: Media source:" << m_mediaPlayer.source().toString();
        qDebug() << "AudioPlayer: Error string:" << m_mediaPlayer.errorString();
    }
}

void AudioPlayer::initializeNewMedia()
{
    // Set initial position to 100ms to handle audio files with embedded video streams
    // This ensures proper audio playback start for files that may have non-zero start times
    // Only called when a new media source is loaded, not during user position changes
    m_mediaPlayer.setPosition(100);

    if (m_playing) {
        m_mediaPlayer.play();
    }

    emit durationChanged();
    emit positionChanged();
}

void AudioPlayer::onPlayStateChanged(QMediaPlayer::PlaybackState state)
{
    // Centralized state management to avoid race conditions
    // This is the single source of truth for playing state
    bool newPlayingState = (state == QMediaPlayer::PlayingState);

    if (m_playing != newPlayingState) {
        m_playing = newPlayingState;
        emit playingChanged();
        qDebug() << "AudioPlayer: Playing state changed to:" << (m_playing ? "playing" : "stopped/paused");
    }
}

