#include "core/ConfigPaths.h"

#include <QDir>
#include <QFile>

QString ConfigPaths::writableDataDir(const QString &executableDir, const QString &appConfigDir) {
    const QDir exeDir(executableDir);
    const QString installMarker = QDir::cleanPath(exeDir.filePath(QStringLiteral("../installed.marker")));
    if (QFile::exists(installMarker)) {
        return appConfigDir;
    }

    QFile probe(exeDir.filePath(QStringLiteral(".ms_write_test")));
    if (probe.open(QIODevice::WriteOnly)) {
        probe.remove();
        return executableDir;
    }
    return appConfigDir;
}
