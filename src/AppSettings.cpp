// SPDX-License-Identifier: GPL-3.0-or-later
#include "AppSettings.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

namespace {

constexpr auto kOrganization = "PdfMergeGs";
constexpr auto kApplication = "config";

constexpr auto kKeyLanguage = "ui/language";
constexpr auto kKeyGsPath = "ghostscript/binary";
constexpr auto kKeyGsPreset = "ghostscript/preset";
constexpr auto kKeyGsArguments = "ghostscript/arguments";
constexpr auto kKeySourceDir = "paths/lastSourceDir";
constexpr auto kKeyDestinationDir = "paths/lastDestinationDir";
constexpr auto kKeyThumbnailWidth = "ui/thumbnailWidth";
constexpr auto kKeyViewMode = "ui/viewMode";
constexpr auto kKeyShowFullPath = "ui/showFullPath";
constexpr auto kKeyMenuBarVisible = "ui/menuBarVisible";

} // namespace

AppSettings::AppSettings()
    : m_settings(QSettings::IniFormat, QSettings::UserScope,
                 QString::fromLatin1(kOrganization),
                 QString::fromLatin1(kApplication))
{
}

AppSettings &AppSettings::instance()
{
    static AppSettings settings;
    return settings;
}

AppSettings::Language AppSettings::language() const
{
    const int stored = m_settings.value(QString::fromLatin1(kKeyLanguage),
                                        static_cast<int>(Language::System))
                           .toInt();
    if (stored < static_cast<int>(Language::System)
        || stored > static_cast<int>(Language::Croatian)) {
        return Language::System;
    }
    return static_cast<Language>(stored);
}

void AppSettings::setLanguage(Language language)
{
    m_settings.setValue(QString::fromLatin1(kKeyLanguage),
                        static_cast<int>(language));
}

QString AppSettings::gsPath() const
{
    return m_settings.value(QString::fromLatin1(kKeyGsPath)).toString();
}

void AppSettings::setGsPath(const QString &path)
{
    m_settings.setValue(QString::fromLatin1(kKeyGsPath), path);
}

QString AppSettings::resolveGsExecutable() const
{
    const QString custom = gsPath().trimmed();
    if (!custom.isEmpty()) {
        const QFileInfo info(custom);
        if (info.exists() && info.isFile() && info.isExecutable()) {
            return info.absoluteFilePath();
        }
    }
    return QStandardPaths::findExecutable(QStringLiteral("gs"));
}

AppSettings::Preset AppSettings::preset() const
{
    const int stored = m_settings.value(QString::fromLatin1(kKeyGsPreset),
                                        static_cast<int>(Preset::Lossless))
                           .toInt();
    if (stored < static_cast<int>(Preset::Lossless)
        || stored > static_cast<int>(Preset::Custom)) {
        return Preset::Lossless;
    }
    return static_cast<Preset>(stored);
}

void AppSettings::setPreset(Preset preset)
{
    m_settings.setValue(QString::fromLatin1(kKeyGsPreset),
                        static_cast<int>(preset));
}

QStringList AppSettings::gsArguments() const
{
    const QStringList stored =
        m_settings.value(QString::fromLatin1(kKeyGsArguments)).toStringList();
    if (stored.isEmpty()) {
        return presetArguments(preset());
    }
    return stored;
}

void AppSettings::setGsArguments(const QStringList &arguments)
{
    m_settings.setValue(QString::fromLatin1(kKeyGsArguments), arguments);
}

QString AppSettings::lastSourceDir() const
{
    return directoryOrDefault(QString::fromLatin1(kKeySourceDir));
}

void AppSettings::setLastSourceDir(const QString &directory)
{
    storeDirectory(QString::fromLatin1(kKeySourceDir), directory);
}

QString AppSettings::lastDestinationDir() const
{
    return directoryOrDefault(QString::fromLatin1(kKeyDestinationDir));
}

void AppSettings::setLastDestinationDir(const QString &directory)
{
    storeDirectory(QString::fromLatin1(kKeyDestinationDir), directory);
}

void AppSettings::restoreDefaults()
{
    m_settings.clear();
    m_settings.sync();
}

