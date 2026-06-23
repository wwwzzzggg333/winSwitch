#include "app/HotkeyManager.h"

#include <QAbstractNativeEventFilter>
#include <QGuiApplication>
#include <windows.h>

namespace {

quint32 modifierFromToken(const QString &token, bool *ok) {
    *ok = true;
    if (token.compare(QStringLiteral("Ctrl"), Qt::CaseInsensitive) == 0
        || token.compare(QStringLiteral("Control"), Qt::CaseInsensitive) == 0) {
        return MOD_CONTROL;
    }
    if (token.compare(QStringLiteral("Alt"), Qt::CaseInsensitive) == 0) {
        return MOD_ALT;
    }
    if (token.compare(QStringLiteral("Shift"), Qt::CaseInsensitive) == 0) {
        return MOD_SHIFT;
    }
    if (token.compare(QStringLiteral("Win"), Qt::CaseInsensitive) == 0
        || token.compare(QStringLiteral("Super"), Qt::CaseInsensitive) == 0) {
        return MOD_WIN;
    }
    *ok = false;
    return 0;
}

quint32 virtualKeyFromToken(const QString &token, bool *ok) {
    *ok = true;
    if (token.length() == 1) {
        const QChar ch = token.at(0).toUpper();
        if (ch >= QChar('A') && ch <= QChar('Z')) {
            return static_cast<quint32>(ch.unicode());
        }
        switch (ch.unicode()) {
        case '`':
            return VK_OEM_3;
        case '-':
            return VK_OEM_MINUS;
        case '=':
            return VK_OEM_PLUS;
        case ',':
            return VK_OEM_COMMA;
        case '.':
            return VK_OEM_PERIOD;
        default:
            break;
        }
    }
    if (token.compare(QStringLiteral("Space"), Qt::CaseInsensitive) == 0) {
        return VK_SPACE;
    }
    if (token.compare(QStringLiteral("Tab"), Qt::CaseInsensitive) == 0) {
        return VK_TAB;
    }
    if (token.startsWith(QStringLiteral("F"), Qt::CaseInsensitive)) {
        bool okNum = false;
        const int num = token.mid(1).toInt(&okNum);
        if (okNum && num >= 1 && num <= 24) {
            return static_cast<quint32>(VK_F1 + num - 1);
        }
    }
    *ok = false;
    return 0;
}

class HotkeyNativeFilter final : public QAbstractNativeEventFilter {
public:
    explicit HotkeyNativeFilter(HotkeyManager *owner, int atomId) : m_owner(owner), m_atomId(atomId) {}

    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override {
        Q_UNUSED(result)
        if (eventType != "windows_generic_MSG" && eventType != "windows_dispatcher_MSG") {
            return false;
        }
        const MSG *msg = static_cast<MSG *>(message);
        if (msg->message == WM_HOTKEY && msg->wParam == static_cast<WPARAM>(m_atomId)) {
            emit m_owner->activated();
            return true;
        }
        return false;
    }

private:
    HotkeyManager *m_owner = nullptr;
    int m_atomId = 0;
};

} // namespace

bool HotkeyManager::parseHotkey(const QString &hotkey, QString *errorMessage) {
    const QStringList parts = hotkey.split('+', Qt::SkipEmptyParts);
    if (parts.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Empty hotkey");
        }
        return false;
    }
    quint32 mods = 0;
    QString keyToken = parts.last().trimmed();
    for (int i = 0; i < parts.size() - 1; ++i) {
        bool ok = false;
        const quint32 mod = modifierFromToken(parts.at(i).trimmed(), &ok);
        if (!ok) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("Unknown modifier: %1").arg(parts.at(i));
            }
            return false;
        }
        mods |= mod;
    }
    bool ok = false;
    const quint32 vk = virtualKeyFromToken(keyToken, &ok);
    if (!ok) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Unknown key: %1").arg(keyToken);
        }
        return false;
    }
    m_modifiers = mods;
    m_virtualKey = vk;
    return true;
}

void HotkeyManager::platformRegister(QString *errorMessage) {
    m_atomId = qHash(m_hotkey) & 0xBFFF;
    if (!RegisterHotKey(nullptr, m_atomId, m_modifiers, m_virtualKey)) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("RegisterHotKey failed (%1)").arg(GetLastError());
        }
        m_registered = false;
        return;
    }
    m_nativeFilter = new HotkeyNativeFilter(this, m_atomId);
    qApp->installNativeEventFilter(m_nativeFilter);
    m_registered = true;
}

void HotkeyManager::platformUnregister() {
    if (m_atomId != 0) {
        UnregisterHotKey(nullptr, m_atomId);
        if (m_nativeFilter) {
            qApp->removeNativeEventFilter(m_nativeFilter);
            delete m_nativeFilter;
            m_nativeFilter = nullptr;
        }
        m_atomId = 0;
    }
}
