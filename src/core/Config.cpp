#include "core/Config.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

namespace {

QString writableDataDir() {
    const QString exeDir = QCoreApplication::applicationDirPath();
    const QString probe = exeDir + QStringLiteral("/.ms_write_test");
    QFile probeFile(probe);
    if (probeFile.open(QIODevice::WriteOnly)) {
        probeFile.remove();
        return exeDir;
    }
    return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
}

QJsonObject toJson(const Config &cfg) {
    QJsonObject obj;
    obj.insert(QStringLiteral("hotkey"), cfg.hotkey);
    obj.insert(QStringLiteral("thumbnail"), cfg.thumbnail);
    obj.insert(QStringLiteral("panel_width"), cfg.panelWidth);
    obj.insert(QStringLiteral("panel_height"), cfg.panelHeight);
    obj.insert(QStringLiteral("language"), cfg.language);

    QJsonArray pinned;
    for (const QString &p : cfg.pinned) {
        pinned.append(p);
    }
    obj.insert(QStringLiteral("pinned"), pinned);

    QJsonArray excluded;
    for (const QString &e : cfg.excluded) {
        excluded.append(e);
    }
    obj.insert(QStringLiteral("excluded"), excluded);
    return obj;
}

Config fromJson(const QJsonObject &obj) {
    Config cfg;
    cfg.hotkey = obj.value(QStringLiteral("hotkey")).toString(cfg.hotkey);
    cfg.thumbnail = obj.value(QStringLiteral("thumbnail")).toBool(cfg.thumbnail);
    cfg.panelWidth = obj.value(QStringLiteral("panel_width")).toDouble(cfg.panelWidth);
    cfg.panelHeight = obj.value(QStringLiteral("panel_height")).toDouble(cfg.panelHeight);
    cfg.language = obj.value(QStringLiteral("language")).toString(cfg.language);

    cfg.pinned.clear();
    for (const QJsonValue &v : obj.value(QStringLiteral("pinned")).toArray()) {
        const QString s = v.toString().trimmed();
        if (!s.isEmpty()) {
            cfg.pinned.append(s);
        }
    }

    cfg.excluded.clear();
    const QJsonArray excludedArr = obj.value(QStringLiteral("excluded")).toArray();
    if (excludedArr.isEmpty() && !obj.contains(QStringLiteral("excluded"))) {
        cfg.excluded = Config{}.excluded;
    } else {
        for (const QJsonValue &v : excludedArr) {
            const QString s = v.toString().trimmed();
            if (!s.isEmpty()) {
                cfg.excluded.append(s);
            }
        }
    }
    return cfg;
}

} // namespace

QString Config::dataDir() {
    const QString dir = writableDataDir();
    QDir().mkpath(dir);
    return dir;
}

QString Config::configPath() {
    return dataDir() + QStringLiteral("/config.json");
}

QString Config::logPath() {
    return dataDir() + QStringLiteral("/app.log");
}

bool Config::welcomeShown() {
    return QFile::exists(dataDir() + QStringLiteral("/.welcome_shown"));
}

void Config::markWelcomeShown() {
    QFile f(dataDir() + QStringLiteral("/.welcome_shown"));
    f.open(QIODevice::WriteOnly);
}

Config Config::load() {
    QFile file(configPath());
    if (!file.open(QIODevice::ReadOnly)) {
        return Config{};
    }
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) {
        return Config{};
    }
    return fromJson(doc.object());
}

bool Config::save() const {
    QDir().mkpath(dataDir());
    QFile file(configPath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    file.write(QJsonDocument(toJson(*this)).toJson(QJsonDocument::Indented));
    return true;
}

bool Config::operator==(const Config &other) const {
    return hotkey == other.hotkey
        && thumbnail == other.thumbnail
        && panelWidth == other.panelWidth
        && panelHeight == other.panelHeight
        && language == other.language
        && pinned == other.pinned
        && excluded == other.excluded;
}
