#include "app/Application.h"
#include "app/HotkeyManager.h"

#include "core/AppLog.h"

#include "ui/AppMessageBox.h"

#include <QApplication>
#include <QDateTime>
#include <QMenu>
#include <QSet>
#include <QStyle>
#include <QTimer>

ApplicationController::ApplicationController(QObject *parent) : QObject(parent) {}

ApplicationController::~ApplicationController() {
    if (m_hotkey) {
        m_hotkey->unregisterHotkey();
    }
}

void ApplicationController::showHotkeyFailure(
    const QString &hotkey,
    const QString &err,
    HotkeyManager::HotkeyError kind) {
    const QString message = kind == HotkeyManager::HotkeyError::PlatformUnsupported
        ? m_i18n.hotkeyPlatformUnsupported()
        : m_i18n.hotkeyFailedMessage(hotkey, err);
    showAppMessage(m_mainWindow, m_i18n.hotkeyFailedTitle(), message, AppMessageIcon::Warning);
}

bool ApplicationController::initialize() {
    m_config = Config::load();
    m_i18n = I18n::fromConfig(m_config);
    m_windowSource = createWindowSource();
    m_iconCapture = createIconCapture();
    m_thumbnailCapture = createThumbnailCapture();

    m_mainWindow = new MainWindow(m_config, m_i18n);
    m_mainWindow->hide();

    m_tray = new QSystemTrayIcon(QApplication::style()->standardIcon(QStyle::SP_ComputerIcon), this);
    m_tray->setToolTip(m_i18n.trayTooltip(m_config.hotkey));

    auto *menu = new QMenu;
    menu->addAction(m_i18n.trayOpenSettings(), this, &ApplicationController::onOpenSettings);
    menu->addSeparator();
    menu->addAction(m_i18n.trayQuit(), this, &ApplicationController::onQuit);
    m_tray->setContextMenu(menu);
    m_tray->show();

    connect(m_tray, &QSystemTrayIcon::activated, this, &ApplicationController::onTrayActivated);
    connect(m_mainWindow, &MainWindow::panelActionRequested, this, &ApplicationController::onPanelAction);
    connect(m_mainWindow, &MainWindow::settingsSaved, this, &ApplicationController::onSettingsSaved);
    connect(m_mainWindow, &MainWindow::focusLost, this, [this]() {
        if (m_view == View::Panel) {
            hideAll();
        }
    });

    m_hotkey = new HotkeyManager(this);
    connect(m_hotkey, &HotkeyManager::activated, this, &ApplicationController::onHotkeyActivated);
    QString hotkeyError;
    if (!m_hotkey->registerHotkey(m_config.hotkey, &hotkeyError)) {
        showHotkeyFailure(m_config.hotkey, hotkeyError, m_hotkey->lastError());
    }

    if (!Config::welcomeShown()) {
        Config::markWelcomeShown();
        showAppMessage(
            nullptr,
            m_i18n.appTitle(),
            m_i18n.welcomeMessage(m_config.hotkey),
            AppMessageIcon::Information);
    }

    return true;
}

void ApplicationController::onHotkeyActivated() {
    if (m_view == View::Panel && m_mainWindow->isVisible()) {
        hideAll();
    } else {
        showPanel();
    }
}

void ApplicationController::onTrayActivated(QSystemTrayIcon::ActivationReason reason) {
    if (reason == QSystemTrayIcon::DoubleClick) {
        showPanel();
    }
}

void ApplicationController::onOpenSettings() {
    showSettings();
}

void ApplicationController::onQuit() {
    QApplication::quit();
}

void ApplicationController::showPanel() {
    AppLog::info(QStringLiteral("showPanel: refresh windows"));
    refreshWindows();
    m_view = View::Panel;
    AppLog::info(QStringLiteral("showPanel: displaying %1 groups").arg(m_state.groups.size()));
    m_mainWindow->showPanel(m_state, m_icons, m_thumbs, m_config.thumbnail);
    QTimer::singleShot(0, this, &ApplicationController::loadTexturesBatch);
}

void ApplicationController::showSettings() {
    m_view = View::Settings;
    m_mainWindow->showSettings(m_config);
}

void ApplicationController::hideAll() {
    AppLog::info(QStringLiteral("hideAll"));
    m_view = View::Hidden;
    m_mainWindow->hide();
}

void ApplicationController::refreshWindows() {
    const QList<RawWindow> raws = m_windowSource->listWindows();
    const QVector<AppGroup> groups = buildGroups(
        raws,
        m_config.pinned,
        m_config.excluded,
        m_config.mruEnabled ? m_config.mruTimes : QHash<QString, qint64>{},
        m_config.mruEnabled ? m_windowMru : QHash<qint64, qint64>{});
    Filter filter;
    filter.kind = FilterKind::All;
    m_state = AppState::create(groups, filter);

    QSet<qint64> ids;
    for (const AppGroup &g : m_state.groups) {
        for (const WindowItem &w : g.windows) {
            ids.insert(w.windowId);
        }
    }
    for (auto it = m_icons.begin(); it != m_icons.end();) {
        if (!ids.contains(it.key())) {
            it = m_icons.erase(it);
        } else {
            ++it;
        }
    }
    for (auto it = m_thumbs.begin(); it != m_thumbs.end();) {
        if (!ids.contains(it.key())) {
            it = m_thumbs.erase(it);
        } else {
            ++it;
        }
    }
    for (auto it = m_windowMru.begin(); it != m_windowMru.end();) {
        if (!ids.contains(it.key())) {
            it = m_windowMru.erase(it);
        } else {
            ++it;
        }
    }

    m_pendingIcons.clear();
    m_pendingThumbs.clear();
    for (qint64 id : ids) {
        if (!m_icons.contains(id)) {
            m_pendingIcons.append(id);
        }
        if (m_config.thumbnail && !m_thumbs.contains(id)) {
            m_pendingThumbs.append(id);
        }
    }
    m_texturesLoading = !m_pendingIcons.isEmpty() || !m_pendingThumbs.isEmpty();
}

