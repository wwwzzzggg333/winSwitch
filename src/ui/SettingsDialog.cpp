#include "ui/SettingsDialog.h"
#include "ui/HotkeyEdit.h"

#include "core/Config.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
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
    m_startAtLogin->setChecked(config.startAtLogin);
    m_excluded->setPlainText(config.excluded.join('\n'));
    const QString lc = config.language.trimmed().toLower();
    const int langId = (lc == QStringLiteral("zh")) ? 1 : (lc == QStringLiteral("en")) ? 2 : 0;
    if (auto *btn = m_languageGroup->button(langId)) {
        btn->setChecked(true);
    }
}

void SettingsDialog::buildUi() {
    auto *layout = new QVBoxLayout(this);
    layout->addWidget(buildGeneralPage());
}

QWidget *SettingsDialog::buildGeneralPage() {
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);

    auto *form = new QFormLayout;
    m_hotkeyEdit = new HotkeyEdit(m_i18n);
    form->addRow(m_i18n.hotkeyLabel(), m_hotkeyEdit);
    layout->addLayout(form);
    layout->addWidget(new QLabel(m_i18n.hotkeyHint()));

    m_thumbnail = new QCheckBox(m_i18n.showThumbnails());
    layout->addWidget(m_thumbnail);

    m_startAtLogin = new QCheckBox(m_i18n.startAtLogin());
    layout->addWidget(m_startAtLogin);

    auto *langBox = new QGroupBox(m_i18n.languageLabel());
    auto *langLayout = new QHBoxLayout(langBox);
    m_languageGroup = new QButtonGroup(page);
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

    auto *saveBtn = new QPushButton(m_i18n.saveButton());
    connect(saveBtn, &QPushButton::clicked, this, &SettingsDialog::saveFromUi);
    layout->addWidget(saveBtn);
    layout->addStretch();
    return page;
}

Config SettingsDialog::collectFromUi() const {
    Config cfg = m_config;
    cfg.hotkey = m_hotkeyEdit->hotkey();
    cfg.thumbnail = m_thumbnail->isChecked();
    cfg.startAtLogin = m_startAtLogin->isChecked();
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
    cfg.excluded = lines(m_excluded);
    switch (m_languageGroup->checkedId()) {
    case 1:
        cfg.language = QStringLiteral("zh");
        break;
    case 2:
        cfg.language = QStringLiteral("en");
        break;
    default:
        cfg.language = QStringLiteral("auto");
        break;
    }
    return cfg;
}

void SettingsDialog::saveFromUi() {
    m_config = collectFromUi();
    emit saved(m_config);
}
