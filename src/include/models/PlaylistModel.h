#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QStringList>
#include <QtQml/qqmlregistration.h>
#include <cstddef>
#include <optional>
#include "core/Playlist.h"
#include "models/AudioInfo.h"

using PlayMode = SongPlayer::Core::PlayMode;

class PlayModeWrapper : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(PlayMode)
    QML_UNCREATABLE("PlayMode is only accessible as an enum")

public:
    enum PlayMode {
        Loop = static_cast<int>(SongPlayer::Core::PlayMode::Loop),
        Shuffle = static_cast<int>(SongPlayer::Core::PlayMode::Shuffle),
        RepeatOne = static_cast<int>(SongPlayer::Core::PlayMode::RepeatOne)
    };
    Q_ENUM(PlayMode)
};

class PlaylistModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("PlaylistModel instances are provided by C++")
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
    std::optional<std::size_t> currentIndex() const noexcept;
    int currentSongIndex() const;

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
    std::optional<std::size_t> nextSongIndex(
        std::optional<std::size_t> shuffleIndex = std::nullopt) const noexcept;
    std::optional<std::size_t> previousSongIndex(
        std::optional<std::size_t> shuffleIndex = std::nullopt) const noexcept;
    
    // Diagnostic methods for Task 1
    Q_INVOKABLE void forceRefresh();
    Q_INVOKABLE bool validateModelState() const;
    Q_INVOKABLE QStringList getModelDiagnostics() const;
    Q_INVOKABLE void performModelDiagnostic() const;

signals:
    void currentSongChanged();
    void duplicateAudioSkipped(const QString& title, const QString& reason);
    void playModeChanged();

private:
    QList<AudioInfo*> m_audioList;
    SongPlayer::Core::Playlist m_playlist;
    AudioInfo *m_currentSong = nullptr;
    void syncCoreCurrentSong();
};
