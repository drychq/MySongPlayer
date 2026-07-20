#include "models/AudioSearchModel.h"

#include <QByteArray>
#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPointer>
#include <QString>
#include <QUrl>
#include <QUrlQuery>

#include <cstring>
#include <iostream>
#include <string_view>
#include <utility>

namespace {

using std::cerr;
using std::memcpy;
using std::move;
using std::string_view;

int failures{0};

void expect(bool condition, string_view message)
{
    if (condition) {
        return;
    }

    cerr << "FAILED: " << message << '\n';
    ++failures;
}

class ControlledReply final : public QNetworkReply
{
public:
    ControlledReply(const QNetworkRequest &request,
                    QNetworkAccessManager::Operation operation,
                    QObject *parent)
        : QNetworkReply{parent}
    {
        setRequest(request);
        setUrl(request.url());
        setOperation(operation);
        open(QIODevice::ReadOnly | QIODevice::Unbuffered);
    }

    void abort() override
    {
        m_wasAborted = true;
    }

    [[nodiscard]] bool wasAborted() const
    {
        return m_wasAborted;
    }

    [[nodiscard]] qsizetype bytesRead() const
    {
        return m_readOffset;
    }

    [[nodiscard]] qsizetype payloadSize() const
    {
        return m_payload.size();
    }

    void finishSuccess(QByteArray payload)
    {
        finish(move(payload), QNetworkReply::NoError, {});
    }

    void finishError(QNetworkReply::NetworkError error, const QString &message)
    {
        finish({}, error, message);
    }

    qint64 bytesAvailable() const override
    {
        const qint64 unreadBytes{
            static_cast<qint64>(m_payload.size() - m_readOffset)};
        return unreadBytes + QNetworkReply::bytesAvailable();
    }

protected:
    qint64 readData(char *data, qint64 maxSize) override
    {
        const qint64 unreadBytes{
            static_cast<qint64>(m_payload.size() - m_readOffset)};
        if (unreadBytes <= 0) {
            return -1;
        }

        const qint64 bytesToCopy{qMin(maxSize, unreadBytes)};
        memcpy(data,
               m_payload.constData() + m_readOffset,
               static_cast<size_t>(bytesToCopy));
        m_readOffset += static_cast<qsizetype>(bytesToCopy);
        return bytesToCopy;
    }

private:
    void finish(QByteArray payload,
                QNetworkReply::NetworkError error,
                const QString &message)
    {
        if (m_isFinished) {
            return;
        }

        m_payload = move(payload);
        if (error != QNetworkReply::NoError) {
            setError(error, message);
        }

        m_isFinished = true;
        setFinished(true);
        emit finished();
    }

    QByteArray m_payload{};
    qsizetype m_readOffset{0};
    bool m_wasAborted{false};
    bool m_isFinished{false};
};

class ControlledNetworkAccessManager final : public QNetworkAccessManager
{
public:
    using QNetworkAccessManager::QNetworkAccessManager;

    [[nodiscard]] qsizetype replyCount() const
    {
        return m_replies.size();
    }

    [[nodiscard]] ControlledReply *replyAt(qsizetype index) const
    {
        return m_replies.at(index);
    }

