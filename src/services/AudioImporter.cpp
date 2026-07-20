#include "services/AudioImporter.h"

#include "adapters/QtAudioTrackAdapter.h"
#include "infrastructure/TagLibAudioMetadataReader.h"

#include <QFuture>
#include <QFutureWatcher>
#include <QPromise>
#include <QStandardPaths>
#include <QThread>
#include <QThreadPool>
#include <QtConcurrentRun>

#include <filesystem>
#include <set>
#include <utility>
#include <vector>

using std::in_range;
using std::make_shared;
using std::make_unique;
using std::set;
using std::shared_ptr;
using std::size_t;
using std::string;
using std::vector;
namespace fs = std::filesystem;

namespace SongPlayer {
namespace {

constexpr auto kDefaultIconUrl{"qrc:/qt/qml/MySongPlayer/assets/icons/app_icon.png"};

} // namespace

struct AudioImporter::ImportSession {
    QThreadPool pool;
    QFutureWatcher<Core::AudioImportResult> watcher;

    ImportSession()
    {
        // One background reader preserves selection order and avoids saturating the disk.
        pool.setMaxThreadCount(1);
        pool.setObjectName(QStringLiteral("audio-import-pool"));
        watcher.setPendingResultsLimit(32);
    }
};

AudioImporter::AudioImporter(QObject* parent)
    : AudioImporter(
          make_shared<Infrastructure::TagLibAudioMetadataReader>(), parent)
{}

AudioImporter::AudioImporter(
    shared_ptr<const Core::IAudioMetadataReader> metadataReader,
    QObject* parent)
    : QObject(parent)
    , m_metadataReader{std::move(metadataReader)}
    , m_session{make_unique<ImportSession>()}
{
    Q_ASSERT(m_metadataReader);

    connect(&m_session->watcher, &QFutureWatcherBase::resultReadyAt,
            this, &AudioImporter::handleResult);
    connect(&m_session->watcher, &QFutureWatcherBase::finished,
            this, &AudioImporter::handleFinished);
}

AudioImporter::~AudioImporter()
{
    if (m_session->watcher.isRunning()) {
        m_session->watcher.future().cancel();
        m_session->watcher.future().waitForFinished();
    }
    m_session->pool.waitForDone();
}

bool AudioImporter::importing() const noexcept
{
    return m_importing;
}

int AudioImporter::importCompleted() const noexcept
{
    return m_importCompleted;
}

int AudioImporter::importTotal() const noexcept
{
    return m_importTotal;
}

void AudioImporter::importLocalAudio(const QList<QUrl>& fileUrls)
{
    Q_ASSERT(QThread::currentThread() == thread());

    if (m_importing) {
        emit importRejected(QStringLiteral("An audio import is already running"));
        return;
    }

    if (!in_range<int>(fileUrls.size())) {
        emit importRejected(QStringLiteral("The import batch is too large"));
        return;
    }

    set<fs::path> seenFiles;
    vector<fs::path> uniqueFiles;
    uniqueFiles.reserve(static_cast<size_t>(fileUrls.size()));
    int invalidInputs{0};
    for (const QUrl& url : fileUrls) {
        fs::path file{QtAdapter::toLocalFilePath(url).lexically_normal()};
        if (file.empty()) {
            ++invalidInputs;
            emit importFailed(url, QStringLiteral("Only local audio files can be imported"));
            continue;
        }
        if (!seenFiles.insert(file).second) {
            ++invalidInputs;
            emit importFailed(url, QStringLiteral("The same file appears more than once in this import"));
            continue;
        }
        uniqueFiles.push_back(std::move(file));
    }

    vector<Core::AudioImportRequest> requests;
    requests.reserve(uniqueFiles.size());
    const fs::path cacheRoot{QtAdapter::toLocalFilePath(QUrl::fromLocalFile(
        QStandardPaths::writableLocation(QStandardPaths::CacheLocation)))};
    const fs::path coverCache{cacheRoot / string{Core::kCoverCacheDirectoryName}};
    for (const fs::path& file : uniqueFiles) {
        requests.push_back(Core::AudioImportRequest{
            .audioFile = file,
            .coverCacheDirectory = coverCache,
        });
    }

    resetProgress(static_cast<int>(fileUrls.size()), invalidInputs);

    if (requests.empty()) {
        emit importStarted(m_importTotal);
        emit importFinished(0, m_failedCount, false);
        return;
    }

    m_importing = true;
    emit importingChanged();
    emit importStarted(m_importTotal);

    auto task{[reader = m_metadataReader,
               requests = std::move(requests)](
                    QPromise<Core::AudioImportResult>& promise) mutable {
        promise.setProgressRange(0, static_cast<int>(requests.size()));

        int completed{0};
        for (const Core::AudioImportRequest& request : requests) {
            promise.suspendIfRequested();
            if (promise.isCanceled()) {
                break;
            }

            promise.addResult(reader->read(request));
            promise.setProgressValue(++completed);
        }
    }};

    QFuture<Core::AudioImportResult> future{
        QtConcurrent::run(&m_session->pool, std::move(task))};
    m_session->watcher.setFuture(future);
}

void AudioImporter::cancelImport()
{
    Q_ASSERT(QThread::currentThread() == thread());
    if (m_importing) {
        m_session->watcher.future().cancel();
    }
}

void AudioImporter::handleResult(int resultIndex)
{
    Q_ASSERT(QThread::currentThread() == thread());

    const Core::AudioImportResult result{
        m_session->watcher.future().resultAt(resultIndex)};
    ++m_importCompleted;

    if (result) {
        ++m_importedCount;
        const Core::ImportedAudio& imported{*result};
        const QUrl coverUrl{imported.coverFile
            ? QtAdapter::fromLocalFilePath(*imported.coverFile)
            : QUrl{QString::fromLatin1(kDefaultIconUrl)}};
        emit audioImported(
            QtAdapter::fromUtf8String(imported.title),
            QtAdapter::fromUtf8String(imported.artist),
            QtAdapter::fromLocalFilePath(imported.audioFile),
            coverUrl);
    } else {
        ++m_failedCount;
        emit importFailed(
            QtAdapter::fromLocalFilePath(result.error().audioFile),
            QtAdapter::fromUtf8String(result.error().message));
    }

    emit importProgressChanged();
}

void AudioImporter::handleFinished()
{
    Q_ASSERT(QThread::currentThread() == thread());

    const bool canceled{m_session->watcher.future().isCanceled()};
    if (m_importing) {
        m_importing = false;
        emit importingChanged();
    }
    emit importFinished(m_importedCount, m_failedCount, canceled);
}

void AudioImporter::resetProgress(int total, int initialFailures)
{
    m_importCompleted = initialFailures;
    m_importTotal = total;
    m_importedCount = 0;
    m_failedCount = initialFailures;
    emit importProgressChanged();
}

} // namespace SongPlayer
