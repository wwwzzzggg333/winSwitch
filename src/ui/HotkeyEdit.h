#pragma once

#include "core/I18n.h"

#include <QLineEdit>

class HotkeyEdit : public QLineEdit {
    Q_OBJECT

public:
    explicit HotkeyEdit(I18n i18n, QWidget *parent = nullptr);

    QString hotkey() const;
    void setHotkey(const QString &hotkey);

protected:
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    QString formatKey(QKeyEvent *event) const;

    I18n m_i18n;
    bool m_listening = false;
};
