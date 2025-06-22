#pragma once

#include <QObject>
#include <QMediaPlayer>
#include <QAudioOutput>

class AudioPlayer : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool playing READ playing NOTIFY playingChanged)
    Q_PROPERTY(qint64 duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(qint64 poistion READ position NOTIFY positionChanged)
    Q_PROPERTY(float volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(bool muted READ isMuted WRITE setMuted NOTIFY mutedChanged)

public:
    explicit AudioPlayer(QObject *parent = nullptr);

    bool playing() const;

    qint64 duration() const;
    qint64 position() const;

    float volume() const;
    void setVolume(float newVolume);
    bool isMuted() const;
    void setMuted(bool muted);

    void playPause();
    void stop();
    void setPosition(qint64 newPosition);
    void setSource(const QUrl &source);

signals:
    void playingChanged();
    void positionChanged();
    void durationChanged();
    void volumeChanged();
    void mutedChanged();
    void playFinished();

private slots:
    void onMediaStatusChanged(QMediaPlayer::MediaStatus status);
    void onPlayStateChanged(QMediaPlayer::PlaybackState state);

private:
    void initializeNewMedia();

    QMediaPlayer m_mediaPlayer;
    QAudioOutput *m_audioOutput;
    bool m_playing = false;
    bool m_isNewMedia = false;
    float m_volume = 0.5f; // Initial volume set to 50%
    bool m_muted = false;
};
