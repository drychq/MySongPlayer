#pragma once

#include "core/AudioTrack.h"

#include <QUrl>
#include <QString>

#include <filesystem>
#include <string_view>

namespace SongPlayer::QtAdapter {

[[nodiscard]] Core::AudioTrack makeCoreTrack(const QString& title,
                                             const QString& authorName,
                                             const QUrl& audioSource,
                                             const QUrl& imageSource,
                                             const QUrl& videoSource = QUrl());

[[nodiscard]] std::string toSourceKey(const QUrl& source);

[[nodiscard]] std::string toUtf8String(const QString& value);

[[nodiscard]] QString fromUtf8String(std::string_view value);

[[nodiscard]] std::filesystem::path toLocalFilePath(const QUrl& value);

[[nodiscard]] QUrl fromLocalFilePath(const std::filesystem::path& value);

} // namespace SongPlayer::QtAdapter
