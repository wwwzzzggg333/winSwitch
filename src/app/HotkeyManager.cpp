#include "app/HotkeyManager.h"

HotkeyManager::HotkeyManager(QObject *parent) : QObject(parent) {}

bool HotkeyManager::registerHotkey(const QString &hotkey, QString *errorMessage) {
    unregisterHotkey();
    m_lastError = HotkeyError::None;
    if (!parseHotkey(hotkey, errorMessage)) {
        if (m_lastError == HotkeyError::None) {
            m_lastError = HotkeyError::ParseFailed;
        }
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
