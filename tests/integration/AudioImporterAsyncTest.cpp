#include "core/AudioImport.h"
#include "services/AudioImporter.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QThread>
#include <QTimer>
#include <QUrl>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

namespace {

class SlowMetadataReader final : public SongPlayer::Core::IAudioMetadataReader {
public:
    explicit SlowMetadataReader(QThread* guiThread)
        : m_guiThread(guiThread)
    {}

    SongPlayer::Core::AudioImportResult read(
        const SongPlayer::Core::AudioImportRequest& request) const noexcept override
    {
        m_ranOutsideGuiThread.store(
            QThread::currentThread() != m_guiThread, std::memory_order_relaxed);
        std::this_thread::sleep_for(std::chrono::milliseconds(120));
        return SongPlayer::Core::ImportedAudio{
            .title = request.audioFile.stem().string(),
            .artist = "Test Artist",
            .audioFile = request.audioFile,
            .coverFile = std::nullopt,
        };
    }

    [[nodiscard]] bool ranOutsideGuiThread() const noexcept
    {
        return m_ranOutsideGuiThread.load(std::memory_order_relaxed);
    }

private:
    QThread* m_guiThread;
    mutable std::atomic_bool m_ranOutsideGuiThread = false;
};

int failures = 0;

void expect(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        ++failures;
    }
}

void waitForImport(QEventLoop& loop, bool& timedOut)
{
    QTimer timeout;
    timeout.setSingleShot(true);
    QObject::connect(&timeout, &QTimer::timeout, &loop, [&] {
        timedOut = true;
        loop.quit();
    });
    timeout.start(std::chrono::seconds(3));
    loop.exec();
}

void verifiesNonBlockingImport(QCoreApplication& application)
{
    auto reader = std::make_shared<SlowMetadataReader>(application.thread());
    SongPlayer::AudioImporter importer(reader);

    QEventLoop loop;
    bool timedOut = false;
    bool heartbeatObserved = false;
    bool callbackOnGuiThread = false;
    bool rejectedConcurrentRequest = false;
    int imported = -1;
    int failed = -1;
    bool canceled = true;

    QObject::connect(&importer, &SongPlayer::AudioImporter::audioImported,
                     &application, [&](const QString&, const QString&, const QUrl&, const QUrl&) {
        callbackOnGuiThread = QThread::currentThread() == application.thread();
    });
    QObject::connect(&importer, &SongPlayer::AudioImporter::importRejected,
                     &application, [&](const QString&) { rejectedConcurrentRequest = true; });
    QObject::connect(&importer, &SongPlayer::AudioImporter::importFinished,
                     &application, [&](int importedCount, int failedCount, bool wasCanceled) {
        imported = importedCount;
        failed = failedCount;
        canceled = wasCanceled;
        loop.quit();
    });

    QElapsedTimer entryTimer;
    entryTimer.start();
    importer.importLocalAudio({QUrl::fromLocalFile(QStringLiteral("/virtual/first.mp3"))});
    const qint64 entryDurationMs = entryTimer.elapsed();
    importer.importLocalAudio({QUrl::fromLocalFile(QStringLiteral("/virtual/rejected.mp3"))});
    QTimer::singleShot(std::chrono::milliseconds(10), &application,
                       [&] { heartbeatObserved = true; });

    waitForImport(loop, timedOut);

    expect(!timedOut, "asynchronous import completes before timeout");
    expect(entryDurationMs < 50, "import entry point returns without waiting for metadata I/O");
    expect(heartbeatObserved, "GUI event loop remains responsive while metadata is read");
    expect(reader->ranOutsideGuiThread(), "metadata reader runs outside the GUI thread");
    expect(callbackOnGuiThread, "Qt-facing result callback runs on the GUI thread");
    expect(rejectedConcurrentRequest, "a concurrent batch is explicitly rejected");
    expect(imported == 1 && failed == 0 && !canceled,
           "successful batch reports a consistent summary");
    expect(importer.importCompleted() == 1 && importer.importTotal() == 1,
           "progress reaches the batch total");
}

void verifiesCooperativeCancellation(QCoreApplication& application)
{
    auto reader = std::make_shared<SlowMetadataReader>(application.thread());
    SongPlayer::AudioImporter importer(reader);

    QEventLoop loop;
    bool timedOut = false;
    bool canceled = false;
    int delivered = 0;

    QObject::connect(&importer, &SongPlayer::AudioImporter::audioImported,
                     &application, [&](const QString&, const QString&, const QUrl&, const QUrl&) {
        if (++delivered == 1) {
            importer.cancelImport();
        }
    });
    QObject::connect(&importer, &SongPlayer::AudioImporter::importFinished,
                     &application, [&](int, int, bool wasCanceled) {
        canceled = wasCanceled;
        loop.quit();
    });

    importer.importLocalAudio({
        QUrl::fromLocalFile(QStringLiteral("/virtual/one.mp3")),
        QUrl::fromLocalFile(QStringLiteral("/virtual/two.mp3")),
        QUrl::fromLocalFile(QStringLiteral("/virtual/three.mp3")),
    });
    waitForImport(loop, timedOut);

    expect(!timedOut, "canceled import reaches a terminal state");
    expect(canceled, "finished summary identifies cooperative cancellation");
    expect(delivered >= 1 && delivered < 3,
           "cancel keeps completed results and stops remaining files");
    expect(!importer.importing(), "importer leaves the busy state after cancellation");
}

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication application(argc, argv);
    verifiesNonBlockingImport(application);
    verifiesCooperativeCancellation(application);
    return failures == 0 ? 0 : 1;
}
