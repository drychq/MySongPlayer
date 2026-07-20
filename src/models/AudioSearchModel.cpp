#include "models/AudioSearchModel.h"

#include "models/AudioInfo.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrlQuery>

#include <limits>
#include <optional>
#include <utility>

namespace {

using std::numeric_limits;
using std::nullopt;
using std::optional;

QUrl defaultRequestUrl()
{
    return QUrl{QStringLiteral("https://api.jamendo.com/v3.0/tracks/")};
}

int boundedRowCount(qsizetype count)
{
    const qsizetype maximum{numeric_limits<int>::max()};
    return static_cast<int>(qMin(count, maximum));
}

optional<QList<AudioInfo *>> audioItemsFromJson(const QByteArray &data, QObject *parent)
{
    QJsonParseError parseError{};
    const QJsonDocument document{QJsonDocument::fromJson(data, &parseError)};
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        qWarning() << "Invalid audio search response:" << parseError.errorString();
        return nullopt;
    }

    const QJsonObject root{document.object()};
    const QJsonObject headers{root.value(QStringLiteral("headers")).toObject()};
    if (headers.value(QStringLiteral("status")).toString() != QStringLiteral("success")) {
        qWarning() << "Audio search failed:"
                   << headers.value(QStringLiteral("error_string")).toString();
        return nullopt;
    }

    const QJsonValue resultsValue{root.value(QStringLiteral("results"))};
    if (!resultsValue.isArray()) {
        qWarning() << "Invalid audio search response: results is not an array";
        return nullopt;
    }

    QList<AudioInfo *> audioItems{};
    // Braces would select QJsonArray's initializer-list constructor and wrap the
    // returned array as a single nested value instead of copying it.
    const QJsonArray results(resultsValue.toArray());
    audioItems.reserve(results.size());
    for (qsizetype index{0}; index < results.size(); ++index) {
        const QJsonObject entry{results.at(index).toObject()};
        if (!entry.value(QStringLiteral("audiodownload_allowed")).toBool()) {
            continue;
        }

        auto *audioInfo{new AudioInfo{parent}};
        audioInfo->setTitle(entry.value(QStringLiteral("name")).toString());
        audioInfo->setAuthorName(entry.value(QStringLiteral("artist_name")).toString());
        audioInfo->setImageSource(
            QUrl{entry.value(QStringLiteral("image")).toString()});
        audioInfo->setAudioSource(
            QUrl{entry.value(QStringLiteral("audiodownload")).toString()});
        audioItems.append(audioInfo);
    }

    return audioItems;
}

} // namespace

AudioSearchModel::AudioSearchModel(QObject *parent)
    : AudioSearchModel{nullptr, defaultRequestUrl(), parent}
{}

AudioSearchModel::AudioSearchModel(QNetworkAccessManager *networkManager,
                                   QUrl requestUrl,
                                   QObject *parent)
    : QAbstractListModel{parent}
    , m_networkManager{networkManager != nullptr ? networkManager : &m_ownedNetworkManager}
    , m_requestUrl{std::move(requestUrl)}
{}

AudioSearchModel::~AudioSearchModel()
{
    cancelActiveRequest();
}

int AudioSearchModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return boundedRowCount(m_audioList.size());
}

QVariant AudioSearchModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_audioList.size()) {
        return {};
    }

    const AudioInfo *audioInfo{m_audioList.at(index.row())};
    switch (static_cast<Role>(role)) {
    case AudioNameRole:
        return audioInfo->title();
    case AudioAuthorRole:
        return audioInfo->authorName();
    case AudioImageSourceRole:
        return audioInfo->imageSource();
    case AudioSourceRole:
        return audioInfo->audioSource();
    }

    return {};
}

QHash<int, QByteArray> AudioSearchModel::roleNames() const
{
    return {
        {AudioNameRole, QByteArrayLiteral("audioName")},
        {AudioAuthorRole, QByteArrayLiteral("audioAuthor")},
        {AudioImageSourceRole, QByteArrayLiteral("audioImageSource")},
        {AudioSourceRole, QByteArrayLiteral("audioSource")},
    };
}

void AudioSearchModel::searchSong(const QString &name)
{
    const QString searchTerm{name.trimmed()};
    cancelActiveRequest();
    if (searchTerm.isEmpty()) {
        setIsSearching(false);
        return;
    }

    if (!m_networkManager) {
        qWarning() << "Cannot search without a network access manager";
        setIsSearching(false);
        return;
    }

    QUrl requestUrl{m_requestUrl};
    QUrlQuery query{requestUrl};
    query.addQueryItem(QStringLiteral("client_id"), QStringLiteral("85b6a59c"));
    query.addQueryItem(QStringLiteral("namesearch"), searchTerm);
    query.addQueryItem(QStringLiteral("format"), QStringLiteral("json"));
    requestUrl.setQuery(query);

    setIsSearching(true);
    QNetworkReply *reply{m_networkManager->get(QNetworkRequest{requestUrl})};
    if (reply == nullptr) {
        qWarning() << "Network access manager returned no reply";
        setIsSearching(false);
        return;
    }

    m_reply = reply;
    const QPointer<QNetworkReply> guardedReply{reply};
    connect(reply, &QNetworkReply::finished, this, [this, guardedReply] {
        handleReply(guardedReply);
    });
}

void AudioSearchModel::cancelActiveRequest()
{
    const QPointer<QNetworkReply> reply{m_reply};
    m_reply.clear();
    if (!reply) {
        return;
    }

    reply->abort();
    reply->deleteLater();
}

void AudioSearchModel::handleReply(const QPointer<QNetworkReply> &reply)
{
    if (!reply) {
        return;
    }

    const bool isCurrentReply{m_reply == reply};
    if (isCurrentReply) {
        m_reply.clear();
    }
    reply->deleteLater();

    if (!isCurrentReply) {
        return;
    }

    setIsSearching(false);
    if (reply->error() == QNetworkReply::OperationCanceledError) {
        return;
    }
    if (reply->error() != QNetworkReply::NoError) {
        qWarning() << "Audio search reply failed:" << reply->errorString();
        return;
    }

    optional<QList<AudioInfo *>> audioItems{audioItemsFromJson(reply->readAll(), this)};
    if (!audioItems) {
        return;
    }

    beginResetModel();
    qDeleteAll(m_audioList);
    m_audioList = std::move(*audioItems);
    endResetModel();
}

bool AudioSearchModel::isSearching() const
{
    return m_isSearching;
}

void AudioSearchModel::setIsSearching(bool newIsSearching)
{
    if (m_isSearching == newIsSearching) {
        return;
    }

    m_isSearching = newIsSearching;
    emit isSearchingChanged();
}
