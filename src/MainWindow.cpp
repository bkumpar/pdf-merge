// SPDX-License-Identifier: GPL-3.0-or-later
#include "MainWindow.h"

#include "AppSettings.h"
#include "GhostscriptRunner.h"
#include "LanguageManager.h"
#include "PdfDocumentInfo.h"
#include "PdfListWidget.h"
#include "SettingsDialog.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QDir>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QStatusBar>
#include <QToolBar>
#include <QUrl>
#include <QVBoxLayout>

namespace {
constexpr int kMaxErrorCharacters = 4000;
} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_runner(new GhostscriptRunner(this))
{
    buildUi();
    retranslateUi();
    refreshState();

    connect(m_runner, &GhostscriptRunner::finished, this,
            &MainWindow::onMergeFinished);
    connect(m_runner, &GhostscriptRunner::failedToStart, this,
            &MainWindow::onMergeFailedToStart);
}

void MainWindow::buildUi()
{
    setMinimumSize(760, 520);

    auto *toolBar = addToolBar(tr("Main"));
    toolBar->setObjectName(QStringLiteral("mainToolBar"));
    toolBar->setMovable(false);
    toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    m_addAction = toolBar->addAction(QIcon::fromTheme(QStringLiteral("document-open")),
                                     QString());
    m_addAction->setShortcut(QKeySequence::Open);
    connect(m_addAction, &QAction::triggered, this, &MainWindow::addFilesViaDialog);

    m_removeAction = toolBar->addAction(
        QIcon::fromTheme(QStringLiteral("list-remove")), QString());
    connect(m_removeAction, &QAction::triggered, this, &MainWindow::removeSelected);

    m_clearAction = toolBar->addAction(
        QIcon::fromTheme(QStringLiteral("edit-clear")), QString());
    connect(m_clearAction, &QAction::triggered, this, &MainWindow::clearAll);

    toolBar->addSeparator();

    m_settingsAction = toolBar->addAction(
        QIcon::fromTheme(QStringLiteral("configure")), QString());
    connect(m_settingsAction, &QAction::triggered, this, &MainWindow::openSettings);

    auto *spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    toolBar->addWidget(spacer);

    m_mergeAction = toolBar->addAction(
        QIcon::fromTheme(QStringLiteral("document-save")), QString());
    connect(m_mergeAction, &QAction::triggered, this, &MainWindow::startMerge);

    auto *central = new QWidget(this);
    auto *layout = new QVBoxLayout(central);

    m_banner = new QFrame(central);
    m_banner->setFrameShape(QFrame::StyledPanel);
    m_banner->setStyleSheet(QStringLiteral(
        "QFrame { background-color: #fff4e5; border: 1px solid #e0a800;"
        " border-radius: 4px; }"));
    auto *bannerLayout = new QHBoxLayout(m_banner);
    m_bannerLabel = new QLabel(m_banner);
    m_bannerLabel->setWordWrap(true);
    m_bannerButton = new QPushButton(m_banner);
    bannerLayout->addWidget(m_bannerLabel, 1);
    bannerLayout->addWidget(m_bannerButton);
    connect(m_bannerButton, &QPushButton::clicked, this, &MainWindow::openSettings);
    layout->addWidget(m_banner);

    m_list = new PdfListWidget(central);
    layout->addWidget(m_list, 1);

    m_progressRow = new QWidget(central);
    auto *progressLayout = new QHBoxLayout(m_progressRow);
    progressLayout->setContentsMargins(0, 0, 0, 0);
    m_progress = new QProgressBar(m_progressRow);
    m_progress->setRange(0, 0); // busy indicator
    m_progress->setTextVisible(false);
    m_cancelButton = new QPushButton(m_progressRow);
    progressLayout->addWidget(m_progress, 1);
    progressLayout->addWidget(m_cancelButton);
    m_progressRow->setVisible(false);
    layout->addWidget(m_progressRow);

    connect(m_cancelButton, &QPushButton::clicked, this, &MainWindow::cancelMerge);

    setCentralWidget(central);

    connect(m_list, &PdfListWidget::filesChanged, this, &MainWindow::refreshState);
    connect(m_list, &PdfListWidget::sourceDirectoryChanged, this,
            [](const QString &directory) {
                if (!directory.isEmpty()) {
                    AppSettings::instance().setLastSourceDir(directory);
                    AppSettings::instance().sync();
                }
            });

    statusBar()->showMessage(tr("Ready"));
}

