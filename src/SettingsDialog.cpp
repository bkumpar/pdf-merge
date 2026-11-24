// SPDX-License-Identifier: GPL-3.0-or-later
#include "SettingsDialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QProcess>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStandardPaths>
#include <QVBoxLayout>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    buildUi();
    loadFromSettings();
    retranslateUi();
    updateGhostscriptHint();
}

void SettingsDialog::buildUi()
{
    setModal(true);
    setMinimumWidth(560);

    auto *layout = new QVBoxLayout(this);

    auto *interfaceBox = new QGroupBox(this);
    auto *interfaceForm = new QFormLayout(interfaceBox);
    m_languageCombo = new QComboBox(interfaceBox);
    m_languageLabel = new QLabel(interfaceBox);
    m_languageLabel->setBuddy(m_languageCombo);
    interfaceForm->addRow(m_languageLabel, m_languageCombo);
    layout->addWidget(interfaceBox);

    auto *ghostscriptBox = new QGroupBox(this);
    auto *ghostscriptForm = new QFormLayout(ghostscriptBox);

    auto *binaryRow = new QWidget(ghostscriptBox);
    auto *binaryLayout = new QHBoxLayout(binaryRow);
    binaryLayout->setContentsMargins(0, 0, 0, 0);
    m_binaryEdit = new QLineEdit(binaryRow);
    m_binaryEdit->setPlaceholderText(
        QStandardPaths::findExecutable(QStringLiteral("gs")));
    m_browseButton = new QPushButton(binaryRow);
    binaryLayout->addWidget(m_binaryEdit, 1);
    binaryLayout->addWidget(m_browseButton);
    m_binaryLabel = new QLabel(ghostscriptBox);
    m_binaryLabel->setBuddy(m_binaryEdit);
    ghostscriptForm->addRow(m_binaryLabel, binaryRow);

    m_binaryHint = new QLabel(ghostscriptBox);
    m_binaryHint->setWordWrap(true);
    ghostscriptForm->addRow(QString(), m_binaryHint);

    m_presetCombo = new QComboBox(ghostscriptBox);
    m_presetLabel = new QLabel(ghostscriptBox);
    m_presetLabel->setBuddy(m_presetCombo);
    ghostscriptForm->addRow(m_presetLabel, m_presetCombo);

    m_argumentsEdit = new QPlainTextEdit(ghostscriptBox);
    m_argumentsEdit->setMinimumHeight(120);
    m_argumentsEdit->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    m_argumentsLabel = new QLabel(ghostscriptBox);
    m_argumentsLabel->setBuddy(m_argumentsEdit);
    ghostscriptForm->addRow(m_argumentsLabel, m_argumentsEdit);

    layout->addWidget(ghostscriptBox);
    layout->addStretch(1);

    m_buttonBox = new QDialogButtonBox(this);
    m_saveButton = m_buttonBox->addButton(QDialogButtonBox::Save);
    m_defaultsButton = m_buttonBox->addButton(QDialogButtonBox::RestoreDefaults);
    m_cancelButton = m_buttonBox->addButton(QDialogButtonBox::Cancel);
    layout->addWidget(m_buttonBox);

    connect(m_browseButton, &QPushButton::clicked, this,
            &SettingsDialog::browseForGhostscript);
    connect(m_presetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsDialog::presetSelected);
    connect(m_saveButton, &QPushButton::clicked, this,
            &SettingsDialog::saveAndClose);
    connect(m_defaultsButton, &QPushButton::clicked, this,
            &SettingsDialog::restoreDefaults);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_binaryEdit, &QLineEdit::textChanged, this,
            &SettingsDialog::updateGhostscriptHint);
}

void SettingsDialog::loadFromSettings()
{
    AppSettings &settings = AppSettings::instance();

    m_languageCombo->clear();
    m_languageCombo->addItem(QString(), static_cast<int>(AppSettings::Language::System));
    m_languageCombo->addItem(QString(), static_cast<int>(AppSettings::Language::English));
    m_languageCombo->addItem(QString(), static_cast<int>(AppSettings::Language::Croatian));
    m_languageCombo->setCurrentIndex(
        m_languageCombo->findData(static_cast<int>(settings.language())));

    m_binaryEdit->setText(settings.gsPath());

    m_presetCombo->clear();
    for (int index = static_cast<int>(AppSettings::Preset::Lossless);
         index <= static_cast<int>(AppSettings::Preset::Custom); ++index) {
        m_presetCombo->addItem(QString(), index);
    }
    m_presetCombo->setCurrentIndex(
        m_presetCombo->findData(static_cast<int>(settings.preset())));

    m_argumentsEdit->setPlainText(settings.gsArguments().join(QLatin1Char(' ')));
    updateArgumentsEnabled();
}

