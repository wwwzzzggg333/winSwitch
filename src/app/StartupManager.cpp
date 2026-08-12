#include "app/StartupManager.h"

#include <QCoreApplication>
#include <QDir>
#include <QSettings>

bool StartupManager::setEnabled(bool enabled, QString *error) {
#if defined(Q_OS_WIN)
    QSettings startup(
        QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run"),
        QSettings::NativeFormat);
    const QString valueName = QStringLiteral("winSwitch");
    if (enabled) {
        const QString executable = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
        startup.setValue(valueName, QStringLiteral("\"%1\"").arg(executable));
    } else {
        startup.remove(valueName);
    }
    startup.sync();
    if (startup.status() == QSettings::NoError) {
        return true;
    }
    if (error) {
        *error = QStringLiteral("Failed to update the current-user startup registry entry");
    }
    return false;
#else
    Q_UNUSED(enabled)
    if (error) {
        *error = QStringLiteral("Start at login is only supported on Windows");
    }
    return false;
#endif
}
