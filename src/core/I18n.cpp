#include "core/I18n.h"

#include <QLocale>

namespace {

I18n::Locale detectSystemLocale() {
    const QLocale locale;
    switch (locale.language()) {
    case QLocale::Chinese:
        return I18n::Locale::Zh;
    default:
        return I18n::Locale::En;
    }
}

I18n::Locale resolveLocale(const QString &language) {
    const QString lc = language.trimmed().toLower();
    if (lc == QStringLiteral("zh")) {
        return I18n::Locale::Zh;
    }
    if (lc == QStringLiteral("en")) {
        return I18n::Locale::En;
    }
    return detectSystemLocale();
}

} // namespace

I18n I18n::fromConfig(const Config &cfg) {
    return I18n(resolveLocale(cfg.language));
}

I18n::I18n(Locale locale) : m_locale(locale) {}

QString I18n::appTitle() const {
    return m_locale == Locale::Zh ? QStringLiteral("窗口切换管理器") : QStringLiteral("Window Switcher");
}

QString I18n::settingsTitle() const {
    return m_locale == Locale::Zh ? QStringLiteral("设置") : QStringLiteral("Settings");
}

QString I18n::settingsWindowTitle() const {
    return settingsTitle();
}

QString I18n::filterAll() const {
    return m_locale == Locale::Zh ? QStringLiteral("全部") : QStringLiteral("All");
}

QString I18n::pin() const {
    return m_locale == Locale::Zh ? QStringLiteral("置顶") : QStringLiteral("Pin");
}

QString I18n::pinned() const {
    return m_locale == Locale::Zh ? QStringLiteral("已置顶") : QStringLiteral("Pinned");
}

QString I18n::closeAllGroup() const {
    return m_locale == Locale::Zh ? QStringLiteral("关全部") : QStringLiteral("Close all");
}

QString I18n::closeGroupTooltip() const {
    return m_locale == Locale::Zh ? QStringLiteral("关闭该应用的全部窗口") : QStringLiteral("Close all windows of this app");
}

QString I18n::closeWindowTooltip() const {
    return m_locale == Locale::Zh ? QStringLiteral("关闭窗口") : QStringLiteral("Close window");
}

QString I18n::windowCount(int count) const {
    return QStringLiteral("(%1)").arg(count);
}

QString I18n::hotkeyLabel() const {
    return m_locale == Locale::Zh ? QStringLiteral("全局快捷键") : QStringLiteral("Global hotkey");
}

QString I18n::hotkeyHint() const {
    return m_locale == Locale::Zh
        ? QStringLiteral("点击后按下组合键；保存后立即生效")
        : QStringLiteral("Click and press a combination; applies immediately after saving");
}

QString I18n::hotkeyListening() const {
    return m_locale == Locale::Zh ? QStringLiteral("请按键… (Esc 取消)") : QStringLiteral("Press keys… (Esc to cancel)");
}

QString I18n::showThumbnails() const {
    return m_locale == Locale::Zh ? QStringLiteral("显示窗口缩略图") : QStringLiteral("Show window thumbnails");
}

QString I18n::mruEnabled() const {
    return m_locale == Locale::Zh ? QStringLiteral("按最近使用排序") : QStringLiteral("Sort by recently used");
}

QString I18n::languageLabel() const {
    return m_locale == Locale::Zh ? QStringLiteral("界面语言") : QStringLiteral("Language");
}

QString I18n::languageAuto() const {
    return m_locale == Locale::Zh ? QStringLiteral("跟随系统") : QStringLiteral("System");
}

QString I18n::languageZh() const {
    return QStringLiteral("中文");
}

QString I18n::languageEn() const {
    return QStringLiteral("English");
}

QString I18n::languageRestartHint() const {
    return m_locale == Locale::Zh ? QStringLiteral("语言变更需重启生效。") : QStringLiteral("Restart required for language change.");
}

