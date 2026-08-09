#include "ui/SwitcherPanel.h"
#include "ui/WindowCard.h"

#include <QApplication>
#include <QLabel>
#include <QLineEdit>
#include <QPointer>
#include <QScrollArea>
#include <QScrollBar>
#include <QTest>
#include <QToolButton>
#include <QWheelEvent>

namespace {
I18n zhI18n() {
    Config cfg;
    cfg.language = QStringLiteral("zh");
    return I18n::fromConfig(cfg);
}

WindowItem sampleItem() {
    return WindowItem{1, QStringLiteral("Sample window"), {}};
}
} // namespace

class SwitcherPanelTest : public QObject {
    Q_OBJECT

private slots:
    void reportsUnavailablePreviewWhenCaptureWasEnabled() {
        QPixmap icon(64, 64);
        icon.fill(Qt::green);
        WindowCard card(sampleItem(), icon, {}, true, false, zhI18n());
        auto *status = card.findChild<QLabel *>(QStringLiteral("ThumbnailStatus"));
        QVERIFY(status != nullptr);
        QCOMPARE(status->text(), QStringLiteral("窗口预览不可用"));
    }

    void doesNotReportFailureWhenThumbnailsWereDisabled() {
        QPixmap icon(64, 64);
        icon.fill(Qt::green);
        WindowCard card(sampleItem(), icon, {}, false, false, zhI18n());
        QVERIFY(card.findChild<QLabel *>(QStringLiteral("ThumbnailStatus")) == nullptr);
    }

    void usesGenericFallbackWhenNoTextureExists() {
        WindowCard card(sampleItem(), {}, {}, true, false, zhI18n());
        QVERIFY(card.findChild<QLabel *>(QStringLiteral("ThumbnailFallbackGlyph")) != nullptr);
    }

    void ownsThumbnailImageLabelWhenFallbackIsRendered() {
        QPointer<QLabel> image;
        {
            WindowCard card(sampleItem(), {}, {}, true, false, zhI18n());
            image = card.findChild<QLabel *>(QStringLiteral("ThumbnailImage"));
            QVERIFY(image != nullptr);
        }
        QVERIFY(image.isNull());
    }

    void showsEmptyStateWithoutWindows() {
        SwitcherPanel panel(zhI18n());
        panel.setData(AppState::create({}, Filter{}), {}, {}, true);
        auto *title = panel.findChild<QLabel *>(QStringLiteral("EmptyStateTitle"));
        QVERIFY(title != nullptr);
        QCOMPARE(title->text(), QStringLiteral("当前没有可切换窗口"));
    }

    void showsNoMatchStateForSearch() {
        RawWindow raw{7, QStringLiteral("Terminal"), QStringLiteral("C:/terminal.exe"),
                      QStringLiteral("Terminal"), {}};
        AppState state = AppState::create(buildGroups({raw}, {}, {}), Filter{});
        SwitcherPanel panel(zhI18n());
        panel.setData(state, {}, {}, true);
        auto *search = panel.findChild<QLineEdit *>(QStringLiteral("SearchEdit"));
        QVERIFY(search != nullptr);
        search->setText(QStringLiteral("not-found"));
        auto *title = panel.findChild<QLabel *>(QStringLiteral("EmptyStateTitle"));
        QVERIFY(title != nullptr);
        QCOMPARE(title->text(), QStringLiteral("没有匹配的窗口"));
    }

