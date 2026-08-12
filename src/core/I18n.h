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
    QString closeAllGroup() const;
    QString closeGroupTooltip() const;
    QString closeWindowTooltip() const;
    QString previewUnavailable() const;
    QString noSwitchableWindows() const;
    QString noMatchingWindows() const;
    QString emptyWindowsHint() const;
    QString noMatchingWindowsHint() const;
    QString windowCount(int count) const;
    QString hotkeyLabel() const;
    QString hotkeyHint() const;
    QString hotkeyListening() const;
    QString showThumbnails() const;
    QString startAtLogin() const;
    QString startAtLoginFailed(const QString &reason) const;
    QString mruEnabled() const;
    QString languageLabel() const;
    QString languageAuto() const;
    QString languageZh() const;
    QString languageEn() const;
    QString languageRestartHint() const;
    QString excludedAppsLabel() const;
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
    QString saveButton() const;

private:
    explicit I18n(Locale locale);

    Locale m_locale;
};
