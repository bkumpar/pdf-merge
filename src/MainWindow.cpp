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
#include <QLocale>
#include <QMenu>
#include <QMenuBar>
#include <QSignalBlocker>
#include <QSlider>
#include <QMessageBox>
#include <QActionGroup>
#include <QProgressBar>
#include <QPushButton>
#include <QSlider>
#include <QStatusBar>
#include <QToolBar>
#include <QToolButton>
#include <QPainter>
#include <QPixmap>
#include <QUrl>
#include <QVBoxLayout>

namespace {
constexpr int kMaxErrorCharacters = 4000;
} // namespace

/* The switch is icon only, so it needs a glyph even when no icon theme is
 * installed; the painted fallback keeps it from turning into a blank button. */
static QIcon menuBarIcon()
{
    for (const QString &name : {QStringLiteral("application-menu"), QStringLiteral("show-menu")}) {
        const QIcon icon = QIcon::fromTheme(name);
        if (!icon.isNull()) {
            return icon;
        }
    }

    QPixmap pixmap(32, 32);
    pixmap.setDevicePixelRatio(2.0);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(QPen(QApplication::palette().color(QPalette::WindowText), 1.6));
    for (int row = 0; row < 3; ++row) {
        const int y = 4 + row * 4;
        painter.drawLine(2, y, 13, y);
    }
    return QIcon(pixmap);
}

