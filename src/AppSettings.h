// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QSettings>
#include <QString>
#include <QStringList>

/* Persistent application settings.
 *
 * Everything lives in one INI file: ~/.config/PdfMergeGs/config.ini
 * (QSettings::IniFormat, UserScope). The class is a thin, typed wrapper so
 * that the rest of the code never deals with raw keys. */
class AppSettings
{
public:
    enum class Language {
        System = 0,
        English = 1,
        Croatian = 2,
    };

    enum class Preset {
        Lossless = 0,
        Print = 1,
        Ebook = 2,
        Custom = 3,
    };

    static AppSettings &instance();

    AppSettings(const AppSettings &) = delete;
    AppSettings &operator=(const AppSettings &) = delete;

    Language language() const;
    void setLanguage(Language language);

    /* Custom Ghostscript binary. An empty value means "look it up on PATH". */
    QString gsPath() const;
    void setGsPath(const QString &path);
    QString resolveGsExecutable() const;

    Preset preset() const;
    void setPreset(Preset preset);

    QStringList gsArguments() const;
    void setGsArguments(const QStringList &arguments);

    QString lastSourceDir() const;
    void setLastSourceDir(const QString &directory);

    QString lastDestinationDir() const;
    void setLastDestinationDir(const QString &directory);

    int thumbnailWidth() const;
    void setThumbnailWidth(int width);

    QString viewMode() const;
    void setViewMode(const QString &mode);

    bool showFullPath() const;
    void setShowFullPath(bool full);

    bool menuBarVisible() const;
    void setMenuBarVisible(bool visible);

    void restoreDefaults();
    void sync();

    static QStringList presetArguments(Preset preset);
    static QString presetName(Preset preset);
    static QString defaultDirectory();

private:
    AppSettings();

    QString directoryOrDefault(const QString &key) const;
    void storeDirectory(const QString &key, const QString &directory);

    QSettings m_settings;
};