void SettingsDialog::retranslateUi()
{
    setWindowTitle(tr("Ghostscript Settings"));

    if (auto *box = qobject_cast<QGroupBox *>(m_languageCombo->parentWidget())) {
        box->setTitle(tr("Interface"));
    }
    if (auto *box = qobject_cast<QGroupBox *>(m_argumentsEdit->parentWidget())) {
        box->setTitle(tr("Ghostscript"));
    }

    m_languageLabel->setText(tr("Language:"));
    const int languageIndex = m_languageCombo->currentIndex();
    m_languageCombo->setItemText(0, tr("System Default"));
    m_languageCombo->setItemText(1, tr("English"));
    m_languageCombo->setItemText(2, tr("Hrvatski"));
    m_languageCombo->setCurrentIndex(languageIndex);

    m_binaryLabel->setText(tr("Ghostscript binary:"));
    m_browseButton->setText(tr("Browse..."));
    m_binaryEdit->setPlaceholderText(tr("Detected automatically (gs on PATH)"));

    m_presetLabel->setText(tr("Preset:"));
    const int presetIndex = m_presetCombo->currentIndex();
    for (int index = 0; index < m_presetCombo->count(); ++index) {
        const auto preset =
            static_cast<AppSettings::Preset>(m_presetCombo->itemData(index).toInt());
        m_presetCombo->setItemText(index, AppSettings::presetName(preset));
    }
    m_presetCombo->setCurrentIndex(presetIndex);

    m_argumentsLabel->setText(tr("Arguments:"));
    m_argumentsEdit->setPlaceholderText(
        tr("Ghostscript arguments, one option per word"));

    m_saveButton->setText(tr("Save Settings"));
    m_defaultsButton->setText(tr("Restore Defaults"));
    m_cancelButton->setText(tr("Cancel"));

    updateGhostscriptHint();
}

void SettingsDialog::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QDialog::changeEvent(event);
}

void SettingsDialog::updateArgumentsEnabled()
{
    const bool custom = m_presetCombo->currentData().toInt()
                        == static_cast<int>(AppSettings::Preset::Custom);
    m_argumentsEdit->setReadOnly(!custom);
}

void SettingsDialog::updateGhostscriptHint()
{
    const QString custom = m_binaryEdit->text().trimmed();
    QString resolved;
    if (!custom.isEmpty()) {
        const QFileInfo info(custom);
        if (info.exists() && info.isFile() && info.isExecutable()) {
            resolved = info.absoluteFilePath();
        }
    } else {
        resolved = QStandardPaths::findExecutable(QStringLiteral("gs"));
    }

    if (resolved.isEmpty()) {
        m_binaryHint->setText(
            tr("Ghostscript was not found. Install the \"ghostscript\" package "
               "or choose the binary manually."));
        m_binaryHint->setStyleSheet(QStringLiteral("color: #b3261e;"));
    } else {
        m_binaryHint->setText(tr("Using: %1").arg(resolved));
        m_binaryHint->setStyleSheet(QStringLiteral("color: #1b6e2f;"));
    }
}

void SettingsDialog::browseForGhostscript()
{
    const QString startDir = m_binaryEdit->text().trimmed().isEmpty()
                                 ? QStringLiteral("/usr/bin")
                                 : QFileInfo(m_binaryEdit->text().trimmed()).absolutePath();
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Select Ghostscript binary"), startDir, tr("All files (*)"));
    if (!path.isEmpty()) {
        m_binaryEdit->setText(path);
    }
}

void SettingsDialog::presetSelected(int index)
{
    Q_UNUSED(index);
    const auto preset =
        static_cast<AppSettings::Preset>(m_presetCombo->currentData().toInt());
    if (preset != AppSettings::Preset::Custom) {
        m_argumentsEdit->setPlainText(
            AppSettings::presetArguments(preset).join(QLatin1Char(' ')));
    }
    updateArgumentsEnabled();
}

void SettingsDialog::restoreDefaults()
{
    AppSettings &settings = AppSettings::instance();
    settings.restoreDefaults();

    m_languageCombo->setCurrentIndex(0);
    m_binaryEdit->clear();
    m_presetCombo->setCurrentIndex(0);
    m_argumentsEdit->setPlainText(
        AppSettings::presetArguments(AppSettings::Preset::Lossless)
            .join(QLatin1Char(' ')));
    updateArgumentsEnabled();
    updateGhostscriptHint();
}

void SettingsDialog::saveAndClose()
{
    AppSettings &settings = AppSettings::instance();

    const auto selectedLanguage =
        static_cast<AppSettings::Language>(m_languageCombo->currentData().toInt());
    if (selectedLanguage != settings.language()) {
        m_languageChanged = true;
    }
    settings.setLanguage(selectedLanguage);

    settings.setGsPath(m_binaryEdit->text().trimmed());

    const auto preset =
        static_cast<AppSettings::Preset>(m_presetCombo->currentData().toInt());
    settings.setPreset(preset);

    const QString arguments = m_argumentsEdit->toPlainText().trimmed();
    settings.setGsArguments(QProcess::splitCommand(arguments));

    settings.sync();
    accept();
}
