#pragma once

#include <QMediaPlayer>
#include <QAbstractListModel>
#include <QtQml/qqmlregistration.h>
#include <QGuiApplication>
#include <QQmlEngine>
#include <QJSEngine>

class AudioInfo;

class PlayerController : public QAbstractListModel
{
    Q_OBJECT
    QML_SINGLETON
    QML_NAMED_ELEMENT(PlayerController)

    Q_PROPERTY(bool playing READ playing NOTIFY playingChanged)
    Q_PROPERTY(AudioInfo* currentSong READ currentSong WRITE setCurrentSong NOTIFY currentSongChanged)

public:
    static QObject* provider(QQmlEngine *engine, QJSEngine *scriptEngine) {
        Q_UNUSED(engine); Q_UNUSED(scriptEngine);
        auto controller = new PlayerController(QGuiApplication::instance());
        return controller;
    }

    enum Role {
        AudioTitleRole = Qt::UserRole + 1,
        AudioAuthorNameRole,
        AudioSourceRole,
        AudioImageSourceRole,
        AudioVideoSourceRole
    };

    explicit PlayerController(QObject *parent = nullptr);

    bool playing() const;

    Q_INVOKABLE void switchToNextSong();

    virtual int rowCount(const QModelIndex &parent) const override;
    virtual QVariant data(const QModelIndex &index, int role) const override;
    virtual QHash<int, QByteArray> roleNames() const override;

    AudioInfo *currentSong() const;
    void setCurrentSong(AudioInfo *newCurrentSong);

public slots:
    void switchToPreviousSong();
    void playPause();
    void changeAudioSource(const QUrl &source);
    void addAudio(const QString& title,
                  const QString& authorName,
                  const QUrl& audioSource,
                  const QUrl& imageSource,
                  const QUrl& videoSource = QUrl());
    void removeAudio(int index);
    void switchToAudioByIndex(int index);

signals:
    void playingChanged();

    void currentSongChanged();

private:
    bool m_playing = false;
    QMediaPlayer m_mediaPlayer;

    QList<AudioInfo*> m_audioList;
    AudioInfo *m_currentSong = nullptr;
};


