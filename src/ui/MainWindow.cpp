#include "ui/MainWindow.h"
#include "ui/SettingsDialog.h"
#include "ui/SwitcherPanel.h"

#include <QApplication>
#include <QCloseEvent>
#include <QColor>
#include <QDateTime>
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

    qApp->installEventFilter(this);
}

bool MainWindow::isPanelVisible() const {
    return isVisible() && centralWidget() == m_panel;
}

void MainWindow::scheduleFocusLostCheck() {
    QTimer::singleShot(0, this, [this]() {
        if (!isPanelVisible()) {
            return;
        }
        if (QDateTime::currentMSecsSinceEpoch() < m_suppressFocusLossUntil) {
            return;
        }
        QWidget *active = QApplication::activeWindow();
        if (active == this || (active && isAncestorOf(active))) {
            return;
        }
        QWidget *focus = QApplication::focusWidget();
        if (focus && (focus == this || isAncestorOf(focus))) {
            return;
        }
        emit focusLost();
    });
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
    m_suppressFocusLossUntil = QDateTime::currentMSecsSinceEpoch() + 400;
    setMaximumSize(panelSize());
    resize(panelSize());
    centerOnScreen();
    setCentralWidget(m_panel);
    m_panel->setData(state, icons, thumbs, showThumbnails);
    show();
    raise();
    activateWindow();
    QTimer::singleShot(0, m_panel, [panel = m_panel]() { panel->focusSearch(); });
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
    if (event->type() == QEvent::ActivationChange && isPanelVisible() && !isActiveWindow()
        && QDateTime::currentMSecsSinceEpoch() >= m_suppressFocusLossUntil) {
        scheduleFocusLostCheck();
    }
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
    if (!isPanelVisible() || QDateTime::currentMSecsSinceEpoch() < m_suppressFocusLossUntil) {
        return;
    }
    if (event->reason() == Qt::ActiveWindowFocusReason) {
        return;
    }
    scheduleFocusLostCheck();
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
    if (!isPanelVisible() || QDateTime::currentMSecsSinceEpoch() < m_suppressFocusLossUntil) {
        return QMainWindow::eventFilter(obj, event);
    }
    if (event->type() == QEvent::MouseButtonPress) {
            auto *mouseEvent = static_cast<QMouseEvent *>(event);
            const QPoint global = mouseEvent->globalPosition().toPoint();
            if (!frameGeometry().contains(global)) {
                QWidget *target = QApplication::widgetAt(global);
                if (!target || (!isAncestorOf(target) && target != this)) {
                    scheduleFocusLostCheck();
                }
            }
        } else if (event->type() == QEvent::WindowDeactivate && obj == this) {
            scheduleFocusLostCheck();
        }
    return QMainWindow::eventFilter(obj, event);
}
