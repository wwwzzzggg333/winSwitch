#include "app/HotkeyManager.h"

bool HotkeyManager::registerHotkey(const QString &hotkey, QString *errorMessage) {
    unregisterHotkey();
    if (!parseHotkey(hotkey, errorMessage)) {
        return false;
    }
    m_hotkey = hotkey;
    platformRegister(errorMessage);
    return m_registered;
}

void HotkeyManager::unregisterHotkey() {
    if (m_registered) {
        platformUnregister();
    }
    m_registered = false;
}