/* The switch means "menu bar is visible", so its wording follows that state. */
static QString menuBarToolTip(bool visible)
{
    return visible ? MainWindow::tr("Hide the menu bar")
                   : MainWindow::tr("Show the menu bar");
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_runner(new GhostscriptRunner(this))
{
    buildUi();
    retranslateUi();

    AppSettings &settings = AppSettings::instance();
    m_toggleMenuBarAction->setChecked(settings.menuBarVisible());
    menuBar()->setVisible(settings.menuBarVisible());

    QSignalBlocker sliderBlocker(m_sizeSlider);
    m_sizeSlider->setValue(settings.thumbnailWidth());

    refreshState();

    connect(m_runner, &GhostscriptRunner::finished, this,
            &MainWindow::onMergeFinished);
    connect(m_runner, &GhostscriptRunner::failedToStart, this,
            &MainWindow::onMergeFailedToStart);
}

void MainWindow::buildUi()
{
    setMinimumSize(760, 520);

    /* Actions live here, not in the toolbar, because the toolbar and the
     * menu bar show the same objects. */
    m_mergeAction = new QAction(QIcon::fromTheme(QStringLiteral("document-save")),
                                QString(), this);
    connect(m_mergeAction, &QAction::triggered, this, &MainWindow::startMerge);

    m_addAction = new QAction(QIcon::fromTheme(QStringLiteral("document-open")),
                              QString(), this);
    m_addAction->setShortcut(QKeySequence::Open);
    connect(m_addAction, &QAction::triggered, this, &MainWindow::addFilesViaDialog);

    m_removeAction = new QAction(QIcon::fromTheme(QStringLiteral("list-remove")),
                                 QString(), this);
    m_removeAction->setShortcut(QKeySequence::Delete);
    connect(m_removeAction, &QAction::triggered, this, &MainWindow::removeSelected);

    m_clearAction = new QAction(QIcon::fromTheme(QStringLiteral("edit-clear")),
                                QString(), this);
    connect(m_clearAction, &QAction::triggered, this, &MainWindow::clearAll);

    m_iconViewAction = new QAction(QIcon::fromTheme(QStringLiteral("view-list-icons")),
                                   QString(), this);
    m_iconViewAction->setCheckable(true);
    m_iconViewAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_1));
    m_detailsViewAction = new QAction(QIcon::fromTheme(QStringLiteral("view-list-details")),
                                      QString(), this);
    m_detailsViewAction->setCheckable(true);
    m_detailsViewAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_2));

    auto *viewGroup = new QActionGroup(this);
    viewGroup->addAction(m_iconViewAction);
    viewGroup->addAction(m_detailsViewAction);
    connect(m_iconViewAction, &QAction::triggered, this, [this] {
        applyDisplayMode(PdfListWidget::Icons);
    });
    connect(m_detailsViewAction, &QAction::triggered, this, [this] {
        applyDisplayMode(PdfListWidget::Details);
    });

    m_fullPathAction = new QAction(QString(), this);
    m_fullPathAction->setCheckable(true);
    m_fullPathAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_L));
    connect(m_fullPathAction, &QAction::toggled, this, [this](bool checked) {
        m_list->setShowFullPath(checked);
        AppSettings::instance().setShowFullPath(checked);
        AppSettings::instance().sync();
    });

    m_settingsAction = new QAction(QIcon::fromTheme(QStringLiteral("configure")),
                                   QString(), this);
    m_settingsAction->setShortcut(QKeySequence::Preferences);
    connect(m_settingsAction, &QAction::triggered, this, &MainWindow::openSettings);

    m_aboutAction = new QAction(QIcon::fromTheme(QStringLiteral("help-about")),
                                QString(), this);
    connect(m_aboutAction, &QAction::triggered, this, &MainWindow::showAbout);

    m_aboutQtAction = new QAction(QString(), this);
    /* Qt ships the official logo for QMessageBox::aboutQt; using it keeps the
     * entry recognisable without adding our own copy of the artwork. */
    m_aboutQtAction->setIcon(
        QIcon(QStringLiteral(":/qt-project.org/qmessagebox/images/qtlogo-64.png")));
    connect(m_aboutQtAction, &QAction::triggered, this, &MainWindow::showAboutQt);

    m_quitAction = new QAction(QString(), this);
    m_quitAction->setShortcut(QKeySequence::Quit);
    connect(m_quitAction, &QAction::triggered, this, &MainWindow::close);

    m_toggleMenuBarAction = new QAction(QString(), this);
    m_toggleMenuBarAction->setCheckable(true);
    m_toggleMenuBarAction->setChecked(true);
    m_toggleMenuBarAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_M));
    connect(m_toggleMenuBarAction, &QAction::toggled, this, [this](bool visible) {
        menuBar()->setVisible(visible);
        m_toggleMenuBarAction->setToolTip(menuBarToolTip(visible));
        AppSettings::instance().setMenuBarVisible(visible);
        AppSettings::instance().sync();
    });
    /* Registered on the window as well, so Ctrl+M still works while the
     * menu bar itself is hidden. */
    addAction(m_toggleMenuBarAction);

    auto *toolBar = addToolBar(tr("Main"));
    toolBar->setObjectName(QStringLiteral("mainToolBar"));
    toolBar->setMovable(false);
    toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    toolBar->addAction(m_mergeAction);
    toolBar->addSeparator();
    toolBar->addAction(m_addAction);
    toolBar->addAction(m_removeAction);
    toolBar->addAction(m_clearAction);
    toolBar->addSeparator();
    toolBar->addAction(m_iconViewAction);
    toolBar->addAction(m_detailsViewAction);
    toolBar->addAction(m_fullPathAction);
    toolBar->addSeparator();
    toolBar->addAction(m_settingsAction);
    toolBar->addAction(m_aboutAction);
    toolBar->addSeparator();
    toolBar->addAction(m_toggleMenuBarAction);
    /* The switch has to stay reachable after the menu bar is gone, so it sits
     * in the toolbar as an icon only instead of taking text space. */
    if (auto *menuButton = qobject_cast<QToolButton *>(
            toolBar->widgetForAction(m_toggleMenuBarAction))) {
        menuButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    }
    /* Qt moves trailing toolbar items into an overflow menu when the window
     * gets narrow, which would hide the only way back to the menu bar. */
    setMinimumWidth(qMax(minimumWidth(), toolBar->sizeHint().width()));

    m_fileMenu = menuBar()->addMenu(QString());
    m_fileMenu->addAction(m_addAction);
    m_fileMenu->addAction(m_removeAction);
    m_fileMenu->addAction(m_clearAction);
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(m_mergeAction);
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(m_quitAction);

    m_viewMenu = menuBar()->addMenu(QString());
    m_viewMenu->addAction(m_iconViewAction);
    m_viewMenu->addAction(m_detailsViewAction);
    m_viewMenu->addSeparator();
    m_viewMenu->addAction(m_fullPathAction);

    m_settingsMenu = menuBar()->addMenu(QString());
    m_settingsMenu->addAction(m_settingsAction);

    m_helpMenu = menuBar()->addMenu(QString());
    m_helpMenu->addAction(m_aboutAction);
    m_helpMenu->addAction(m_aboutQtAction);

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

    auto *summaryBar = new QWidget(central);
    summaryBar->setObjectName(QStringLiteral("summaryBar"));
    auto *summaryLayout = new QHBoxLayout(summaryBar);
    summaryLayout->setContentsMargins(6, 2, 6, 2);

    m_summaryLabel = new QLabel(summaryBar);
    summaryLayout->addWidget(m_summaryLabel, 0);
    summaryLayout->addStretch(1);

    m_sizeLabel = new QLabel(summaryBar);
    summaryLayout->addWidget(m_sizeLabel);

    m_sizeSlider = new QSlider(Qt::Horizontal, summaryBar);
    m_sizeSlider->setRange(80, 280);
    m_sizeSlider->setSingleStep(10);
    m_sizeSlider->setPageStep(20);
    m_sizeSlider->setFixedWidth(160);
    summaryLayout->addWidget(m_sizeSlider);
    connect(m_sizeSlider, &QSlider::valueChanged, this,
            &MainWindow::onSizeSliderMoved);
    connect(m_sizeSlider, &QSlider::sliderReleased, this, [this] {
        AppSettings &settings = AppSettings::instance();
        if (m_list->displayMode() == PdfListWidget::Icons) {
            settings.setThumbnailWidth(m_sizeSlider->value());
        } else {
            settings.setDetailsIconWidth(m_sizeSlider->value());
        }
        settings.sync();
    });

    layout->addWidget(summaryBar);

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

    AppSettings &settings = AppSettings::instance();
    m_list->setThumbnailWidth(settings.thumbnailWidth());
    m_list->setDetailsIconWidth(settings.detailsIconWidth());
    m_fullPathAction->setChecked(settings.showFullPath());
    m_list->setShowFullPath(settings.showFullPath());
    applyDisplayMode(settings.viewMode() == QStringLiteral("details")
                         ? PdfListWidget::Details
                         : PdfListWidget::Icons);

    statusBar()->showMessage(tr("Ready"));
}

