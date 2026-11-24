// SPDX-License-Identifier: GPL-3.0-or-later
#include "GhostscriptRunner.h"

GhostscriptRunner::GhostscriptRunner(QObject *parent)
    : QObject(parent)
{
    m_process.setProcessChannelMode(QProcess::SeparateChannels);

    connect(&m_process, &QProcess::readyReadStandardError, this,
            &GhostscriptRunner::collectStandardError);

    connect(&m_process, &QProcess::errorOccurred, this,
            [this](QProcess::ProcessError error) {
                if (error == QProcess::FailedToStart) {
                    emit failedToStart(m_process.errorString());
                }
            });

    connect(&m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int exitCode, QProcess::ExitStatus status) {
                collectStandardError();
                const bool success = !m_cancelled
                                     && status == QProcess::NormalExit
                                     && exitCode == 0;
                emit finished(success, exitCode, m_standardError.trimmed());
            });
}

bool GhostscriptRunner::start(const QString &program, const QStringList &arguments)
{
    if (m_process.state() != QProcess::NotRunning) {
        return false;
    }
    m_standardError.clear();
    m_cancelled = false;
    m_process.setProgram(program);
    m_process.setArguments(arguments);
    m_process.start();
    return true;
}

void GhostscriptRunner::cancel()
{
    if (m_process.state() == QProcess::NotRunning) {
        return;
    }
    m_cancelled = true;
    m_process.kill();
}

bool GhostscriptRunner::isRunning() const
{
    return m_process.state() != QProcess::NotRunning;
}

QString GhostscriptRunner::standardErrorText() const
{
    return m_standardError;
}

void GhostscriptRunner::collectStandardError()
{
    const QString chunk = QString::fromLocal8Bit(m_process.readAllStandardError());
    if (chunk.isEmpty()) {
        return;
    }
    m_standardError += chunk;
    emit standardErrorReceived(chunk);
}
