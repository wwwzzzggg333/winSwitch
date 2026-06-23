#pragma once

#include "core/Config.h"

class I18n {
public:
    enum class Locale { Zh, En };

    static I18n fromConfig(const Config &cfg);

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
    QString welcomeMessage(const QString &hotkey, const QString &logPath) const;
    QString hotkeyFailedTitle() const;
    QString hotkeyFailedMessage(const QString &hotkey, const QString &err) const;
    QString startupFailedTitle() const;
    QString startupFailedMessage(const QString &glowErr, const QString &wgpuErr, const QString &logPath) const;

private:
    explicit I18n(Locale locale);

    Locale m_locale;
};
