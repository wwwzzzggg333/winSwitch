#pragma once

#include "core/I18n.h"
#include "core/WindowModel.h"
#include "ui/MainWindow.h"

#include <QHash>
#include <QPixmap>
#include <QWidget>

class SwitcherPanel : public QWidget {
    Q_OBJECT

public:
    explicit SwitcherPanel(I18n i18n, QWidget *parent = nullptr);

    void setData(
        const AppState &state,
        const QHash<qint64, QPixmap> &icons,
        const QHash<qint64, QPixmap> &thumbs,
        bool showThumbnails);
    void updateTextures(const QHash<qint64, QPixmap> &icons, const QHash<qint64, QPixmap> &thumbs);

signals:
    void actionTriggered(MainWindow::PanelAction action);

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    void rebuild();
    void emitAction(MainWindow::PanelAction action);

    I18n m_i18n;
    AppState m_state;
    QHash<qint64, QPixmap> m_icons;
    QHash<qint64, QPixmap> m_thumbs;
    bool m_showThumbnails = true;
};
