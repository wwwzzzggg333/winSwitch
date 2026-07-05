#pragma once

#include "core/Config.h"

#include <QString>

class I18n {
public:
    enum class Locale { Zh, En };

    static I18n fromConfig(const Config &cfg);

    I18n() : m_locale(Locale::En) {}

    QString appTitle() const;
    QString settingsTitle() const;
    QString settingsWindowTitle() const;
    QString filterAll() const;
    QString pin() const;
    QString pinned() const;
    QString closeAllGroup() const;
    QString closeGroupTooltip() const;
    QString closeWindowTooltip() const;
    QString windowCount(int count) const;
    QString hotkeyLabel() const;
    QString hotkeyHint() const;
    QString hotkeyListening() const;
    QString showThumbnails() const;
    QString mruEnabled() const;
    QString languageLabel() const;
    QString languageAuto() const;
    QString languageZh() const;
    QString languageEn() const;
    QString languageRestartHint() const;
    QString excludedAppsLabel() const;
    QString pinnedAppsLabel() const;
    QString trayOpenSettings() const;
    QString trayQuit() const;
    QString trayTooltip(const QString &hotkey) const;
    QString alreadyRunningMessage() const;
    QString welcomeMessage(const QString &hotkey) const;
    QString hotkeyFailedTitle() const;
    QString hotkeyFailedMessage(const QString &hotkey, const QString &err) const;
    QString hotkeyPlatformUnsupported() const;
    QString hotkeyRolledBack(const QString &oldHotkey, const QString &reason) const;
    QString startupFailedTitle() const;
    QString startupFailedMessage(const QString &glowErr, const QString &wgpuErr, const QString &logPath) const;
    QString searchPlaceholder() const;
    QString settingsTabGeneral() const;
    QString settingsTabDiagnostics() const;
    QString saveButton() const;
    QString diagAppVersion() const;
    QString diagPlatform() const;
    QString diagSessionType() const;
    QString diagHotkey() const;
    QString diagActivate() const;
    QString diagCloseWindow() const;
    QString diagIcon() const;
    QString diagThumbnail() const;
    QString diagFolderPath() const;
    QString diagConfigPath() const;
    QString diagLogPath() const;
    QString diagOpenDataDir() const;
    QString capabilityFull() const;
    QString capabilityPartial() const;
    QString capabilityNone() const;
    QString importConfig() const;
    QString exportConfig() const;
    QString configFileFilter() const;
    QString exportSucceeded(const QString &path) const;
    QString exportFailed(const QString &err) const;
    QString importSucceeded() const;
    QString importFailed(const QString &err) const;

private:
    explicit I18n(Locale locale);

    Locale m_locale;
};
