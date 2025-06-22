#pragma once

#include <QAbstractListModel>
#include <QRandomGenerator>
#include <QtQml/qqmlregistration.h>
#include "models/AudioInfo.h"

enum class PlayMode {
    Loop,
    Shuffle,
    RepeatOne
};

class PlayModeWrapper : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("PlayMode is only accessible as an enum")

public:
    enum PlayMode {
        Loop = static_cast<int>(::PlayMode::Loop),
        Shuffle = static_cast<int>(::PlayMode::Shuffle),
        RepeatOne = static_cast<int>(::PlayMode::RepeatOne)
    };
    Q_ENUM(PlayMode)
};

class PlaylistModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(AudioInfo* currentSong READ currentSong WRITE setCurrentSong NOTIFY currentSongChanged)

public:
    enum Role {
        AudioTitleRole = Qt::UserRole + 1,
        AudioAuthorNameRole,
        AudioSourceRole,
        AudioImageSourceRole,
        AudioVideoSourceRole
    };
    Q_ENUM(Role)

    explicit PlaylistModel(QObject *parent = nullptr);
    virtual ~PlaylistModel();

    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    virtual QVariant data(const QModelIndex &index, int role) const override;
    virtual QHash<int, QByteArray> roleNames() const override;

    AudioInfo *currentSong() const;
    void setCurrentSong(AudioInfo *newCurrentSong);

    PlayMode playMode() const;
    void setPlayMode(PlayMode newMode);

    Q_INVOKABLE void addAudio(const QString& title,
                              const QString& authorName,
                              const QUrl& audioSource,
                              const QUrl& imageSource,
                              const QUrl& videoSource = QUrl());
    Q_INVOKABLE void removeAudio(int index);
    Q_INVOKABLE void clearPlaylist();

    Q_INVOKABLE AudioInfo* getAudioInfoAtIndex(int index) const;

    bool isDuplicateAudio(const QUrl& audioSource) const;

signals:
    void currentSongChanged();
    void duplicateAudioSkipped(const QString& title, const QString& reason);
    void playModeChanged();

private:
    QList<AudioInfo*> m_audioList;
    AudioInfo *m_currentSong = nullptr;
    PlayMode m_playMode = PlayMode::Loop;
    QList<int> m_shuffleIndices;  // Index sequence for shuffle mode
    int m_currentShuffleIndex = -1;  // Current position in shuffle sequence
    void generateShuffleSequence();
};


