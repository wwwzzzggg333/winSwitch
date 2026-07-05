#include "app/Application.h"
#include "app/SingleInstance.h"
#include "core/Config.h"
#include "core/I18n.h"

#include "ui/AppMessageBox.h"

#include <QApplication>
#include <QFile>
#include <QStyle>

static void loadStyleSheet(QApplication &app) {
    QFile file(QStringLiteral(":/styles/app.qss"));
    if (file.open(QIODevice::ReadOnly)) {
        app.setStyleSheet(QString::fromUtf8(file.readAll()));
    }
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("mySwitcher"));
    QApplication::setOrganizationName(QStringLiteral("mySwitcher"));
    QApplication::setQuitOnLastWindowClosed(false);
    app.setWindowIcon(QApplication::style()->standardIcon(QStyle::SP_ComputerIcon));
    loadStyleSheet(app);

    SingleInstance instance(QStringLiteral("mySwitcher_singleton"));
    if (instance.isAnotherRunning()) {
        const Config cfg = Config::load();
        const I18n i18n = I18n::fromConfig(cfg);
        showAppMessage(nullptr, i18n.appTitle(), i18n.alreadyRunningMessage(), AppMessageIcon::Information);
        return 0;
    }

    ApplicationController controller;
    if (!controller.initialize()) {
        return 1;
    }
    return app.exec();
}
