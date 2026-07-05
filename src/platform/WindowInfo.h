#pragma once

#include <QString>

struct RawWindow {
    qint64 windowId = 0;
    QString title;
    QString exePath;
    QString appName;
    QString folderPath;

    bool isListable() const {
        if (!appName.compare(QStringLiteral("explorer"), Qt::CaseInsensitive)
            || !appName.compare(QStringLiteral("Finder"), Qt::CaseInsensitive)) {
            return !title.trimmed().isEmpty();
        }
        return true;
    }
};

struct ImageRgba {
    int width = 0;
    int height = 0;
    QByteArray pixels;
};
