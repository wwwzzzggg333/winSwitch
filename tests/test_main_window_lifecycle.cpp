#include "ui/MainWindow.h"
#include "ui/SettingsDialog.h"
#include "ui/SwitcherPanel.h"

#include <QCoreApplication>
#include <QEvent>
#include <QPointer>
#include <QTest>

class MainWindowLifecycleTest : public QObject {
    Q_OBJECT

private slots:
    void switchingPagesKeepsBothPagesAlive() {
        const Config config;
        MainWindow window(config, I18n::fromConfig(config));
        QPointer<SwitcherPanel> panel = window.findChild<SwitcherPanel *>();
        QPointer<SettingsDialog> settings = window.findChild<SettingsDialog *>();

        QVERIFY(panel);
        QVERIFY(settings);

        window.showSettings(config);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY2(panel, "The switcher panel must not be deleted when settings are shown");

        window.showPanel(AppState{}, {}, {}, false);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY2(settings, "The settings page must not be deleted when the panel is shown");
    }
};

QTEST_MAIN(MainWindowLifecycleTest)
#include "test_main_window_lifecycle.moc"
