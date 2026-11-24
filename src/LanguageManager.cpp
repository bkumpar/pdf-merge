// SPDX-License-Identifier: GPL-3.0-or-later
#include "LanguageManager.h"

#include <QCoreApplication>
#include <QLocale>
#include <QDebug>
#include <QTranslator>

QTranslator *LanguageManager::s_translator = nullptr;

AppSettings::Language
LanguageManager::effectiveLanguage(AppSettings::Language language)
{
    if (language != AppSettings::Language::System) {
        return language;
    }
    return QLocale::system().language() == QLocale::Croatian
               ? AppSettings::Language::Croatian
               : AppSettings::Language::English;
}

QString LanguageManager::languageCode(AppSettings::Language language)
{
    switch (effectiveLanguage(language)) {
    case AppSettings::Language::Croatian:
        return QStringLiteral("hr");
    case AppSettings::Language::English:
    case AppSettings::Language::System:
        break;
    }
    return QStringLiteral("en");
}

void LanguageManager::apply(AppSettings::Language language)
{
    if (s_translator) {
        QCoreApplication::removeTranslator(s_translator);
        delete s_translator;
        s_translator = nullptr;
    }

    if (effectiveLanguage(language) != AppSettings::Language::Croatian) {
        // English is the source language: no translator needed.
        return;
    }

    auto *translator = new QTranslator(QCoreApplication::instance());

    // Normally the .qm is embedded in the binary. A loose file next to the
    // executable is accepted as a fallback for builds made without lrelease.
    const QString resource = QStringLiteral(":/i18n/pdfmergegs_hr.qm");
    const QString looseFile = QCoreApplication::applicationDirPath()
                              + QStringLiteral("/pdfmergegs_hr.qm");

    if (translator->load(resource) || translator->load(looseFile)) {
        QCoreApplication::installTranslator(translator);
        s_translator = translator;
    } else {
        qWarning("pdfmergegs: Croatian translation not found (tried %s and %s)",
                 qUtf8Printable(resource), qUtf8Printable(looseFile));
        delete translator;
    }
}
