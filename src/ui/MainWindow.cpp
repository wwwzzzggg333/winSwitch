#include "ui/MainWindow.h"
#include "ui/SettingsDialog.h"
#include "ui/SwitcherPanel.h"

#include "core/AppLog.h"

#include <QApplication>
#include <QCloseEvent>
#include <QColor>
#include <QCursor>
#include <QEvent>
#include <QFocusEvent>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QPalette>
#include <QScreen>
#include <QTimer>

MainWindow::MainWindow(const Config &config, I18n i18n, QWidget *parent)
    : QMainWindow(parent), m_config(config), m_i18n(i18n) {
    setWindowTitle(m_i18n.appTitle());
    setWindowFlags(
        Qt::Window | Qt::WindowStaysOnTopHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint
        | Qt::WindowMinimizeButtonHint);
    setAttribute(Qt::WA_StyledBackground, true);
    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor(QStringLiteral("#1a1b20")));
    pal.setColor(QPalette::Base, QColor(QStringLiteral("#1a1b20")));
    setPalette(pal);
    resize(panelSize());

    m_panel = new SwitcherPanel(m_i18n, this);
    setCentralWidget(m_panel);
    connect(m_panel, &SwitcherPanel::actionTriggered, this, [this](MainWindow::PanelAction action) {
        m_lastAction = action;
        emit panelActionRequested();
    });

    m_settings = new SettingsDialog(m_i18n, this);
    connect(m_settings, &SettingsDialog::saved, this, [this](const Config &cfg) {
        m_config = cfg;
        emit settingsSaved(cfg);
    });

    connect(qApp, &QGuiApplication::applicationStateChanged, this, [this](Qt::ApplicationState state) {
        if (state != Qt::ApplicationInactive || !isPanelVisible()) {
            return;
        }
        constexpr int kActivationGuardMs = 500;
        if (m_activationTimer.isValid() && m_activationTimer.elapsed() < kActivationGuardMs) {
            return;
        }
        QTimer::singleShot(120, this, [this]() {
            if (!isPanelVisible()) {
                return;
            }
            if (qApp->applicationState() == Qt::ApplicationInactive) {
                AppLog::info(QStringLiteral("panel hide: application inactive"));
                emit focusLost();
            }
        });
    });

    qApp->installEventFilter(this);
}

bool MainWindow::isPanelVisible() const {
    return isVisible() && centralWidget() == m_panel;
}

QScreen *MainWindow::targetScreen() const {
    QScreen *screen = QGuiApplication::screenAt(QCursor::pos());
    if (!screen) {
        screen = QApplication::primaryScreen();
    }
    return screen;
}

QSize MainWindow::panelSize() const {
    QScreen *screen = targetScreen();
    const QSize monitor = screen ? screen->availableGeometry().size() : QSize(1920, 1080);
    const int autoW = qBound(840, static_cast<int>(monitor.width() * 0.70), 1280);
    const int autoH = qBound(540, static_cast<int>(monitor.height() * 0.62), 820);
    return QSize(qMax(static_cast<int>(m_config.panelWidth), autoW),
                 qMax(static_cast<int>(m_config.panelHeight), autoH));
}

void MainWindow::centerOnScreen() {
    QScreen *screen = targetScreen();
    if (!screen) {
        return;
    }
    const QRect geo = screen->availableGeometry();
    const QSize size = frameGeometry().size();
    move(geo.x() + (geo.width() - size.width()) / 2, geo.y() + (geo.height() - size.height()) / 2);
}

void MainWindow::showPanel(
    const AppState &state,
    const QHash<qint64, QPixmap> &icons,
    const QHash<qint64, QPixmap> &thumbs,
    bool showThumbnails) {
    setMaximumSize(panelSize());
    resize(panelSize());
    centerOnScreen();
    setCentralWidget(m_panel);
    m_panel->setData(state, icons, thumbs, showThumbnails);
    show();
    raise();
    activateWindow();
    m_activationTimer.start();
    QTimer::singleShot(0, m_panel, [panel = m_panel]() { panel->focusSearch(); });
    AppLog::info(QStringLiteral("panel shown, groups=%1").arg(state.groups.size()));
}

void MainWindow::showSettings(Config config) {
    setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    m_settings->setConfig(config);
    resize(520, 560);
    centerOnScreen();
    setCentralWidget(m_settings);
    show();
    raise();
    activateWindow();
}

void MainWindow::updateTextures(const QHash<qint64, QPixmap> &icons, const QHash<qint64, QPixmap> &thumbs) {
    m_panel->updateTextures(icons, thumbs);
}

void MainWindow::changeEvent(QEvent *event) {
    QMainWindow::changeEvent(event);
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (!m_quitting) {
        event->ignore();
        hide();
    } else {
        QMainWindow::closeEvent(event);
    }
}

void MainWindow::focusOutEvent(QFocusEvent *event) {
    QMainWindow::focusOutEvent(event);
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
    if (isPanelVisible() && event->type() == QEvent::MouseButtonPress) {
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        const QPoint global = mouseEvent->globalPosition().toPoint();
        if (!frameGeometry().contains(global)) {
            QWidget *target = QApplication::widgetAt(global);
            if (!target || (!isAncestorOf(target) && target != this)) {
                AppLog::info(QStringLiteral("panel hide: click outside"));
                emit focusLost();
            }
        }
    }
    return QMainWindow::eventFilter(obj, event);
}
