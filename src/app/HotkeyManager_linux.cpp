#include "app/HotkeyManager.h"

#include <X11/Xlib.h>
#include <X11/keysym.h>

bool HotkeyManager::parseHotkey(const QString &hotkey, QString *errorMessage) {
    Q_UNUSED(hotkey)
    if (errorMessage) {
        *errorMessage = QStringLiteral("Global hotkeys on Linux require X11 session.");
    }
    return false;
}

void HotkeyManager::platformRegister(QString *errorMessage) {
    Q_UNUSED(errorMessage)
    m_registered = false;
}

void HotkeyManager::platformUnregister() {}
