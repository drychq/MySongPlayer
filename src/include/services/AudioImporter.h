#pragma once

#include "core/AudioImport.h"

#include <QList>
#include <QObject>
#include <QString>
#include <QUrl>

#include <memory>

namespace SongPlayer {

class AudioImporter final : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool importing READ importing NOTIFY importingChanged)
    Q_PROPERTY(int importCompleted READ importCompleted NOTIFY importProgressChanged)
    Q_PROPERTY(int importTotal READ importTotal NOTIFY importProgressChanged)

public:
    explicit AudioImporter(QObject* parent = nullptr);
    explicit AudioImporter(
        std::shared_ptr<const Core::IAudioMetadataReader> metadataReader,
        QObject* parent = nullptr);
    ~AudioImporter() override;

    [[nodiscard]] bool importing() const noexcept;
    [[nodiscard]] int importCompleted() const noexcept;
    [[nodiscard]] int importTotal() const noexcept;

    Q_INVOKABLE void importLocalAudio(const QList<QUrl>& fileUrls);
    Q_INVOKABLE void cancelImport();

signals:
    void importingChanged();
    void importProgressChanged();
    void importStarted(int total);
    void importFinished(int imported, int failed, bool canceled);
    void importRejected(const QString& reason);
    void importFailed(const QUrl& source, const QString& reason);

    void audioImported(const QString& title,
                       const QString& authorName,
                       const QUrl& audioSource,
                       const QUrl& imageSource);

private:
    struct ImportSession;

    void handleResult(int resultIndex);
    void handleFinished();
    void resetProgress(int total, int initialFailures);

    std::shared_ptr<const Core::IAudioMetadataReader> m_metadataReader;
    std::unique_ptr<ImportSession> m_session;
    bool m_importing{false};
    int m_importCompleted{0};
    int m_importTotal{0};
    int m_importedCount{0};
    int m_failedCount{0};
};

} // namespace SongPlayer
