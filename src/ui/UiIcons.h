#pragma once

#include <QIcon>
#include <QPainter>
#include <QPixmap>

namespace UiIcons {

inline const QIcon &closeIcon() {
    static const QIcon icon = []() {
        QPixmap pixmap(12, 12);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(QPen(Qt::white, 1.8, Qt::SolidLine, Qt::RoundCap));
        painter.drawLine(QPointF(2.5, 2.5), QPointF(9.5, 9.5));
        painter.drawLine(QPointF(9.5, 2.5), QPointF(2.5, 9.5));
        return QIcon(pixmap);
    }();
    return icon;
}

} // namespace UiIcons
