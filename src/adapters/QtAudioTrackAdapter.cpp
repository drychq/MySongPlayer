#include "adapters/QtAudioTrackAdapter.h"

#include <QByteArray>
#include <QFile>

namespace SongPlayer::QtAdapter {

std::string toUtf8String(const QString& value)
{
    const QByteArray bytes = value.toUtf8();
    return {bytes.constData(), static_cast<std::size_t>(bytes.size())};
}

QString fromUtf8String(std::string_view value)
{
    return QString::fromUtf8(value.data(), static_cast<qsizetype>(value.size()));
}

std::filesystem::path toLocalFilePath(const QUrl& value)
{
    if (!value.isLocalFile()) {
        return {};
    }

#ifdef Q_OS_WIN
    return std::filesystem::path(value.toLocalFile().toStdWString());
#else
    const QByteArray encoded = QFile::encodeName(value.toLocalFile());
    return std::filesystem::path(
        std::string(encoded.constData(), static_cast<std::size_t>(encoded.size())));
#endif
}

QUrl fromLocalFilePath(const std::filesystem::path& value)
{
#ifdef Q_OS_WIN
    return QUrl::fromLocalFile(QString::fromStdWString(value.native()));
#else
    const std::string& native = value.native();
    return QUrl::fromLocalFile(QFile::decodeName(
        QByteArray(native.data(), static_cast<qsizetype>(native.size()))));
#endif
}

std::string toSourceKey(const QUrl& source)
{
    return toUtf8String(source.toString());
}

Core::AudioTrack makeCoreTrack(const QString& title,
                               const QString& authorName,
                               const QUrl& audioSource,
                               const QUrl& imageSource,
                               const QUrl& videoSource)
{
    return Core::AudioTrack{
        .title = toUtf8String(title),
        .authorName = toUtf8String(authorName),
        .audioSource = toSourceKey(audioSource),
        .imageSource = toSourceKey(imageSource),
        .videoSource = toSourceKey(videoSource),
    };
}

} // namespace SongPlayer::QtAdapter
