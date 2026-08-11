#pragma once

#include "core/Config.h"
#include "core/I18n.h"
#include "core/WindowModel.h"

#include <QHash>
#include <QMainWindow>
#include <QPixmap>
#include <QElapsedTimer>

class QScreen;
class QStackedWidget;

class SwitcherPanel;
class SettingsDialog;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    enum class PanelActionType {
        None,
        Activate,
        CloseWindow,
        CloseGroup,
        Dismiss
    };

    struct PanelAction {
        PanelActionType type = PanelActionType::None;
        qint64 windowId = 0;
        QString exePath;
    };

    explicit MainWindow(const Config &config, I18n i18n, QWidget *parent = nullptr);

    void showPanel(
        const AppState &state,
        const QHash<qint64, QPixmap> &icons,
        const QHash<qint64, QPixmap> &thumbs,
        bool showThumbnails);
    void showSettings(Config config);
    void updateTextures(const QHash<qint64, QPixmap> &icons, const QHash<qint64, QPixmap> &thumbs);

    PanelAction lastPanelAction() const { return m_lastAction; }

signals:
    void panelActionRequested();
    void settingsSaved(const Config &cfg);
    void focusLost();

protected:
    void changeEvent(QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void centerOnScreen();
    QSize panelSize() const;
    QScreen *targetScreen() const;
    bool isPanelVisible() const;

    Config m_config;
    I18n m_i18n;
    QStackedWidget *m_pages = nullptr;
    SwitcherPanel *m_panel = nullptr;
    SettingsDialog *m_settings = nullptr;
    PanelAction m_lastAction;
    bool m_quitting = false;
    QElapsedTimer m_activationTimer;
};