void MainWindow::retranslateUi()
{
    setWindowTitle(tr("PDF Merge GS"));

    m_addAction->setText(tr("Add PDFs..."));
    m_removeAction->setText(tr("Remove Selected"));
    m_clearAction->setText(tr("Clear All"));
    m_settingsAction->setText(tr("Settings..."));
    m_mergeAction->setText(tr("Merge"));

    m_bannerLabel->setText(
        tr("Ghostscript (gs) was not found on this system. Install the "
           "\"ghostscript\" package or point to the binary in the settings. "
           "You can still add and arrange documents."));
    m_bannerButton->setText(tr("Open Settings"));

    m_cancelButton->setText(tr("Cancel"));

    statusBar()->showMessage(m_runner->isRunning() ? tr("Merging...") : tr("Ready"));
}

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
        m_list->retranslateItems();
    }
    QMainWindow::changeEvent(event);
}

void MainWindow::addFilesFromCommandLine(const QStringList &paths)
{
    QStringList accepted;
    for (const QString &path : paths) {
        if (PdfTools::isPdfFile(path)) {
            accepted.append(path);
        }
    }
    if (accepted.isEmpty()) {
        return;
    }
    m_list->addFiles(accepted);
    statusBar()->showMessage(
        tr("%n file(s) loaded from the command line.", nullptr, accepted.size()),
        4000);
}

void MainWindow::addFilesViaDialog()
{
    AppSettings &settings = AppSettings::instance();
    const QStringList paths = QFileDialog::getOpenFileNames(
        this, tr("Add PDF files"), settings.lastSourceDir(),
        tr("PDF documents (*.pdf)"));
    if (paths.isEmpty()) {
        return;
    }

    const int added = m_list->addFiles(paths);
    if (added > 0) {
        settings.setLastSourceDir(QFileInfo(paths.first()).absolutePath());
        settings.sync();
        statusBar()->showMessage(
            tr("%n file(s) added.", nullptr, added), 4000);
    }
}

void MainWindow::removeSelected()
{
    m_list->removeSelected();
}

void MainWindow::clearAll()
{
    m_list->clearAll();
}

void MainWindow::openSettings()
{
    SettingsDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    if (dialog.languageChanged()) {
        LanguageManager::apply(AppSettings::instance().language());
    }
    refreshState();
}

void MainWindow::updateGhostscriptBanner()
{
    const QString executable = AppSettings::instance().resolveGsExecutable();
    m_banner->setVisible(executable.isEmpty());
}

void MainWindow::refreshState()
{
    updateGhostscriptBanner();

    const bool running = m_runner->isRunning();
    const bool hasGhostscript =
        !AppSettings::instance().resolveGsExecutable().isEmpty();
    const bool hasFiles = m_list->count() > 0;

    m_mergeAction->setEnabled(hasGhostscript && hasFiles && !running);
    m_removeAction->setEnabled(hasFiles && !running);
    m_clearAction->setEnabled(hasFiles && !running);
    m_addAction->setEnabled(!running);

    m_progressRow->setVisible(running);
}

void MainWindow::setBusy(bool busy)
{
    m_progressRow->setVisible(busy);
    if (busy) {
        statusBar()->showMessage(tr("Merging..."));
    }
    refreshState();
}

bool MainWindow::confirmSignedDocuments()
{
    const QStringList signedFiles = m_list->signedFilePaths();
    if (signedFiles.isEmpty()) {
        return true;
    }

    QStringList names;
    for (const QString &path : signedFiles) {
        names.append(QFileInfo(path).fileName());
    }

    QMessageBox box(this);
    box.setIcon(QMessageBox::Warning);
    box.setWindowTitle(tr("Digitally signed documents"));
    box.setText(tr("The following documents are digitally signed:"));
    box.setInformativeText(
        tr("Merging rewrites the files, so every digital signature will be "
           "permanently removed and the documents will no longer be valid as "
           "signed originals.\n\nDo you want to continue?"));
    box.setDetailedText(names.join(QLatin1Char('\n')));

    QPushButton *continueButton = box.addButton(tr("Merge Anyway"), QMessageBox::YesRole);
    QPushButton *cancelButton = box.addButton(QMessageBox::No);
    box.setDefaultButton(cancelButton);
    box.setEscapeButton(cancelButton);
    box.exec();

    return box.clickedButton() == continueButton;
}

