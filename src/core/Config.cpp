#include "core/Config.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QStandardPaths>
#include <limits>

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

void trimMruTimes(QHash<QString, qint64> *mruTimes) {
    constexpr int kMaxMruEntries = 50;
    while (mruTimes->size() > kMaxMruEntries) {
        QString oldestKey;
        qint64 oldestTime = std::numeric_limits<qint64>::max();
        for (auto it = mruTimes->cbegin(); it != mruTimes->cend(); ++it) {
            if (it.value() < oldestTime) {
                oldestTime = it.value();
                oldestKey = it.key();
            }
        }
        if (oldestKey.isEmpty()) {
            break;
        }
        mruTimes->remove(oldestKey);
    }
}

QJsonObject toJson(const Config &cfg) {
    QJsonObject obj;
    obj.insert(QStringLiteral("hotkey"), cfg.hotkey);
    obj.insert(QStringLiteral("thumbnail"), cfg.thumbnail);
    obj.insert(QStringLiteral("panel_width"), cfg.panelWidth);
    obj.insert(QStringLiteral("panel_height"), cfg.panelHeight);
    obj.insert(QStringLiteral("language"), cfg.language);
    obj.insert(QStringLiteral("start_at_login"), cfg.startAtLogin);
    obj.insert(QStringLiteral("mru_enabled"), cfg.mruEnabled);

    QJsonArray excluded;
    for (const QString &e : cfg.excluded) {
        excluded.append(e);
    }
    obj.insert(QStringLiteral("excluded"), excluded);

    QJsonObject mru;
    for (auto it = cfg.mruTimes.cbegin(); it != cfg.mruTimes.cend(); ++it) {
        mru.insert(it.key(), static_cast<double>(it.value()));
    }
    obj.insert(QStringLiteral("mru"), mru);
    return obj;
}

Config fromJson(const QJsonObject &obj) {
    Config cfg;
    cfg.hotkey = obj.value(QStringLiteral("hotkey")).toString(cfg.hotkey);
    cfg.thumbnail = obj.value(QStringLiteral("thumbnail")).toBool(cfg.thumbnail);
    cfg.panelWidth = obj.value(QStringLiteral("panel_width")).toDouble(cfg.panelWidth);
    cfg.panelHeight = obj.value(QStringLiteral("panel_height")).toDouble(cfg.panelHeight);
    cfg.language = obj.value(QStringLiteral("language")).toString(cfg.language);
    cfg.startAtLogin = obj.value(QStringLiteral("start_at_login")).toBool(cfg.startAtLogin);
    cfg.mruEnabled = obj.value(QStringLiteral("mru_enabled")).toBool(cfg.mruEnabled);

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

    cfg.mruTimes.clear();
    const QJsonObject mruObj = obj.value(QStringLiteral("mru")).toObject();
    for (auto it = mruObj.begin(); it != mruObj.end(); ++it) {
        cfg.mruTimes.insert(it.key().toLower(), static_cast<qint64>(it.value().toDouble()));
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
    Config trimmed = *this;
    trimMruTimes(&trimmed.mruTimes);
    QDir().mkpath(dataDir());
    QFile file(configPath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    file.write(QJsonDocument(toJson(trimmed)).toJson(QJsonDocument::Indented));
    return true;
}

bool Config::exportTo(const QString &path, QString *error) const {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (error) {
            *error = file.errorString();
        }
        return false;
    }
    file.write(QJsonDocument(toJson(*this)).toJson(QJsonDocument::Indented));
    return true;
}

bool Config::importFrom(const QString &path, Config *out, QString *error) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error) {
            *error = file.errorString();
        }
        return false;
    }
    QJsonParseError parseError{};
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        if (error) {
            *error = parseError.errorString();
        }
        return false;
    }
    if (!doc.isObject()) {
        if (error) {
            *error = QStringLiteral("Root is not a JSON object");
        }
        return false;
    }
    Config cfg = fromJson(doc.object());
    if (cfg.hotkey.trimmed().isEmpty()) {
        if (error) {
            *error = QStringLiteral("Field 'hotkey' is empty");
        }
        return false;
    }
    *out = cfg;
    return true;
}

bool Config::operator==(const Config &other) const {
    return hotkey == other.hotkey
        && thumbnail == other.thumbnail
        && panelWidth == other.panelWidth
        && panelHeight == other.panelHeight
        && language == other.language
        && excluded == other.excluded
        && startAtLogin == other.startAtLogin
        && mruEnabled == other.mruEnabled
        && mruTimes == other.mruTimes;
}