    void wheelScrollsLongFilterRowHorizontally() {
        QVector<AppGroup> groups;
        for (int i = 0; i < 12; ++i) {
            AppGroup group;
            group.exePath = QStringLiteral("C:/apps/application-%1.exe").arg(i);
            group.appName = QStringLiteral("application-%1").arg(i);
            group.windows.append(WindowItem{i + 1, QStringLiteral("window"), {}});
            groups.append(group);
        }

        SwitcherPanel panel(zhI18n());
        panel.resize(420, 500);
        panel.setData(AppState::create(groups, Filter{}), {}, {}, false);
        panel.show();
        QTest::qWait(1);

        auto *scroll = panel.findChild<QScrollArea *>(QStringLiteral("FilterScroll"));
        QVERIFY(scroll != nullptr);
        QVERIFY(scroll->horizontalScrollBar()->maximum() > 0);
        const int before = scroll->horizontalScrollBar()->value();
        QWheelEvent wheel(QPointF(20, 20), QPointF(20, 20), {}, QPoint(0, -120),
                          Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
        QApplication::sendEvent(scroll->viewport(), &wheel);
        QVERIFY(scroll->horizontalScrollBar()->value() > before);
    }

    void horizontalWheelUsesNativeFilterScrolling() {
        QVector<AppGroup> groups;
        for (int i = 0; i < 12; ++i) {
            AppGroup group;
            group.exePath = QStringLiteral("C:/apps/application-%1.exe").arg(i);
            group.appName = QStringLiteral("application-%1").arg(i);
            group.windows.append(WindowItem{i + 1, QStringLiteral("window"), {}});
            groups.append(group);
        }

        SwitcherPanel panel(zhI18n());
        panel.resize(420, 500);
        panel.setData(AppState::create(groups, Filter{}), {}, {}, false);
        panel.show();
        QTest::qWait(1);

        auto *scroll = panel.findChild<QScrollArea *>(QStringLiteral("FilterScroll"));
        QVERIFY(scroll != nullptr);
        auto *bar = scroll->horizontalScrollBar();
        QVERIFY(bar->maximum() > 0);
        bar->setValue(bar->maximum() / 2);
        const int before = bar->value();
        QWheelEvent wheel(QPointF(20, 20), QPointF(20, 20), {}, QPoint(-120, 0),
                          Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
        QApplication::sendEvent(scroll->viewport(), &wheel);
        QVERIFY(bar->value() != before);
    }

    void groupCloseActionEmitsImmediately() {
        AppGroup group;
        group.exePath = QStringLiteral("C:/apps/application.exe");
        group.appName = QStringLiteral("application");
        group.windows.append(sampleItem());

        SwitcherPanel panel(zhI18n());
        panel.setData(AppState::create({group}, Filter{}), {}, {}, false);
        MainWindow::PanelAction received;
        int actionCount = 0;
        connect(&panel, &SwitcherPanel::actionTriggered, this,
                [&received, &actionCount](const MainWindow::PanelAction &action) {
                    received = action;
                    ++actionCount;
                });

        auto *closeAll = panel.findChild<QToolButton *>(QStringLiteral("GroupCloseAction"));
        QVERIFY(closeAll != nullptr);
        QTest::mouseClick(closeAll, Qt::LeftButton);
        QCOMPARE(actionCount, 1);
        QCOMPARE(received.type, MainWindow::PanelActionType::CloseGroup);
        QCOMPARE(received.exePath, group.exePath);
    }

    void windowCloseActionEmitsImmediately() {
        AppGroup group;
        group.exePath = QStringLiteral("C:/apps/application.exe");
        group.appName = QStringLiteral("application");
        group.windows.append(sampleItem());

        SwitcherPanel panel(zhI18n());
        panel.setData(AppState::create({group}, Filter{}), {}, {}, false);
        MainWindow::PanelAction received;
        int actionCount = 0;
        connect(&panel, &SwitcherPanel::actionTriggered, this,
                [&received, &actionCount](const MainWindow::PanelAction &action) {
                    received = action;
                    ++actionCount;
                });

        auto *closeWindow = panel.findChild<QToolButton *>(QStringLiteral("CardClose"));
        QVERIFY(closeWindow != nullptr);
        QTest::mouseClick(closeWindow, Qt::LeftButton);
        QCOMPARE(actionCount, 1);
        QCOMPARE(received.type, MainWindow::PanelActionType::CloseWindow);
        QCOMPARE(received.windowId, qint64{1});
    }
};

QTEST_MAIN(SwitcherPanelTest)
#include "test_switcher_panel.moc"
