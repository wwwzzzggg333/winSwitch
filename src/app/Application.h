#pragma once

#include "app/HotkeyManager.h"
#include "core/Config.h"
#include "core/I18n.h"
#include "core/WindowModel.h"
#include "platform/IWindowSource.h"
#include "ui/MainWindow.h"

#include <QHash>
#include <QPixmap>
#include <QSystemTrayIcon>
#include <memory>

class ApplicationController : public QObject {
    Q_OBJECT

public:
    explicit ApplicationController(QObject *parent = nullptr);
    ~ApplicationController() override;

    bool initialize();

private slots:
    void onHotkeyActivated();
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);
    void onOpenSettings();
    void onQuit();
    void onPanelAction();
    void onSettingsSaved(const Config &cfg);

private:
    enum class View { Hidden, Panel, Settings };

    void showPanel();
    void showSettings();
    void hideAll();
    void refreshWindows();
    void loadTexturesBatch();
    void recordActivation(qint64 windowId);
    QPixmap toPixmap(const ImageRgba &image) const;
    void showHotkeyFailure(const QString &hotkey, const QString &err, HotkeyManager::HotkeyError kind);

    Config m_config;
    I18n m_i18n;
    std::unique_ptr<IWindowSource> m_windowSource;
    std::unique_ptr<IIconCapture> m_iconCapture;
    std::unique_ptr<IThumbnailCapture> m_thumbnailCapture;
    AppState m_state;
    View m_view = View::Hidden;

    MainWindow *m_mainWindow = nullptr;
    QSystemTrayIcon *m_tray = nullptr;
    HotkeyManager *m_hotkey = nullptr;

    QHash<qint64, QPixmap> m_icons;
    QHash<qint64, QPixmap> m_thumbs;
    QHash<qint64, qint64> m_windowMru;
    QVector<qint64> m_pendingIcons;
    QVector<qint64> m_pendingThumbs;
    bool m_texturesLoading = false;
};
