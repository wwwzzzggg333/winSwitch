#pragma once

#include <QHash>
#include <QString>
#include <QStringList>

struct Config {
    QString hotkey = QStringLiteral("Alt+`");
    bool thumbnail = true;
    double panelWidth = 960.0;
    double panelHeight = 600.0;
    QString language = QStringLiteral("auto");
    QStringList excluded = {QStringLiteral("TextInputHost.exe")};
    bool mruEnabled = true;
    QHash<QString, qint64> mruTimes;

    static Config load();
    bool save() const;

    bool exportTo(const QString &path, QString *error = nullptr) const;
    static bool importFrom(const QString &path, Config *out, QString *error = nullptr);

    static QString dataDir();
    static QString configPath();
    static QString logPath();
    static bool welcomeShown();
    static void markWelcomeShown();

    bool operator==(const Config &other) const;
};
