// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>

/* Asynchronous wrapper around a single Ghostscript run.
 *
 * The program is started with an argument list, never through a shell, so
 * paths containing spaces or diacritics are passed through untouched. */
class GhostscriptRunner : public QObject
{
    Q_OBJECT

public:
    explicit GhostscriptRunner(QObject *parent = nullptr);

    bool start(const QString &program, const QStringList &arguments);
    void cancel();
    bool isRunning() const;

    QString standardErrorText() const;

signals:
    void standardErrorReceived(const QString &chunk);
    void finished(bool success, int exitCode, const QString &errorText);
    void failedToStart(const QString &message);

private:
    void collectStandardError();

    QProcess m_process;
    QString m_standardError;
    bool m_cancelled = false;
};
