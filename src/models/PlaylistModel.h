#pragma once

#include <QAbstractListModel>
#include <QRandomGenerator>
#include <QtQml/qqmlregistration.h>
#include "models/AudioInfo.h"

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

private:
    QList<AudioInfo*> m_audioList;
    AudioInfo *m_currentSong = nullptr;
};


