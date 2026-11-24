// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "PdfListWidget.h"

#include <QMainWindow>
#include <QStringList>

class GhostscriptRunner;
class QAction;
class QFrame;
class QLabel;
class QMenu;
class QSlider;
class QProgressBar;
class QPushButton;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

    /* Files passed on the command line (Dolphin "Open With", terminal). */
    void addFilesFromCommandLine(const QStringList &paths);

protected:
    void changeEvent(QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private:
    void buildUi();
    void retranslateUi();
    void refreshState();
    void updateGhostscriptBanner();

    void addFilesViaDialog();
    void removeSelected();
    void clearAll();
    void openSettings();
    void showAbout();
    void showAboutQt();
    void updateDocumentSummary();
    void onSizeSliderMoved(int value);
    void syncSizeSlider();
    void applyDisplayMode(PdfListWidget::DisplayMode mode);
    void startMerge();
    void cancelMerge();

    bool confirmSignedDocuments();
    void setBusy(bool busy);
    void onMergeFinished(bool success, int exitCode, const QString &errorText);
    void onMergeFailedToStart(const QString &message);
    void showMergeSuccess(const QString &outputPath);
    void showMergeFailure(const QString &details);

    PdfListWidget *m_list = nullptr;
    QMenu *m_fileMenu = nullptr;
    QMenu *m_viewMenu = nullptr;
    QMenu *m_settingsMenu = nullptr;
    QMenu *m_helpMenu = nullptr;
    QFrame *m_banner = nullptr;
    QLabel *m_bannerLabel = nullptr;
    QPushButton *m_bannerButton = nullptr;
    QWidget *m_progressRow = nullptr;
    QProgressBar *m_progress = nullptr;
    QPushButton *m_cancelButton = nullptr;
    GhostscriptRunner *m_runner = nullptr;

    QAction *m_addAction = nullptr;
    QAction *m_removeAction = nullptr;
    QAction *m_clearAction = nullptr;
    QAction *m_settingsAction = nullptr;
    QAction *m_aboutAction = nullptr;
    QAction *m_aboutQtAction = nullptr;
    QAction *m_quitAction = nullptr;
    QAction *m_toggleMenuBarAction = nullptr;
    QAction *m_iconViewAction = nullptr;
    QAction *m_detailsViewAction = nullptr;
    QAction *m_fullPathAction = nullptr;
    QLabel *m_sizeLabel = nullptr;
    QLabel *m_summaryLabel = nullptr;
    QSlider *m_sizeSlider = nullptr;
    QAction *m_mergeAction = nullptr;

    QString m_lastOutputPath;
    bool m_mergeCancelled = false;
};
