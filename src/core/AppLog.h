#pragma once

#include <QString>

namespace AppLog {

void init();
void info(const QString &message);
void warn(const QString &message);

} // namespace AppLog