void MainWindow::startMerge()
{
    const QStringList files = m_list->filePaths();
    if (files.size() < 2) {
        QMessageBox::information(
            this, tr("Not enough documents"),
            tr("Add at least two PDF documents before merging."));
        return;
    }

    const QString executable = AppSettings::instance().resolveGsExecutable();
    if (executable.isEmpty()) {
        QMessageBox::warning(
            this, tr("Ghostscript not found"),
            tr("Ghostscript (gs) is required for merging. Install the "
               "\"ghostscript\" package or set the binary path in the settings."));
        openSettings();
        return;
    }

    if (!confirmSignedDocuments()) {
        return;
    }

    AppSettings &settings = AppSettings::instance();
    const QString suggested = QDir(settings.lastDestinationDir())
                                  .filePath(tr("merged.pdf"));
    QString outputPath = QFileDialog::getSaveFileName(
        this, tr("Save merged PDF"), suggested, tr("PDF documents (*.pdf)"));
    if (outputPath.isEmpty()) {
        return;
    }
    if (!outputPath.endsWith(QStringLiteral(".pdf"), Qt::CaseInsensitive)) {
        outputPath += QStringLiteral(".pdf");
    }

    settings.setLastDestinationDir(QFileInfo(outputPath).absolutePath());
    settings.sync();

    QStringList arguments = settings.gsArguments();
    arguments << QStringLiteral("-sOutputFile=%1").arg(outputPath);
    arguments << QStringLiteral("-f");
    arguments << files;

    m_lastOutputPath = outputPath;
    m_mergeCancelled = false;
    setBusy(true);

    if (!m_runner->start(executable, arguments)) {
        setBusy(false);
        QMessageBox::warning(this, tr("Merge failed"),
                             tr("Another merge is already running."));
    }
}

void MainWindow::cancelMerge()
{
    if (!m_runner->isRunning()) {
        return;
    }
    m_mergeCancelled = true;
    m_runner->cancel();
    statusBar()->showMessage(tr("Merge cancelled."), 4000);
}

void MainWindow::onMergeFinished(bool success, int exitCode,
                                 const QString &errorText)
{
    setBusy(false);

    if (m_mergeCancelled) {
        statusBar()->showMessage(tr("Merge cancelled."), 5000);
        return;
    }

    if (success) {
        statusBar()->showMessage(tr("Merged document saved."), 5000);
        showMergeSuccess(m_lastOutputPath);
        return;
    }

    QString details = errorText;
    if (details.isEmpty()) {
        details = tr("Ghostscript exited with code %1.").arg(exitCode);
    }
    statusBar()->showMessage(tr("Merge failed."), 5000);
    showMergeFailure(details);
}

void MainWindow::onMergeFailedToStart(const QString &message)
{
    setBusy(false);
    showMergeFailure(message);
}

void MainWindow::showMergeSuccess(const QString &outputPath)
{
    QMessageBox box(this);
    box.setIcon(QMessageBox::Information);
    box.setWindowTitle(tr("Merge finished"));
    box.setText(tr("The merged document was saved to:"));
    box.setInformativeText(outputPath);

    QPushButton *openPdf = box.addButton(tr("Open PDF"), QMessageBox::AcceptRole);
    QPushButton *openFolder =
        box.addButton(tr("Open Folder"), QMessageBox::ActionRole);
    box.addButton(QMessageBox::Close);
    box.exec();

    if (box.clickedButton() == openPdf) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(outputPath));
    } else if (box.clickedButton() == openFolder) {
        QDesktopServices::openUrl(
            QUrl::fromLocalFile(QFileInfo(outputPath).absolutePath()));
    }
}

void MainWindow::showMergeFailure(const QString &details)
{
    QString text = details;
    if (text.size() > kMaxErrorCharacters) {
        text = text.left(kMaxErrorCharacters)
               + QStringLiteral("\n...");
    }

    QMessageBox box(this);
    box.setIcon(QMessageBox::Critical);
    box.setWindowTitle(tr("Merge failed"));
    box.setText(tr("Ghostscript could not merge the documents."));
    box.setInformativeText(
        tr("The error output below may name the file or the option that "
           "caused the problem."));
    box.setDetailedText(text);
    box.exec();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (!m_runner->isRunning()) {
        event->accept();
        return;
    }

    const QMessageBox::StandardButton answer = QMessageBox::question(
        this, tr("Merge in progress"),
        tr("A merge is still running. Cancel it and quit?"),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes) {
        event->ignore();
        return;
    }

    m_mergeCancelled = true;
    m_runner->cancel();
    event->accept();
}
