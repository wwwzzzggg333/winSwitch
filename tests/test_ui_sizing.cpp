#include "ui/UiSizing.h"
#include <QTest>

class UiSizingTest : public QObject {
    Q_OBJECT
private slots:
    void keepsConfiguredDefaultOn1280x720() {
        QCOMPARE(calculatePanelSize({1280, 720}, {960, 600}), QSize(960, 600));
    }
    void clampsHeightOnShortScreen() {
        QCOMPARE(calculatePanelSize({1280, 654}, {960, 600}), QSize(960, 558));
    }
    void usesResponsiveTargetOnFullHd() {
        QCOMPARE(calculatePanelSize({1920, 1080}, {960, 600}), QSize(1280, 669));
    }
    void honorsLargeConfigWithinSafeArea() {
        QCOMPARE(calculatePanelSize({3840, 2160}, {2000, 1200}), QSize(2000, 1200));
    }
    void survivesTinyArea() {
        QCOMPARE(calculatePanelSize({320, 240}, {960, 600}), QSize(256, 144));
    }
    void sanitizesInvalidInputs() {
        QCOMPARE(calculatePanelSize({}, {-1, -1}), QSize(1, 1));
    }
};

QTEST_APPLESS_MAIN(UiSizingTest)
#include "test_ui_sizing.moc"
