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
        bool thumbnailsEnabled,
        bool selected,
        I18n i18n,
        QWidget *parent = nullptr);

signals:
    void activateRequested();
    void closeRequested();

private:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

    QWidget *m_thumbnailBox = nullptr;
    class QToolButton *m_closeButton = nullptr;
};
