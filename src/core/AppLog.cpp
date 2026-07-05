#include "core/AppLog.h"
#include "core/Config.h"

#include <QDateTime>
#include <QFile>
#include <QMutex>
#include <QMutexLocker>
#include <QtGlobal>

namespace {

QMutex g_logMutex;
QtMessageHandler g_previousHandler = nullptr;

void writeRaw(const QString &line) {
    QMutexLocker lock(&g_logMutex);
    QFile file(Config::logPath());
    if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        file.write(line.toUtf8());
        file.write("\n");
    }
}

void qtMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &message) {
    Q_UNUSED(context)
    const char *level = "INFO";
    switch (type) {
    case QtDebugMsg:
        level = "DEBUG";
        break;
    case QtInfoMsg:
        level = "INFO";
        break;
    case QtWarningMsg:
        level = "WARN";
        break;
    case QtCriticalMsg:
        level = "CRIT";
        break;
    case QtFatalMsg:
        level = "FATAL";
        break;
    }
    writeRaw(QStringLiteral("[%1] %2: %3")
                 .arg(QDateTime::currentDateTime().toString(Qt::ISODateWithMs), level, message));
    if (g_previousHandler) {
        g_previousHandler(type, context, message);
    }
}

} // namespace

void AppLog::init() {
    g_previousHandler = qInstallMessageHandler(qtMessageHandler);
    info(QStringLiteral("logger initialized, path=%1").arg(Config::logPath()));
}

void AppLog::info(const QString &message) {
    qInfo().noquote() << message;
}

void AppLog::warn(const QString &message) {
    qWarning().noquote() << message;
}
