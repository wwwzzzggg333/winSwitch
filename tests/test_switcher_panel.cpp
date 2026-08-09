#include "ui/SwitcherPanel.h"
#include "ui/WindowCard.h"

#include <QLabel>
#include <QLineEdit>
#include <QPointer>
#include <QTest>

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
};

QTEST_MAIN(SwitcherPanelTest)
#include "test_switcher_panel.moc"