    [[nodiscard]] QNetworkRequest requestAt(qsizetype index) const
    {
        return m_replies.at(index)->request();
    }

protected:
    QNetworkReply *createRequest(Operation operation,
                                 const QNetworkRequest &request,
                                 QIODevice *outgoingData) override
    {
        Q_UNUSED(outgoingData);
        auto *reply{new ControlledReply{request, operation, this}};
        m_replies.append(reply);
        return reply;
    }

private:
    QList<QPointer<ControlledReply>> m_replies{};
};

QByteArray successResponse(const QString &title)
{
    const QJsonObject entry{
        {QStringLiteral("audiodownload_allowed"), true},
        {QStringLiteral("name"), title},
        {QStringLiteral("artist_name"), QStringLiteral("Test Artist")},
        {QStringLiteral("image"), QStringLiteral("https://example.test/cover.jpg")},
        {QStringLiteral("audiodownload"), QStringLiteral("https://example.test/audio.mp3")},
    };
    const QJsonObject root{
        {QStringLiteral("headers"),
         QJsonObject{{QStringLiteral("status"), QStringLiteral("success")}}},
        {QStringLiteral("results"), QJsonArray{entry}},
    };
    return QJsonDocument{root}.toJson(QJsonDocument::Compact);
}

QByteArray serviceFailureResponse()
{
    const QJsonObject root{
        {QStringLiteral("headers"),
         QJsonObject{
             {QStringLiteral("status"), QStringLiteral("failed")},
             {QStringLiteral("error_string"), QStringLiteral("service rejected request")},
         }},
        {QStringLiteral("results"), QJsonArray{}},
    };
    return QJsonDocument{root}.toJson(QJsonDocument::Compact);
}

} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication application{argc, argv};
    ControlledNetworkAccessManager networkManager{};
    AudioSearchModel model{
        &networkManager, QUrl{QStringLiteral("https://example.test/search")}};

    int searchingSignalCount{0};
    QObject::connect(&model, &AudioSearchModel::isSearchingChanged, [&searchingSignalCount] {
        ++searchingSignalCount;
    });

    expect(!model.isSearching(), "searching state is initialized to false");
    expect(model.rowCount({}) == 0, "model starts empty");

    model.searchSong(QStringLiteral("  A  "));
    expect(model.isSearching(), "first request sets searching state");
    expect(networkManager.replyCount() == 1, "first request is created");
    const QUrlQuery firstQuery{networkManager.requestAt(0).url()};
    expect(firstQuery.queryItemValue(QStringLiteral("namesearch")) == QStringLiteral("A"),
           "trimmed search term is sent");
    ControlledReply *firstReply{networkManager.replyAt(0)};

    model.searchSong(QStringLiteral("B"));
    expect(firstReply->wasAborted(), "replacement aborts the previous request");
    expect(model.isSearching(), "replacement remains busy");
    expect(networkManager.replyCount() == 2, "replacement request is created");
    ControlledReply *secondReply{networkManager.replyAt(1)};
    expect(secondReply->error() == QNetworkReply::NoError,
           "controlled reply starts without an error");

    firstReply->finishSuccess(successResponse(QStringLiteral("stale A")));
    expect(model.isSearching(), "late old reply does not clear busy state");
    expect(model.rowCount({}) == 0, "late old reply does not replace results");
    expect(searchingSignalCount == 1, "late old reply emits no searching change");

    const QByteArray freshResponse{successResponse(QStringLiteral("fresh B"))};
    const QJsonDocument freshDocument{QJsonDocument::fromJson(freshResponse)};
    expect(freshDocument.object()
               .value(QStringLiteral("results"))
               .toArray()
               .first()
               .toObject()
               .value(QStringLiteral("audiodownload_allowed"))
               .toBool(),
           "controlled success payload contains a downloadable result");
    secondReply->finishSuccess(freshResponse);
    expect(secondReply->error() == QNetworkReply::NoError,
           "controlled success remains error-free after finishing");
    expect(secondReply->bytesRead() > 0, "model reads the current reply payload");
    expect(secondReply->bytesRead() == secondReply->payloadSize(),
           "model reads the complete current reply payload");
    expect(!model.isSearching(), "current success clears busy state");
    expect(searchingSignalCount == 2, "current success emits one searching change");
    expect(model.rowCount({}) == 1, "current success replaces results");
    expect(model.data(model.index(0), AudioSearchModel::AudioNameRole).toString()
               == QStringLiteral("fresh B"),
           "current response data is exposed");

    model.searchSong(QStringLiteral("network error"));
    ControlledReply *errorReply{networkManager.replyAt(2)};
    errorReply->finishError(QNetworkReply::ConnectionRefusedError,
                            QStringLiteral("connection refused"));
    expect(!model.isSearching(), "network error clears busy state");
    expect(model.rowCount({}) == 1, "network error preserves previous results");

    model.searchSong(QStringLiteral("invalid json"));
    ControlledReply *invalidJsonReply{networkManager.replyAt(3)};
    invalidJsonReply->finishSuccess(QByteArrayLiteral("{not-json"));
    expect(!model.isSearching(), "invalid JSON clears busy state");
    expect(model.rowCount({}) == 1, "invalid JSON preserves previous results");

    model.searchSong(QStringLiteral("service failure"));
    ControlledReply *serviceFailureReply{networkManager.replyAt(4)};
    serviceFailureReply->finishSuccess(serviceFailureResponse());
    expect(!model.isSearching(), "service failure clears busy state");
    expect(model.rowCount({}) == 1, "service failure preserves previous results");

    model.searchSong(QStringLiteral("cancelled"));
    ControlledReply *cancelledReply{networkManager.replyAt(5)};
    cancelledReply->finishError(QNetworkReply::OperationCanceledError,
                                QStringLiteral("cancelled"));
    expect(!model.isSearching(), "cancelled current reply clears busy state");
    expect(model.rowCount({}) == 1, "cancelled reply preserves previous results");

    model.searchSong(QStringLiteral("clear with empty query"));
    ControlledReply *clearedReply{networkManager.replyAt(6)};
    model.searchSong(QStringLiteral("   "));
    expect(clearedReply->wasAborted(), "empty query aborts the active request");
    expect(!model.isSearching(), "empty query clears busy state");
    clearedReply->finishSuccess(successResponse(QStringLiteral("late cleared result")));
    expect(model.rowCount({}) == 1, "reply cancelled by empty query cannot replace results");

    return failures == 0 ? 0 : 1;
}