QString I18n::excludedAppsLabel() const {
    return m_locale == Locale::Zh ? QStringLiteral("排除应用（每行一个 exe 文件名）")
                                    : QStringLiteral("Excluded apps (one exe name per line)");
}

QString I18n::pinnedAppsLabel() const {
    return m_locale == Locale::Zh ? QStringLiteral("置顶应用（每行一个 exe 文件名）")
                                    : QStringLiteral("Pinned apps (one exe name per line)");
}

QString I18n::trayOpenSettings() const {
    return m_locale == Locale::Zh ? QStringLiteral("打开配置") : QStringLiteral("Settings");
}

QString I18n::trayQuit() const {
    return m_locale == Locale::Zh ? QStringLiteral("退出") : QStringLiteral("Quit");
}

QString I18n::trayTooltip(const QString &hotkey) const {
    return m_locale == Locale::Zh
        ? QStringLiteral("窗口切换管理器 — 按 %1 唤出面板").arg(hotkey)
        : QStringLiteral("Window Switcher — press %1 to open panel").arg(hotkey);
}

QString I18n::alreadyRunningMessage() const {
    return m_locale == Locale::Zh ? QStringLiteral("程序已在运行。") : QStringLiteral("Application is already running.");
}

QString I18n::welcomeMessage(const QString &hotkey) const {
    if (m_locale == Locale::Zh) {
        return QStringLiteral(
            "程序已在后台运行。\n"
            "按 %1 唤出/隐藏切换面板。\n"
            "右键托盘图标可打开配置或退出。")
            .arg(hotkey);
    }
    return QStringLiteral(
        "Running in the background.\n"
        "Press %1 to show/hide the switcher panel.\n"
        "Right-click the tray icon for settings or quit.")
        .arg(hotkey);
}

QString I18n::hotkeyFailedTitle() const {
    return m_locale == Locale::Zh ? QStringLiteral("快捷键注册失败") : QStringLiteral("Hotkey registration failed");
}

QString I18n::hotkeyFailedMessage(const QString &hotkey, const QString &err) const {
    return m_locale == Locale::Zh
        ? QStringLiteral("无法注册快捷键「%1」：\n%2").arg(hotkey, err)
        : QStringLiteral("Failed to register hotkey \"%1\":\n%2").arg(hotkey, err);
}

QString I18n::hotkeyPlatformUnsupported() const {
    return m_locale == Locale::Zh
        ? QStringLiteral("当前平台尚未支持全局热键，请通过托盘图标唤出面板")
        : QStringLiteral("Global hotkeys are not supported on this platform yet. Use the tray icon to open the panel.");
}

QString I18n::hotkeyRolledBack(const QString &oldHotkey, const QString &reason) const {
    return m_locale == Locale::Zh
        ? QStringLiteral("新热键设置失败，已恢复为 %1。原因：%2").arg(oldHotkey, reason)
        : QStringLiteral("Failed to apply the new hotkey; restored %1. Reason: %2").arg(oldHotkey, reason);
}

QString I18n::startupFailedTitle() const {
    return m_locale == Locale::Zh ? QStringLiteral("启动失败") : QStringLiteral("Startup failed");
}

QString I18n::startupFailedMessage(const QString &glowErr, const QString &wgpuErr, const QString &logPath) const {
    Q_UNUSED(wgpuErr)
    return m_locale == Locale::Zh
        ? QStringLiteral("程序启动失败：\n%1\n\n日志：%2").arg(glowErr, logPath)
        : QStringLiteral("Failed to start:\n%1\n\nLog: %2").arg(glowErr, logPath);
}

QString I18n::searchPlaceholder() const {
    return m_locale == Locale::Zh ? QStringLiteral("搜索窗口标题、应用名或路径…")
                                  : QStringLiteral("Search title, app or path...");
}

QString I18n::settingsTabGeneral() const {
    return m_locale == Locale::Zh ? QStringLiteral("常规") : QStringLiteral("General");
}

QString I18n::settingsTabDiagnostics() const {
    return m_locale == Locale::Zh ? QStringLiteral("诊断") : QStringLiteral("Diagnostics");
}

