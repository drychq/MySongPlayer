#pragma once

#include <QObject>
#include <QUrl>

#include "core/PlayMode.h"

class AudioPlayer;
class AudioInfo;

/**
 * @brief Enhanced AudioCoordinator负责协调音频播放相关的业务逻辑
 * 
 * 增强版的音频播放协调器，封装AudioPlayer的所有功能，提供完整的播放控制、
 * 状态管理和信号机制，专为MVVM架构设计
 */
class AudioCoordinator : public QObject
{
    Q_OBJECT

public:
    explicit AudioCoordinator(AudioPlayer* audioPlayer, QObject* parent = nullptr);
    virtual ~AudioCoordinator() = default;

    // 播放控制接口 - 封装AudioPlayer的所有功能
    void playPause();
    void stop();
    void setPosition(qint64 position);
    void setVolume(float volume);
    void setMuted(bool muted);
    void setSource(const QUrl& source);
    
    // 播放模式管理
    SongPlayer::Core::PlayMode playMode() const;
    void setPlayMode(SongPlayer::Core::PlayMode mode);
    
    // 当前歌曲管理
    void setCurrentSong(AudioInfo* song);
    AudioInfo* currentSong() const;
    
    // 状态查询接口 - 提供完整的状态信息
    bool isPlaying() const;
    qint64 duration() const;
    qint64 position() const;
    float volume() const;
    bool isMuted() const;
    QUrl currentSource() const;
    
    // 播放控制增强方法
    void play();
    void pause();
    bool canPlay() const;
    bool canPause() const;

signals:
    // 播放状态信号 - 完整的状态变化通知
    void playingChanged(bool playing);
    void positionChanged(qint64 position);
    void durationChanged(qint64 duration);
    void volumeChanged(float volume);
    void mutedChanged(bool muted);
    void sourceChanged(const QUrl& source);
    
    // 播放模式信号
    void playModeChanged(SongPlayer::Core::PlayMode mode);
    
    // 歌曲相关信号
    void currentSongChanged(AudioInfo* song);
    void playFinished();
    
    // 播放状态变化信号
    void playStarted();
    void playStopped();
    void playPaused();
    
    // 错误处理信号
    void errorOccurred(const QString& error);
    
    // 请求信号（用于与其他协调器通信）
    void nextSongRequested();
    void previousSongRequested();

public slots:
    void requestNextSong();
    void requestPreviousSong();
    
    // 错误处理槽
    void handleAudioError(const QString& error);

private slots:
    void onAudioPlayerPlayingChanged();
    void onAudioPlayerPositionChanged();
    void onAudioPlayerDurationChanged();
    void onAudioPlayerVolumeChanged();
    void onAudioPlayerMutedChanged();
    void onAudioPlayerPlayFinished();

private:
    AudioPlayer* m_audioPlayer;
    AudioInfo* m_currentSong;
    SongPlayer::Core::PlayMode m_playMode;
    QUrl m_currentSource;
    QString m_lastError;
    
    void connectAudioPlayerSignals();
    void updatePlayingState();
    void setError(const QString& error);
    void clearError();
};
