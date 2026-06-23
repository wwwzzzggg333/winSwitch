#pragma once

#include <QString>
#include <QStringList>

struct Config {
    QString hotkey = QStringLiteral("Alt+`");
    bool thumbnail = true;
    double panelWidth = 960.0;
    double panelHeight = 600.0;
    QString language = QStringLiteral("auto");
    QStringList pinned;
    QStringList excluded = {QStringLiteral("TextInputHost.exe")};

    static Config load();
    bool save() const;

    static QString dataDir();
    static QString configPath();
    static QString logPath();
    static bool welcomeShown();
    static void markWelcomeShown();

    bool operator==(const Config &other) const;
};