QString I18n::saveButton() const {
    return m_locale == Locale::Zh ? QStringLiteral("保存") : QStringLiteral("Save");
}

QString I18n::diagAppVersion() const {
    return m_locale == Locale::Zh ? QStringLiteral("应用版本") : QStringLiteral("App version");
}

QString I18n::diagPlatform() const {
    return m_locale == Locale::Zh ? QStringLiteral("平台") : QStringLiteral("Platform");
}

QString I18n::diagSessionType() const {
    return m_locale == Locale::Zh ? QStringLiteral("会话类型") : QStringLiteral("Session type");
}

QString I18n::diagHotkey() const {
    return m_locale == Locale::Zh ? QStringLiteral("全局热键") : QStringLiteral("Global hotkey");
}

QString I18n::diagActivate() const {
    return m_locale == Locale::Zh ? QStringLiteral("激活窗口") : QStringLiteral("Activate window");
}

QString I18n::diagCloseWindow() const {
    return m_locale == Locale::Zh ? QStringLiteral("关闭窗口") : QStringLiteral("Close window");
}

QString I18n::diagIcon() const {
    return m_locale == Locale::Zh ? QStringLiteral("窗口图标") : QStringLiteral("Window icon");
}

QString I18n::diagThumbnail() const {
    return m_locale == Locale::Zh ? QStringLiteral("窗口缩略图") : QStringLiteral("Window thumbnail");
}

QString I18n::diagFolderPath() const {
    return m_locale == Locale::Zh ? QStringLiteral("文件夹路径") : QStringLiteral("Folder path");
}

QString I18n::diagConfigPath() const {
    return m_locale == Locale::Zh ? QStringLiteral("配置文件") : QStringLiteral("Config file");
}

QString I18n::diagLogPath() const {
    return m_locale == Locale::Zh ? QStringLiteral("日志文件") : QStringLiteral("Log file");
}

QString I18n::diagOpenDataDir() const {
    return m_locale == Locale::Zh ? QStringLiteral("打开数据目录") : QStringLiteral("Open data folder");
}

QString I18n::capabilityFull() const {
    return m_locale == Locale::Zh ? QStringLiteral("支持") : QStringLiteral("Supported");
}

QString I18n::capabilityPartial() const {
    return m_locale == Locale::Zh ? QStringLiteral("部分支持") : QStringLiteral("Partial");
}

QString I18n::capabilityNone() const {
    return m_locale == Locale::Zh ? QStringLiteral("不支持") : QStringLiteral("Not supported");
}

QString I18n::importConfig() const {
    return m_locale == Locale::Zh ? QStringLiteral("导入配置…") : QStringLiteral("Import config...");
}

QString I18n::exportConfig() const {
    return m_locale == Locale::Zh ? QStringLiteral("导出配置…") : QStringLiteral("Export config...");
}

QString I18n::configFileFilter() const {
    return m_locale == Locale::Zh ? QStringLiteral("JSON 配置文件 (*.json)")
                                  : QStringLiteral("JSON config files (*.json)");
}

QString I18n::exportSucceeded(const QString &path) const {
    return m_locale == Locale::Zh ? QStringLiteral("配置已导出到 %1").arg(path)
                                  : QStringLiteral("Config exported to %1").arg(path);
}

QString I18n::exportFailed(const QString &err) const {
    return m_locale == Locale::Zh ? QStringLiteral("导出失败：%1").arg(err)
                                  : QStringLiteral("Export failed: %1").arg(err);
}

QString I18n::importSucceeded() const {
    return m_locale == Locale::Zh ? QStringLiteral("配置已载入，点击保存后生效")
                                  : QStringLiteral("Config loaded. Click Save to apply.");
}

QString I18n::importFailed(const QString &err) const {
    return m_locale == Locale::Zh ? QStringLiteral("导入失败：%1").arg(err)
                                  : QStringLiteral("Import failed: %1").arg(err);
}
