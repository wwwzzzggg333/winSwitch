#pragma once

#include "core/I18n.h"
#include "core/WindowModel.h"

#include <QLabel>
#include <QPixmap>
#include <QWidget>

class WindowCard : public QWidget {
    Q_OBJECT

public:
    WindowCard(
        const WindowItem &item,
        const QPixmap &icon,
        const QPixmap &thumbnail,
        bool selected,
        I18n i18n,
        QWidget *parent = nullptr);

signals:
    void activateRequested();
    void closeRequested();

private:
    void mousePressEvent(QMouseEvent *event) override;
};