void AppSettings::sync()
{
    m_settings.sync();
}

QStringList AppSettings::presetArguments(Preset preset)
{
    switch (preset) {
    case Preset::Lossless:
        return {
            QStringLiteral("-dBATCH"),
            QStringLiteral("-dNOPAUSE"),
            QStringLiteral("-q"),
            QStringLiteral("-sDEVICE=pdfwrite"),
            QStringLiteral("-dPDFSETTINGS=/default"),
            QStringLiteral("-dPassThroughJPEGImages=true"),
            QStringLiteral("-dPassThroughJPXImages=true"),
            QStringLiteral("-dDownsampleColorImages=false"),
            QStringLiteral("-dDownsampleGrayImages=false"),
            QStringLiteral("-dDownsampleMonoImages=false"),
            QStringLiteral("-dColorConversionStrategy=/LeaveColorUnchanged"),
        };
    case Preset::Print:
        return {
            QStringLiteral("-dBATCH"),
            QStringLiteral("-dNOPAUSE"),
            QStringLiteral("-q"),
            QStringLiteral("-sDEVICE=pdfwrite"),
            QStringLiteral("-dPDFSETTINGS=/printer"),
        };
    case Preset::Ebook:
        return {
            QStringLiteral("-dBATCH"),
            QStringLiteral("-dNOPAUSE"),
            QStringLiteral("-q"),
            QStringLiteral("-sDEVICE=pdfwrite"),
            QStringLiteral("-dPDFSETTINGS=/ebook"),
        };
    case Preset::Custom:
        break;
    }
    return presetArguments(Preset::Lossless);
}

QString AppSettings::presetName(Preset preset)
{
    switch (preset) {
    case Preset::Lossless:
        return QCoreApplication::translate(
            "AppSettings", "Lossless Quality (Recommended)");
    case Preset::Print:
        return QCoreApplication::translate(
            "AppSettings", "High Quality / Print (300 dpi)");
    case Preset::Ebook:
        return QCoreApplication::translate(
            "AppSettings", "Compressed / E-mail (150 dpi)");
    case Preset::Custom:
        return QCoreApplication::translate("AppSettings", "Custom");
    }
    return QCoreApplication::translate("AppSettings", "Custom");
}

QString AppSettings::defaultDirectory()
{
    const QString documents =
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (!documents.isEmpty()) {
        return documents;
    }
    const QString home = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    return home.isEmpty() ? QDir::currentPath() : home;
}

QString AppSettings::directoryOrDefault(const QString &key) const
{
    const QString stored = m_settings.value(key).toString();
    if (!stored.isEmpty() && QDir(stored).exists()) {
        return stored;
    }
    return defaultDirectory();
}

void AppSettings::storeDirectory(const QString &key, const QString &directory)
{
    if (directory.isEmpty()) {
        return;
    }
    m_settings.setValue(key, QDir(directory).absolutePath());
}

int AppSettings::thumbnailWidth() const
{
    return m_settings.value(QString::fromLatin1(kKeyThumbnailWidth), 150).toInt();
}

void AppSettings::setThumbnailWidth(int width)
{
    m_settings.setValue(QString::fromLatin1(kKeyThumbnailWidth), width);
}

QString AppSettings::viewMode() const
{
    return m_settings.value(QString::fromLatin1(kKeyViewMode),
                            QStringLiteral("icons")).toString();
}

void AppSettings::setViewMode(const QString &mode)
{
    m_settings.setValue(QString::fromLatin1(kKeyViewMode), mode);
}

bool AppSettings::showFullPath() const
{
    return m_settings.value(QString::fromLatin1(kKeyShowFullPath), false).toBool();
}

void AppSettings::setShowFullPath(bool full)
{
    m_settings.setValue(QString::fromLatin1(kKeyShowFullPath), full);
}

bool AppSettings::menuBarVisible() const
{
    return m_settings.value(QString::fromLatin1(kKeyMenuBarVisible), true).toBool();
}

void AppSettings::setMenuBarVisible(bool visible)
{
    m_settings.setValue(QString::fromLatin1(kKeyMenuBarVisible), visible);
}