void ApplicationController::recordActivation(qint64 windowId) {
    if (!m_config.mruEnabled) {
        return;
    }
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    m_windowMru.insert(windowId, now);
    for (const AppGroup &g : m_state.groups) {
        for (const WindowItem &w : g.windows) {
            if (w.windowId == windowId) {
                const QString fname = g.exePath.section('/', -1).section('\\', -1).toLower();
                m_config.mruTimes.insert(fname, now);
                m_config.save();
                return;
            }
        }
    }
}

QPixmap ApplicationController::toPixmap(const ImageRgba &image) const {
    if (image.width <= 0 || image.height <= 0 || image.pixels.isEmpty()) {
        return {};
    }
    QImage img(
        reinterpret_cast<const uchar *>(image.pixels.constData()),
        image.width,
        image.height,
        QImage::Format_RGBA8888);
    return QPixmap::fromImage(img.copy());
}

void ApplicationController::loadTexturesBatch() {
    if (!m_texturesLoading) {
        return;
    }
    AppLog::info(QStringLiteral("loadTexturesBatch: icons=%1 thumbs=%2")
                     .arg(m_pendingIcons.size())
                     .arg(m_pendingThumbs.size()));
    constexpr int batch = 4;
    for (int i = 0; i < batch && !m_pendingIcons.isEmpty(); ++i) {
        const qint64 id = m_pendingIcons.takeFirst();
        const ImageRgba ic = m_iconCapture->windowIcon(id);
        if (!ic.pixels.isEmpty()) {
            m_icons.insert(id, toPixmap(ic));
        }
    }
    if (m_config.thumbnail) {
        for (int i = 0; i < batch && !m_pendingThumbs.isEmpty(); ++i) {
            const qint64 id = m_pendingThumbs.takeFirst();
            const ImageRgba tb = m_thumbnailCapture->capture(id);
            if (!tb.pixels.isEmpty()) {
                m_thumbs.insert(id, toPixmap(tb));
            }
        }
    }
    m_texturesLoading = !m_pendingIcons.isEmpty() || !m_pendingThumbs.isEmpty();
    if (m_texturesLoading) {
        m_mainWindow->updateTextures(m_icons, m_thumbs);
        QTimer::singleShot(0, this, &ApplicationController::loadTexturesBatch);
    } else {
        m_mainWindow->updateTextures(m_icons, m_thumbs);
    }
}

void ApplicationController::onPanelAction() {
    const auto action = m_mainWindow->lastPanelAction();
    switch (action.type) {
    case MainWindow::PanelActionType::Activate:
        recordActivation(action.windowId);
        m_windowSource->activate(action.windowId);
        hideAll();
        break;
    case MainWindow::PanelActionType::CloseWindow:
        m_windowSource->closeWindow(action.windowId);
        m_state.removeWindow(action.windowId);
        m_icons.remove(action.windowId);
        m_thumbs.remove(action.windowId);
        m_windowMru.remove(action.windowId);
        m_mainWindow->showPanel(m_state, m_icons, m_thumbs, m_config.thumbnail);
        break;
    case MainWindow::PanelActionType::CloseGroup:
        for (const AppGroup &g : m_state.groups) {
            if (g.exePath == action.exePath) {
                for (const WindowItem &w : g.windows) {
                    m_windowSource->closeWindow(w.windowId);
                    m_icons.remove(w.windowId);
                    m_thumbs.remove(w.windowId);
                    m_windowMru.remove(w.windowId);
                }
                break;
            }
        }
        m_state.removeGroup(action.exePath);
        m_mainWindow->showPanel(m_state, m_icons, m_thumbs, m_config.thumbnail);
        break;
    case MainWindow::PanelActionType::TogglePin: {
        const QString fname = action.exePath.section('/', -1).section('\\', -1).toLower();
        int idx = m_config.pinned.indexOf(fname);
        if (idx >= 0) {
            m_config.pinned.removeAt(idx);
        } else {
            m_config.pinned.append(fname);
        }
        m_config.save();
        m_state.setPinned(m_config.pinned);
        m_mainWindow->showPanel(m_state, m_icons, m_thumbs, m_config.thumbnail);
        break;
    }
    case MainWindow::PanelActionType::Dismiss:
        hideAll();
        break;
    case MainWindow::PanelActionType::None:
        break;
    }
}

void ApplicationController::onSettingsSaved(const Config &cfg) {
    const QString oldHotkey = m_config.hotkey;
    m_config = cfg;
    m_config.save();

    if (m_config.hotkey != oldHotkey) {
        m_hotkey->unregisterHotkey();
        QString err;
        if (!m_hotkey->registerHotkey(m_config.hotkey, &err)) {
            const auto kind = m_hotkey->lastError();
            QString rollbackErr;
            m_hotkey->registerHotkey(oldHotkey, &rollbackErr);
            m_config.hotkey = oldHotkey;
            m_config.save();
            const QString reason = kind == HotkeyManager::HotkeyError::PlatformUnsupported
                ? m_i18n.hotkeyPlatformUnsupported()
                : m_i18n.hotkeyFailedMessage(cfg.hotkey, err);
            showAppMessage(
                m_mainWindow,
                m_i18n.hotkeyFailedTitle(),
                m_i18n.hotkeyRolledBack(oldHotkey, reason),
                AppMessageIcon::Warning);
        }
        m_tray->setToolTip(m_i18n.trayTooltip(m_config.hotkey));
    }
    hideAll();
}
