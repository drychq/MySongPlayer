#pragma once

#include <QObject>
#include <QUrl>

class AudioInfo;

class ICurrentSongManager
{
public:
    virtual ~ICurrentSongManager() = default;
    virtual AudioInfo* currentSong() const = 0;
    virtual void setCurrentSong(AudioInfo *song) = 0;
    virtual void switchToNextSong() = 0;
    virtual void switchToPreviousSong() = 0;
    virtual void switchToAudioByIndex(int index) = 0;
    virtual void handlePlayFinished() = 0;

};

Q_DECLARE_INTERFACE(ICurrentSongManager, "com.songplayer.ICurrentSongManager")
