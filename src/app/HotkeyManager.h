#pragma once

#include <QObject>
#include <QString>

#if defined(Q_OS_WIN)
#include <QAbstractNativeEventFilter>
#endif

class HotkeyManager :
#if defined(Q_OS_WIN)
    public QObject,
    public QAbstractNativeEventFilter
#else
    public QObject
#endif
{
    Q_OBJECT

public:
    explicit HotkeyManager(QObject *parent = nullptr);

    bool registerHotkey(const QString &hotkey, QString *errorMessage = nullptr);
    void unregisterHotkey();

signals:
    void activated();

#if defined(Q_OS_WIN)
    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override;
#endif

private:
    bool parseHotkey(const QString &hotkey, QString *errorMessage);
    void platformRegister(QString *errorMessage);
    void platformUnregister();

    QString m_hotkey;
    bool m_registered = false;

#if defined(Q_OS_WIN)
    int m_atomId = 0;
    quint32 m_modifiers = 0;
    quint32 m_virtualKey = 0;
#elif defined(Q_OS_MACOS)
    void *m_hotkeyRef = nullptr;
#elif defined(Q_OS_LINUX)
    int m_keycode = 0;
    unsigned int m_modMask = 0;
#endif
};
