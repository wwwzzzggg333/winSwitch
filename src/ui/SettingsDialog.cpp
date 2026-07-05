#include "ui/SettingsDialog.h"
#include "ui/HotkeyEdit.h"

#include "core/Config.h"
#include "platform/PlatformCapabilities.h"

#include <QButtonGroup>
#include <QCheckBox>
#include <QDesktopServices>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QTabWidget>
#include <QUrl>
#include <QVBoxLayout>

namespace {

QString capabilityText(CapabilityLevel level, const I18n &i18n) {
    switch (level) {
    case CapabilityLevel::Full:
        return QStringLiteral("✓ ") + i18n.capabilityFull();
    case CapabilityLevel::Partial:
        return QStringLiteral("△ ") + i18n.capabilityPartial();
    case CapabilityLevel::None:
        return QStringLiteral("✗ ") + i18n.capabilityNone();
    }
    return {};
}

QLabel *selectablePathLabel(const QString &path) {
    auto *label = new QLabel(path);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    label->setWordWrap(true);
    return label;
}

} // namespace

SettingsDialog::SettingsDialog(I18n i18n, QWidget *parent) : QWidget(parent), m_i18n(i18n) {
    buildUi();
}

void SettingsDialog::setConfig(const Config &config) {
    m_config = config;
    m_hotkeyEdit->setHotkey(config.hotkey);
    m_thumbnail->setChecked(config.thumbnail);
    m_mruEnabled->setChecked(config.mruEnabled);
    m_excluded->setPlainText(config.excluded.join('\n'));
    m_pinned->setPlainText(config.pinned.join('\n'));
    const QString lc = config.language.trimmed().toLower();
    const int langId = (lc == QStringLiteral("zh")) ? 1 : (lc == QStringLiteral("en")) ? 2 : 0;
    if (auto *btn = m_languageGroup->button(langId)) {
        btn->setChecked(true);
    }
}

void SettingsDialog::buildUi() {
    auto *layout = new QVBoxLayout(this);
    auto *tabs = new QTabWidget;
    tabs->addTab(buildGeneralTab(), m_i18n.settingsTabGeneral());
    tabs->addTab(buildDiagnosticsTab(), m_i18n.settingsTabDiagnostics());
    layout->addWidget(tabs);
}

QWidget *SettingsDialog::buildGeneralTab() {
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
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

    m_mruEnabled = new QCheckBox(m_i18n.mruEnabled());
    layout->addWidget(m_mruEnabled);

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

    layout->addWidget(new QLabel(m_i18n.pinnedAppsLabel()));
    m_pinned = new QPlainTextEdit;
    m_pinned->setFixedHeight(100);
    layout->addWidget(m_pinned);

    auto *ioRow = new QHBoxLayout;
    auto *importBtn = new QPushButton(m_i18n.importConfig());
    auto *exportBtn = new QPushButton(m_i18n.exportConfig());
    connect(importBtn, &QPushButton::clicked, this, &SettingsDialog::onImportConfig);
    connect(exportBtn, &QPushButton::clicked, this, &SettingsDialog::onExportConfig);
    ioRow->addWidget(importBtn);
    ioRow->addWidget(exportBtn);
    ioRow->addStretch();
    layout->addLayout(ioRow);

    auto *saveBtn = new QPushButton(m_i18n.saveButton());
    connect(saveBtn, &QPushButton::clicked, this, &SettingsDialog::saveFromUi);
    layout->addWidget(saveBtn);
    layout->addStretch();
    return page;
}

QWidget *SettingsDialog::buildDiagnosticsTab() {
    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    const PlatformCapabilities caps = queryPlatformCapabilities();
    auto *form = new QFormLayout;

#ifdef MYSWITCHER_VERSION
    form->addRow(m_i18n.diagAppVersion(), new QLabel(QStringLiteral(MYSWITCHER_VERSION)));
#else
    form->addRow(m_i18n.diagAppVersion(), new QLabel(QStringLiteral("unknown")));
#endif
    form->addRow(m_i18n.diagPlatform(), new QLabel(caps.platformName));
    form->addRow(m_i18n.diagSessionType(), new QLabel(caps.sessionType));
    form->addRow(m_i18n.diagHotkey(), new QLabel(capabilityText(caps.hotkey, m_i18n)));
    form->addRow(m_i18n.diagActivate(), new QLabel(capabilityText(caps.activate, m_i18n)));
    form->addRow(m_i18n.diagCloseWindow(), new QLabel(capabilityText(caps.closeWindow, m_i18n)));
    form->addRow(m_i18n.diagIcon(), new QLabel(capabilityText(caps.icon, m_i18n)));
    form->addRow(m_i18n.diagThumbnail(), new QLabel(capabilityText(caps.thumbnail, m_i18n)));
    form->addRow(m_i18n.diagFolderPath(), new QLabel(capabilityText(caps.folderPath, m_i18n)));
    form->addRow(m_i18n.diagConfigPath(), selectablePathLabel(Config::configPath()));
    form->addRow(m_i18n.diagLogPath(), selectablePathLabel(Config::logPath()));
    layout->addLayout(form);

    auto *openDirBtn = new QPushButton(m_i18n.diagOpenDataDir());
    connect(openDirBtn, &QPushButton::clicked, this, []() {
        QDesktopServices::openUrl(QUrl::fromLocalFile(Config::dataDir()));
    });
    layout->addWidget(openDirBtn);
    layout->addStretch();
    return page;
}

Config SettingsDialog::collectFromUi() const {
    Config cfg = m_config;
    cfg.hotkey = m_hotkeyEdit->hotkey();
    cfg.thumbnail = m_thumbnail->isChecked();
    cfg.mruEnabled = m_mruEnabled->isChecked();
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
    cfg.pinned = lines(m_pinned);
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

void SettingsDialog::onImportConfig() {
    const QString path = QFileDialog::getOpenFileName(
        this,
        m_i18n.importConfig(),
        Config::dataDir(),
        m_i18n.configFileFilter());
    if (path.isEmpty()) {
        return;
    }
    Config imported;
    QString err;
    if (!Config::importFrom(path, &imported, &err)) {
        QMessageBox::warning(this, m_i18n.importConfig(), m_i18n.importFailed(err));
        return;
    }
    setConfig(imported);
    QMessageBox::information(this, m_i18n.importConfig(), m_i18n.importSucceeded());
}

void SettingsDialog::onExportConfig() {
    const QString path = QFileDialog::getSaveFileName(
        this,
        m_i18n.exportConfig(),
        Config::dataDir() + QStringLiteral("/mySwitcher-config.json"),
        m_i18n.configFileFilter());
    if (path.isEmpty()) {
        return;
    }
    QString err;
    if (!collectFromUi().exportTo(path, &err)) {
        QMessageBox::warning(this, m_i18n.exportConfig(), m_i18n.exportFailed(err));
        return;
    }
    QMessageBox::information(this, m_i18n.exportConfig(), m_i18n.exportSucceeded(path));
}
