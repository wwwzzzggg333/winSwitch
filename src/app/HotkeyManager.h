#pragma once

#include <QObject>
#include <QString>

class HotkeyManager : public QObject {
    Q_OBJECT

public:
    enum class HotkeyError {
        None,
        ParseFailed,
        RegisterFailed,
        PlatformUnsupported
    };

    explicit HotkeyManager(QObject *parent = nullptr);

    bool registerHotkey(const QString &hotkey, QString *errorMessage = nullptr);
    void unregisterHotkey();
    HotkeyError lastError() const { return m_lastError; }

signals:
    void activated();

private:
    bool parseHotkey(const QString &hotkey, QString *errorMessage);
    void platformRegister(QString *errorMessage);
    void platformUnregister();

    QString m_hotkey;
    bool m_registered = false;
    HotkeyError m_lastError = HotkeyError::None;

#if defined(Q_OS_WIN)
    int m_atomId = 0;
    quint32 m_modifiers = 0;
    quint32 m_virtualKey = 0;
    class QAbstractNativeEventFilter *m_nativeFilter = nullptr;
#elif defined(Q_OS_MACOS)
    void *m_hotkeyRef = nullptr;
#elif defined(Q_OS_LINUX)
    int m_keycode = 0;
    unsigned int m_modMask = 0;
#endif
};
