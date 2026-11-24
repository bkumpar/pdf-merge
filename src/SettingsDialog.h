// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "AppSettings.h"

#include <QDialog>

class QComboBox;
class QDialogButtonBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;

/* Interface language, Ghostscript binary and merge arguments. */
class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);

    /* True when the interface language was changed by this dialog. */
    bool languageChanged() const { return m_languageChanged; }

protected:
    void changeEvent(QEvent *event) override;

private:
    void buildUi();
    void loadFromSettings();
    void retranslateUi();
    void updateGhostscriptHint();
    void updateArgumentsEnabled();

    void browseForGhostscript();
    void presetSelected(int index);
    void restoreDefaults();
    void saveAndClose();

    QLabel *m_languageLabel = nullptr;
    QComboBox *m_languageCombo = nullptr;

    QLabel *m_binaryLabel = nullptr;
    QLineEdit *m_binaryEdit = nullptr;
    QPushButton *m_browseButton = nullptr;
    QLabel *m_binaryHint = nullptr;

    QLabel *m_presetLabel = nullptr;
    QComboBox *m_presetCombo = nullptr;
    QLabel *m_argumentsLabel = nullptr;
    QPlainTextEdit *m_argumentsEdit = nullptr;

    QDialogButtonBox *m_buttonBox = nullptr;
    QPushButton *m_saveButton = nullptr;
    QPushButton *m_defaultsButton = nullptr;
    QPushButton *m_cancelButton = nullptr;

    bool m_languageChanged = false;
};