void MainWindow::retranslateUi()
{
    setWindowTitle(tr("PDF Merge GS"));

    m_addAction->setText(tr("Add PDFs..."));
    m_removeAction->setText(tr("Remove Selected"));
    m_clearAction->setText(tr("Clear All"));
    m_settingsAction->setText(tr("Settings..."));
    m_aboutAction->setText(tr("About"));
    m_aboutAction->setToolTip(tr("About this application"));
    m_aboutQtAction->setText(tr("About Qt"));
    m_quitAction->setText(tr("Quit"));
    m_toggleMenuBarAction->setText(tr("Show Menu Bar"));
    m_toggleMenuBarAction->setIcon(menuBarIcon());
    m_toggleMenuBarAction->setToolTip(menuBarToolTip(m_toggleMenuBarAction->isChecked()));

    m_fileMenu->setTitle(tr("&File"));
    m_viewMenu->setTitle(tr("&View"));
    m_settingsMenu->setTitle(tr("&Settings"));
    m_helpMenu->setTitle(tr("&Help"));

    m_iconViewAction->setText(tr("Icons"));
    m_iconViewAction->setToolTip(tr("Show page thumbnails"));
    m_detailsViewAction->setText(tr("Details"));
    m_detailsViewAction->setToolTip(tr("Show a compact list with document paths"));
    m_fullPathAction->setText(tr("Full path"));
    m_fullPathAction->setToolTip(tr("Show the whole path instead of the common part"));
    m_sizeLabel->setText(tr("Icon size"));
    m_sizeSlider->setToolTip(tr("Thumbnail size in pixels"));
    m_mergeAction->setText(tr("Merge"));

    m_bannerLabel->setText(
        tr("Ghostscript (gs) was not found on this system. Install the "
           "\"ghostscript\" package or point to the binary in the settings. "
           "You can still add and arrange documents."));
    m_bannerButton->setText(tr("Open Settings"));

    m_cancelButton->setText(tr("Cancel"));

    updateDocumentSummary();

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

void MainWindow::onSizeSliderMoved(int value)
{
    if (m_list->displayMode() == PdfListWidget::Icons) {
        m_list->setThumbnailWidth(value);
    } else {
        m_list->setDetailsIconWidth(value);
    }
}

void MainWindow::syncSizeSlider()
{
    const bool icons = m_list->displayMode() == PdfListWidget::Icons;
    const int minimum = icons ? 80 : 16;
    const int maximum = icons ? 280 : 72;
    const int current = icons ? m_list->thumbnailWidth() : m_list->detailsIconWidth();

    m_sizeSlider->blockSignals(true);
    m_sizeSlider->setRange(minimum, maximum);
    m_sizeSlider->setSingleStep(icons ? 10 : 4);
    m_sizeSlider->setValue(current);
    m_sizeSlider->blockSignals(false);
}

void MainWindow::applyDisplayMode(PdfListWidget::DisplayMode mode)
{
    m_list->setDisplayMode(mode);

    const bool icons = mode == PdfListWidget::Icons;
    m_iconViewAction->setChecked(icons);
    m_detailsViewAction->setChecked(!icons);
    /* The slider drives both modes, each with its own range and value. */
    syncSizeSlider();

    AppSettings::instance().setViewMode(icons ? QStringLiteral("icons")
                                              : QStringLiteral("details"));
    AppSettings::instance().sync();
}

void MainWindow::showAbout()
{
    // Standard Qt about box: application identity first, the framework
    // details behind the About Qt button.
    QMessageBox box(this);
    box.setWindowTitle(tr("About PDF Merge GS"));
    box.setIconPixmap(QIcon(QStringLiteral(":/icons/pdfmergegs.svg")).pixmap(64, 64));
    box.setTextFormat(Qt::RichText);
    box.setText(tr(
        "<h3>PDF Merge GS %1</h3>"
        "<p>Merges PDF documents into a single file with the external "
        "Ghostscript tool.</p>"
        "<p>Built with Qt %2 and the Qt PDF module.</p>"
        "<p>Licensed under the GNU General Public License v3.0 or later.</p>"
        "<p>Ghostscript is a separate program and is not part of this "
        "application.</p>")
        .arg(QApplication::applicationVersion(),
             QString::fromLatin1(qVersion())));

    QPushButton *aboutQtButton = box.addButton(tr("About Qt..."),
                                               QMessageBox::ActionRole);
    box.addButton(QMessageBox::Close);
    box.exec();

    if (box.clickedButton() == aboutQtButton)
        QMessageBox::aboutQt(this, tr("About Qt"));
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

    updateDocumentSummary();
}

void MainWindow::updateDocumentSummary()
{
    const QStringList files = m_list->filePaths();
    qint64 bytes = 0;
    for (const QString &path : files) {
        bytes += QFileInfo(path).size();
    }
    const QString size = QLocale().toString(bytes / 1024.0, 'f', 1)
                         + QStringLiteral(" kB");
    /* English has no plural rule in Qt without a translation file, so the
     * singular gets its own string and the plural uses %n. */
    const QString count = files.size() == 1
                              ? tr("1 PDF file")
                              : tr("%n PDF files", nullptr, files.size());
    m_summaryLabel->setText(count + QStringLiteral(", ") + size);
}

void MainWindow::showAboutQt()
{
    QMessageBox::aboutQt(this, tr("About Qt"));
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
