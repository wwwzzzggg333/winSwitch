#include "ui/HotkeyEdit.h"

#include <QFocusEvent>
#include <QKeyEvent>

HotkeyEdit::HotkeyEdit(I18n i18n, QWidget *parent) : QLineEdit(parent), m_i18n(i18n) {
    setReadOnly(true);
}

QString HotkeyEdit::hotkey() const {
    return text();
}

void HotkeyEdit::setHotkey(const QString &hotkey) {
    setText(hotkey);
}

void HotkeyEdit::focusInEvent(QFocusEvent *event) {
    m_listening = true;
    setPlaceholderText(m_i18n.hotkeyListening());
    QLineEdit::focusInEvent(event);
}

void HotkeyEdit::focusOutEvent(QFocusEvent *event) {
    m_listening = false;
    setPlaceholderText({});
    QLineEdit::focusOutEvent(event);
}

QString HotkeyEdit::formatKey(QKeyEvent *event) const {
    if (event->key() == Qt::Key_Escape) {
        return {};
    }
    QStringList parts;
    const Qt::KeyboardModifiers mods = event->modifiers()
        & (Qt::ControlModifier | Qt::AltModifier | Qt::ShiftModifier | Qt::MetaModifier);
    if (mods & Qt::ControlModifier) {
        parts.append(QStringLiteral("Ctrl"));
    }
    if (mods & Qt::AltModifier) {
        parts.append(QStringLiteral("Alt"));
    }
    if (mods & Qt::ShiftModifier) {
        parts.append(QStringLiteral("Shift"));
    }
    if (mods & Qt::MetaModifier) {
        parts.append(QStringLiteral("Win"));
    }

    QString keyToken;
    const int key = event->key();
    if (key >= Qt::Key_A && key <= Qt::Key_Z) {
        keyToken = QChar(key);
    } else if (key >= Qt::Key_0 && key <= Qt::Key_9) {
        keyToken = QChar(key);
    } else if (key == Qt::Key_Space) {
        keyToken = QStringLiteral("Space");
    } else if (key == Qt::Key_Tab) {
        keyToken = QStringLiteral("Tab");
    } else if (key == Qt::Key_QuoteLeft) {
        keyToken = QStringLiteral("`");
    } else if (key == Qt::Key_Minus) {
        keyToken = QStringLiteral("-");
    } else if (key == Qt::Key_Equal) {
        keyToken = QStringLiteral("=");
    } else if (key == Qt::Key_Comma) {
        keyToken = QStringLiteral(",");
    } else if (key == Qt::Key_Period) {
        keyToken = QStringLiteral(".");
    } else if (key == Qt::Key_Semicolon) {
        keyToken = QStringLiteral(";");
    } else if (key == Qt::Key_Apostrophe) {
        keyToken = QStringLiteral("'");
    } else if (key == Qt::Key_BracketLeft) {
        keyToken = QStringLiteral("[");
    } else if (key == Qt::Key_BracketRight) {
        keyToken = QStringLiteral("]");
    } else if (key == Qt::Key_Slash) {
        keyToken = QStringLiteral("/");
    } else if (key == Qt::Key_Backslash) {
        keyToken = QStringLiteral("\\");
    } else if (key >= Qt::Key_F1 && key <= Qt::Key_F24) {
        keyToken = QStringLiteral("F%1").arg(key - Qt::Key_F1 + 1);
    } else if (key == Qt::Key_Up) {
        keyToken = QStringLiteral("Up");
    } else if (key == Qt::Key_Down) {
        keyToken = QStringLiteral("Down");
    } else if (key == Qt::Key_Left) {
        keyToken = QStringLiteral("Left");
    } else if (key == Qt::Key_Right) {
        keyToken = QStringLiteral("Right");
    } else if (key == Qt::Key_Home) {
        keyToken = QStringLiteral("Home");
    } else if (key == Qt::Key_End) {
        keyToken = QStringLiteral("End");
    } else if (key == Qt::Key_PageUp) {
        keyToken = QStringLiteral("PgUp");
    } else if (key == Qt::Key_PageDown) {
        keyToken = QStringLiteral("PgDn");
    } else {
        return {};
    }

    const bool allowBareKey = key >= Qt::Key_F1 && key <= Qt::Key_F24
        || key == Qt::Key_Up || key == Qt::Key_Down || key == Qt::Key_Left || key == Qt::Key_Right
        || key == Qt::Key_Home || key == Qt::Key_End || key == Qt::Key_PageUp || key == Qt::Key_PageDown;
    if (parts.isEmpty() && !allowBareKey) {
        return {};
    }

    parts.append(keyToken);
    return parts.join('+');
}

void HotkeyEdit::keyPressEvent(QKeyEvent *event) {
    if (!m_listening) {
        QLineEdit::keyPressEvent(event);
        return;
    }
    if (event->key() == Qt::Key_Escape) {
        clearFocus();
        return;
    }
    const QString captured = formatKey(event);
    if (!captured.isEmpty()) {
        setText(captured);
        clearFocus();
    }
}
