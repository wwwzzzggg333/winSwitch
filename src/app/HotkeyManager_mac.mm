#include "app/HotkeyManager.h"

#include <Carbon/Carbon.h>

bool HotkeyManager::parseHotkey(const QString &hotkey, QString *errorMessage) {
    Q_UNUSED(hotkey)
    Q_UNUSED(errorMessage)
    return true;
}

void HotkeyManager::platformRegister(QString *errorMessage) {
    if (errorMessage) {
        *errorMessage = QStringLiteral("Global hotkeys are not implemented on macOS yet.");
    }
    m_lastError = HotkeyError::PlatformUnsupported;
    m_registered = false;
}

void HotkeyManager::platformUnregister() {
    m_hotkeyRef = nullptr;
}
