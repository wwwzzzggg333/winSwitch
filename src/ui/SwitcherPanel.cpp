#include "ui/SwitcherPanel.h"
#include "ui/WindowCard.h"

#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QKeyEvent>
#include <QScrollArea>
#include <QToolButton>
#include <QVBoxLayout>

SwitcherPanel::SwitcherPanel(I18n i18n, QWidget *parent) : QWidget(parent), m_i18n(i18n) {
    setObjectName(QStringLiteral("SwitcherPanel"));
}

void SwitcherPanel::setData(
    const AppState &state,
    const QHash<qint64, QPixmap> &icons,
    const QHash<qint64, QPixmap> &thumbs,
    bool showThumbnails) {
    m_state = state;
    m_icons = icons;
    m_thumbs = thumbs;
    m_showThumbnails = showThumbnails;
    rebuild();
}

void SwitcherPanel::updateTextures(const QHash<qint64, QPixmap> &icons, const QHash<qint64, QPixmap> &thumbs) {
    m_icons = icons;
    m_thumbs = thumbs;
    rebuild();
}

void SwitcherPanel::rebuild() {
    if (layout()) {
        QLayoutItem *child = nullptr;
        while ((child = layout()->takeAt(0)) != nullptr) {
            delete child->widget();
            delete child;
        }
        delete layout();
    }

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(10, 10, 10, 10);
    root->setSpacing(8);

    auto *filterScroll = new QScrollArea;
    filterScroll->setWidgetResizable(true);
    filterScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    filterScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    filterScroll->setFrameShape(QFrame::NoFrame);
    auto *filterRow = new QWidget;
    auto *filterLayout = new QHBoxLayout(filterRow);
    filterLayout->setContentsMargins(0, 0, 0, 0);
    filterLayout->setSpacing(8);

    auto makeFilterButton = [this](const QString &label, bool selected) {
        auto *btn = new QToolButton;
        btn->setText(label);
        btn->setCheckable(true);
        btn->setChecked(selected);
        btn->setObjectName(QStringLiteral("FilterPill"));
        return btn;
    };

    auto *allBtn = makeFilterButton(m_i18n.filterAll(), m_state.filter.kind == FilterKind::All);
    connect(allBtn, &QToolButton::clicked, this, [this]() {
        m_state.filter.kind = FilterKind::All;
        m_state.selectedGroup = 0;
        m_state.selectedWindow = 0;
        rebuild();
    });
    filterLayout->addWidget(allBtn);

    for (const AppGroup &g : m_state.groups) {
        const bool on = m_state.filter.kind == FilterKind::OnlyApp && m_state.filter.exePath == g.exePath;
        auto *btn = makeFilterButton(QStringLiteral("%1 %2").arg(g.appName).arg(g.windows.size()), on);
        connect(btn, &QToolButton::clicked, this, [this, g]() {
            m_state.filter.kind = FilterKind::OnlyApp;
            m_state.filter.exePath = g.exePath;
            m_state.selectedGroup = 0;
            m_state.selectedWindow = 0;
            rebuild();
        });
        filterLayout->addWidget(btn);

        auto *closeChip = new QToolButton;
        closeChip->setText(QStringLiteral("×"));
        closeChip->setToolTip(m_i18n.closeGroupTooltip());
        closeChip->setObjectName(QStringLiteral("CloseChip"));
        connect(closeChip, &QToolButton::clicked, this, [this, g]() {
            MainWindow::PanelAction action;
            action.type = MainWindow::PanelActionType::CloseGroup;
            action.exePath = g.exePath;
            emit actionTriggered(action);
        });
        filterLayout->addWidget(closeChip);
    }
    filterLayout->addStretch();
    filterScroll->setWidget(filterRow);
    root->addWidget(filterScroll);

    auto *contentScroll = new QScrollArea;
    contentScroll->setWidgetResizable(true);
    contentScroll->setFrameShape(QFrame::NoFrame);
    auto *content = new QWidget;
    auto *contentLayout = new QVBoxLayout(content);
    contentLayout->setSpacing(12);

    const QVector<const AppGroup *> groups = m_state.visibleGroups();
    for (int gi = 0; gi < groups.size(); ++gi) {
        const AppGroup *g = groups.at(gi);
        auto *header = new QHBoxLayout;
        auto *title = new QLabel(QStringLiteral("%1 %2").arg(g->appName, m_i18n.windowCount(g->windows.size())));
        title->setObjectName(QStringLiteral("GroupTitle"));
        header->addWidget(title);
        header->addStretch();

        auto *pinBtn = new QToolButton;
        pinBtn->setText(g->pinned ? m_i18n.pinned() : m_i18n.pin());
        connect(pinBtn, &QToolButton::clicked, this, [this, g]() {
            MainWindow::PanelAction action;
            action.type = MainWindow::PanelActionType::TogglePin;
            action.exePath = g->exePath;
            emit actionTriggered(action);
        });
        header->addWidget(pinBtn);

        auto *closeAllBtn = new QToolButton;
        closeAllBtn->setText(m_i18n.closeAllGroup());
        connect(closeAllBtn, &QToolButton::clicked, this, [this, g]() {
            MainWindow::PanelAction action;
            action.type = MainWindow::PanelActionType::CloseGroup;
            action.exePath = g->exePath;
            emit actionTriggered(action);
        });
        header->addWidget(closeAllBtn);
        contentLayout->addLayout(header);

        auto *gridHost = new QWidget;
        auto *grid = new QGridLayout(gridHost);
        grid->setSpacing(10);
        const int perRow = qMax(1, width() / 230);
        for (int wi = 0; wi < g->windows.size(); ++wi) {
            const WindowItem &item = g->windows.at(wi);
            const bool selected = m_state.selectedGroup == gi && m_state.selectedWindow == wi;
            auto *card = new WindowCard(
                item,
                m_icons.value(item.windowId),
                m_showThumbnails ? m_thumbs.value(item.windowId) : QPixmap(),
                selected,
                m_i18n);
            connect(card, &WindowCard::activateRequested, this, [this, item]() {
                MainWindow::PanelAction action;
                action.type = MainWindow::PanelActionType::Activate;
                action.windowId = item.windowId;
                emit actionTriggered(action);
            });
            connect(card, &WindowCard::closeRequested, this, [this, item]() {
                MainWindow::PanelAction action;
                action.type = MainWindow::PanelActionType::CloseWindow;
                action.windowId = item.windowId;
                emit actionTriggered(action);
            });
            grid->addWidget(card, wi / perRow, wi % perRow);
        }
        contentLayout->addWidget(gridHost);
    }
    contentLayout->addStretch();
    contentScroll->setWidget(content);
    root->addWidget(contentScroll, 1);
}

void SwitcherPanel::keyPressEvent(QKeyEvent *event) {
    switch (event->key()) {
    case Qt::Key_Escape:
        emitAction({MainWindow::PanelActionType::Dismiss});
        break;
    case Qt::Key_Return:
    case Qt::Key_Enter: {
        const qint64 id = m_state.selectedWindowId();
        if (id != 0) {
            MainWindow::PanelAction action;
            action.type = MainWindow::PanelActionType::Activate;
            action.windowId = id;
            emit actionTriggered(action);
        }
        break;
    }
    case Qt::Key_Up:
        m_state.moveSelection(-1, 0);
        rebuild();
        break;
    case Qt::Key_Down:
        m_state.moveSelection(1, 0);
        rebuild();
        break;
    case Qt::Key_Left:
        m_state.moveSelection(0, -1);
        rebuild();
        break;
    case Qt::Key_Right:
        m_state.moveSelection(0, 1);
        rebuild();
        break;
    default:
        QWidget::keyPressEvent(event);
    }
}

void SwitcherPanel::emitAction(MainWindow::PanelAction action) {
    emit actionTriggered(action);
}