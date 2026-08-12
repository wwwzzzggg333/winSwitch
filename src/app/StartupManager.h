#pragma once

#include <QString>

class StartupManager {
public:
    static bool setEnabled(bool enabled, QString *error = nullptr);
};
