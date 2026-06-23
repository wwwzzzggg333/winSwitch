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
    if (event->modifiers() & Qt::ControlModifier) {
        parts.append(QStringLiteral("Ctrl"));
    }
    if (event->modifiers() & Qt::AltModifier) {
        parts.append(QStringLiteral("Alt"));
    }
    if (event->modifiers() & Qt::ShiftModifier) {
        parts.append(QStringLiteral("Shift"));
    }
    if (event->modifiers() & Qt::MetaModifier) {
        parts.append(QStringLiteral("Win"));
    }
    const int key = event->key();
    if (key >= Qt::Key_A && key <= Qt::Key_Z) {
        parts.append(QChar(key));
    } else if (key == Qt::Key_Space) {
        parts.append(QStringLiteral("Space"));
    } else if (key == Qt::Key_QuoteLeft) {
        parts.append(QStringLiteral("`"));
    } else if (key >= Qt::Key_F1 && key <= Qt::Key_F24) {
        parts.append(QStringLiteral("F%1").arg(key - Qt::Key_F1 + 1));
    } else {
        return {};
    }
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