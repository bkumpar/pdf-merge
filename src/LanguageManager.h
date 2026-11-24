// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "AppSettings.h"

#include <QString>

class QTranslator;

/* Loads and installs the interface translation.
 *
 * Croatian strings are compiled into the binary as a Qt resource
 * (:/i18n/pdfmergegs_hr.qm), so no external files are needed at runtime.
 * Installing or removing a translator makes Qt post QEvent::LanguageChange
 * to every widget, which is where the windows retranslate themselves. */
class LanguageManager
{
public:
    static void apply(AppSettings::Language language);

    /* Language actually in effect: resolves "System" against the desktop. */
    static AppSettings::Language effectiveLanguage(AppSettings::Language language);

    static QString languageCode(AppSettings::Language language);

private:
    static QTranslator *s_translator;
};
