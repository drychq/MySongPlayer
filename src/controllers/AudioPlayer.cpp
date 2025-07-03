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
        // To prevent unnecessary updates and signal emissions due to floating-point inaccuracies,
        // ensuring that volume changes are only processed when a significant difference is detected.
        m_volume = newVolume;

        if (!m_muted) {
            // To ensure that the audio output volume is only adjusted if the player is not currently muted.
            // This maintains the mute state independently of volume level changes.
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
    m_isNewMedia = true;  // To differentiate between a new media load and a simple position seek,
                          // ensuring that `initializeNewMedia` is only called for genuinely new tracks.
    m_mediaPlayer.setSource(source);
}

void AudioPlayer::onMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    if (status == QMediaPlayer::LoadedMedia) {
        // To ensure that media-specific initialization (like setting initial position) occurs
        // only when a new media source has been successfully loaded, not on subsequent status changes.
        if (m_isNewMedia) {
            initializeNewMedia();
            m_isNewMedia = false;
        }

    } else if (status == QMediaPlayer::EndOfMedia) {
        // To signal that the current track has finished playback, allowing external components
        // (e.g., playlist manager) to advance to the next song or handle repeat logic.
        emit playFinished();

    } else if (status == QMediaPlayer::InvalidMedia) {
        // To provide clear diagnostic information when a media source cannot be played,
        // aiding in debugging and user feedback regarding unsupported or corrupted files.
        qWarning() << "AudioPlayer: Invalid media source detected";
        qDebug() << "AudioPlayer: Media source:" << m_mediaPlayer.source().toString();
        qDebug() << "AudioPlayer: Error string:" << m_mediaPlayer.errorString();
    }
}

void AudioPlayer::initializeNewMedia()
{
    // Workaround for QMediaPlayer: Setting a small initial position (100ms) helps ensure
    // proper audio playback initiation for certain media formats that might have non-zero
    // start times or embedded video streams, preventing initial silence or glitches.
    m_mediaPlayer.setPosition(100);

    if (m_playing) {
        m_mediaPlayer.play();
    }

    emit durationChanged();
    emit positionChanged();
}

void AudioPlayer::onPlayStateChanged(QMediaPlayer::PlaybackState state)
{
    // Centralized state management: This method acts as the single source of truth for the 'playing' state,
    // preventing potential race conditions and ensuring consistency across the application.
    bool newPlayingState = (state == QMediaPlayer::PlayingState);

    if (m_playing != newPlayingState) {
        m_playing = newPlayingState;
        emit playingChanged();
        // Log state changes for debugging and monitoring playback flow.
        qDebug() << "AudioPlayer: Playing state changed to:" << (m_playing ? "playing" : "stopped/paused");
    }
}

