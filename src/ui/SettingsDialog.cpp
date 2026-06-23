#include "ui/SettingsDialog.h"
#include "ui/HotkeyEdit.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>

SettingsDialog::SettingsDialog(I18n i18n, QWidget *parent) : QWidget(parent), m_i18n(i18n) {
    buildUi();
}

void SettingsDialog::setConfig(const Config &config) {
    m_config = config;
    m_hotkeyEdit->setHotkey(config.hotkey);
    m_thumbnail->setChecked(config.thumbnail);
    m_excluded->setPlainText(config.excluded.join('\n'));
    m_pinned->setPlainText(config.pinned.join('\n'));
}

void SettingsDialog::buildUi() {
    auto *layout = new QVBoxLayout(this);
    auto *title = new QLabel(m_i18n.settingsTitle());
    title->setObjectName(QStringLiteral("SettingsTitle"));
    layout->addWidget(title);

    auto *form = new QFormLayout;
    m_hotkeyEdit = new HotkeyEdit(m_i18n);
    form->addRow(m_i18n.hotkeyLabel(), m_hotkeyEdit);
    layout->addLayout(form);
    layout->addWidget(new QLabel(m_i18n.hotkeyHint()));

    m_thumbnail = new QCheckBox(m_i18n.showThumbnails());
    layout->addWidget(m_thumbnail);

    auto *langBox = new QGroupBox(m_i18n.languageLabel());
    auto *langLayout = new QHBoxLayout(langBox);
    m_languageGroup = new QButtonGroup(this);
    auto *autoBtn = new QRadioButton(m_i18n.languageAuto());
    auto *zhBtn = new QRadioButton(m_i18n.languageZh());
    auto *enBtn = new QRadioButton(m_i18n.languageEn());
    m_languageGroup->addButton(autoBtn, 0);
    m_languageGroup->addButton(zhBtn, 1);
    m_languageGroup->addButton(enBtn, 2);
    langLayout->addWidget(autoBtn);
    langLayout->addWidget(zhBtn);
    langLayout->addWidget(enBtn);
    layout->addWidget(langBox);
    layout->addWidget(new QLabel(m_i18n.languageRestartHint()));

    layout->addWidget(new QLabel(m_i18n.excludedAppsLabel()));
    m_excluded = new QPlainTextEdit;
    m_excluded->setFixedHeight(100);
    layout->addWidget(m_excluded);

    layout->addWidget(new QLabel(m_i18n.pinnedAppsLabel()));
    m_pinned = new QPlainTextEdit;
    m_pinned->setFixedHeight(100);
    layout->addWidget(m_pinned);

    auto *saveBtn = new QPushButton(QStringLiteral("Save"));
    connect(saveBtn, &QPushButton::clicked, this, &SettingsDialog::saveFromUi);
    layout->addWidget(saveBtn);
    layout->addStretch();
}

void SettingsDialog::saveFromUi() {
    m_config.hotkey = m_hotkeyEdit->hotkey();
    m_config.thumbnail = m_thumbnail->isChecked();
    const auto lines = [](QPlainTextEdit *edit) {
        QStringList out;
        for (const QString &line : edit->toPlainText().split('\n')) {
            const QString trimmed = line.trimmed();
            if (!trimmed.isEmpty()) {
                out.append(trimmed);
            }
        }
        return out;
    };
    m_config.excluded = lines(m_excluded);
    m_config.pinned = lines(m_pinned);
    switch (m_languageGroup->checkedId()) {
    case 1:
        m_config.language = QStringLiteral("zh");
        break;
    case 2:
        m_config.language = QStringLiteral("en");
        break;
    default:
        m_config.language = QStringLiteral("auto");
        break;
    }
    emit saved(m_config);
}