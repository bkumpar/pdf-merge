// SPDX-License-Identifier: GPL-3.0-or-later
#include "AppSettings.h"
#include "LanguageManager.h"
#include "MainWindow.h"

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QIcon>
#include <QLocale>
#include <QStringList>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("pdfmergegs"));
    QApplication::setApplicationDisplayName(QStringLiteral("PDF Merge GS"));
    QApplication::setApplicationVersion(QStringLiteral("1.0.0"));
    QApplication::setOrganizationName(QStringLiteral("PdfMergeGs"));
    QApplication::setDesktopFileName(QStringLiteral("pdfmergegs"));
    QApplication::setWindowIcon(QIcon(QStringLiteral(":/icons/pdfmergegs.svg")));

    QCommandLineParser parser;
    parser.setApplicationDescription(
        QCoreApplication::translate(
            "main", "Merge several PDF documents into one file using Ghostscript."));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument(
        QStringLiteral("files"),
        QCoreApplication::translate("main", "PDF files to load at start."),
        QStringLiteral("[files...]"));

    parser.process(app);

    // Interface language: explicit choice, otherwise the desktop locale.
    LanguageManager::apply(AppSettings::instance().language());

    MainWindow window;
    const QStringList positional = parser.positionalArguments();
    if (!positional.isEmpty()) {
        window.addFilesFromCommandLine(positional);
    }
    window.show();

    return app.exec();
}
