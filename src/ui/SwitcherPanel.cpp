#include "ui/SwitcherPanel.h"
#include "ui/WindowCard.h"

#include "core/AppLog.h"

#include <QFrame>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QLabel>
#include <QKeyEvent>
#include <QLineEdit>
#include <QScrollArea>
#include <QScrollBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWheelEvent>

SwitcherPanel::SwitcherPanel(I18n i18n, QWidget *parent) : QWidget(parent), m_i18n(i18n) {
    setObjectName(QStringLiteral("SwitcherPanel"));
    setAttribute(Qt::WA_StyledBackground, true);
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(12, 12, 12, 12);
    root->setSpacing(10);

    m_searchEdit = new QLineEdit;
    m_searchEdit->setObjectName(QStringLiteral("SearchEdit"));
    m_searchEdit->setPlaceholderText(m_i18n.searchPlaceholder());
    m_searchEdit->setClearButtonEnabled(true);
    m_searchEdit->setFixedHeight(40);
    m_searchEdit->installEventFilter(this);
    connect(m_searchEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
        m_state.setSearchText(text);
        rebuildContent();
    });
    root->addWidget(m_searchEdit);

    m_filterScroll = new QScrollArea;
    m_filterScroll->setObjectName(QStringLiteral("FilterScroll"));
    m_filterScroll->setWidgetResizable(true);
    m_filterScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_filterScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_filterScroll->setFrameShape(QFrame::NoFrame);
    m_filterScroll->setFixedHeight(48);
    m_filterScroll->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_filterScroll->viewport()->setObjectName(QStringLiteral("FilterViewport"));
    m_filterScroll->viewport()->installEventFilter(this);

    m_filterRow = new QWidget;
    m_filterRow->setObjectName(QStringLiteral("FilterRow"));
    m_filterRow->setAttribute(Qt::WA_StyledBackground, true);
    m_filterRow->setMinimumHeight(36);
    m_filterLayout = new QHBoxLayout(m_filterRow);
    m_filterLayout->setContentsMargins(0, 2, 0, 2);
    m_filterLayout->setSpacing(8);
    m_filterScroll->setWidget(m_filterRow);
    root->addWidget(m_filterScroll);

    m_contentScroll = new QScrollArea;
    m_contentScroll->setObjectName(QStringLiteral("ContentScroll"));
    m_contentScroll->setWidgetResizable(true);
    m_contentScroll->setFrameShape(QFrame::NoFrame);
    m_contentScroll->viewport()->setObjectName(QStringLiteral("ContentViewport"));
    m_contentWidget = new QWidget;
    m_contentWidget->setObjectName(QStringLiteral("PanelContent"));
    m_contentWidget->setAttribute(Qt::WA_StyledBackground, true);
    m_contentLayout = new QVBoxLayout(m_contentWidget);
    m_contentLayout->setContentsMargins(0, 4, 0, 8);
    m_contentLayout->setSpacing(12);
    m_contentScroll->setWidget(m_contentWidget);
    root->addWidget(m_contentScroll, 1);
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
    m_searchEdit->blockSignals(true);
    m_searchEdit->clear();
    m_searchEdit->blockSignals(false);
    rebuild();
}

void SwitcherPanel::focusSearch() {
    if (m_searchEdit) {
        m_searchEdit->setFocus(Qt::ShortcutFocusReason);
    }
}

void SwitcherPanel::updateTextures(const QHash<qint64, QPixmap> &icons, const QHash<qint64, QPixmap> &thumbs) {
    m_icons = icons;
    m_thumbs = thumbs;
    AppLog::info(QStringLiteral("updateTextures: rebuilding content, icons=%1 thumbs=%2")
                     .arg(m_icons.size())
                     .arg(m_thumbs.size()));
    rebuildContent();
    AppLog::info(QStringLiteral("updateTextures: rebuild done"));
}

void SwitcherPanel::clearLayout(QLayout *layout) {
    if (!layout) {
        return;
    }
    QLayoutItem *child = nullptr;
    while ((child = layout->takeAt(0)) != nullptr) {
        if (QWidget *widget = child->widget()) {
            widget->deleteLater();
        } else if (QLayout *sub = child->layout()) {
            clearLayout(sub);
        }
        delete child;
    }
}

void SwitcherPanel::rebuild() {
    rebuildFilters();
    rebuildContent();
}

void SwitcherPanel::rebuildFilters() {
    if (!m_filterLayout) {
        return;
    }
    clearLayout(m_filterLayout);

    auto makeFilterButton = [this](const QString &label, bool selected, const QString &toolTip = QString()) {
        constexpr int filterButtonMaxWidth = 180;
        constexpr int filterTextMaxWidth = 148;
        auto *btn = new QToolButton;
        btn->setText(btn->fontMetrics().elidedText(label, Qt::ElideRight, filterTextMaxWidth));
        btn->setToolTip(toolTip);
        btn->setCheckable(true);
        btn->setChecked(selected);
        btn->setObjectName(QStringLiteral("FilterPill"));
        btn->setMaximumWidth(filterButtonMaxWidth);
        btn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        return btn;
    };

    auto *allBtn = makeFilterButton(m_i18n.filterAll(), m_state.filter.kind == FilterKind::All);
    connect(allBtn, &QToolButton::clicked, this, [this]() {
        m_state.filter.kind = FilterKind::All;
        m_state.selectedGroup = 0;
        m_state.selectedWindow = 0;
        rebuild();
    });
    m_filterLayout->addWidget(allBtn);

    for (const AppGroup &g : m_state.groups) {
        const bool on = m_state.filter.kind == FilterKind::OnlyApp && m_state.filter.exePath == g.exePath;
        auto *btn = makeFilterButton(
            QStringLiteral("%1 %2").arg(g.appName).arg(g.windows.size()), on, g.appName);
        connect(btn, &QToolButton::clicked, this, [this, g]() {
            m_state.filter.kind = FilterKind::OnlyApp;
            m_state.filter.exePath = g.exePath;
            m_state.selectedGroup = 0;
            m_state.selectedWindow = 0;
            rebuild();
        });

        auto *closeChip = new QToolButton;
        closeChip->setText(QStringLiteral("×"));
        closeChip->setToolTip(m_i18n.closeGroupTooltip());
        closeChip->setObjectName(QStringLiteral("CloseChip"));
        closeChip->setCursor(Qt::PointingHandCursor);
        connect(closeChip, &QToolButton::clicked, this, [this, g]() {
            MainWindow::PanelAction action;
            action.type = MainWindow::PanelActionType::CloseGroup;
            action.exePath = g.exePath;
            emit actionTriggered(action);
        });

        auto *chip = new QFrame;
        chip->setObjectName(on ? QStringLiteral("FilterChipActive") : QStringLiteral("FilterChip"));
        chip->setAttribute(Qt::WA_StyledBackground, true);
        chip->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        auto *chipLayout = new QHBoxLayout(chip);
        chipLayout->setContentsMargins(2, 2, 4, 2);
        chipLayout->setSpacing(0);
        chipLayout->addWidget(btn);
        chipLayout->addWidget(closeChip);
        m_filterLayout->addWidget(chip);
    }
    m_filterLayout->addStretch();
}

void SwitcherPanel::rebuildContent() {
    if (!m_contentLayout) {
        return;
    }
    AppLog::info(QStringLiteral("rebuildContent: start, groups=%1 width=%2")
                     .arg(m_state.visibleGroups().size())
                     .arg(width()));
    clearLayout(m_contentLayout);
    AppLog::info(QStringLiteral("rebuildContent: layout cleared"));

    const QVector<AppGroup> groups = m_state.visibleGroups();
    if (groups.isEmpty()) {
        auto *emptyState = new QWidget;
        emptyState->setObjectName(QStringLiteral("EmptyState"));
        auto *layout = new QVBoxLayout(emptyState);
        layout->setAlignment(Qt::AlignCenter);
        layout->setSpacing(8);
        const bool searching = !m_state.searchText.trimmed().isEmpty();
        auto *glyph = new QLabel(QStringLiteral("⌕"));
        glyph->setObjectName(QStringLiteral("EmptyStateGlyph"));
        glyph->setAlignment(Qt::AlignCenter);
        layout->addWidget(glyph);
        auto *title = new QLabel(searching ? m_i18n.noMatchingWindows()
                                           : m_i18n.noSwitchableWindows());
        title->setObjectName(QStringLiteral("EmptyStateTitle"));
        title->setAlignment(Qt::AlignCenter);
        layout->addWidget(title);
        auto *hint = new QLabel(searching ? m_i18n.noMatchingWindowsHint()
                                          : m_i18n.emptyWindowsHint());
        hint->setObjectName(QStringLiteral("EmptyStateHint"));
        hint->setAlignment(Qt::AlignCenter);
        hint->setWordWrap(true);
        layout->addWidget(hint);
        m_contentLayout->addWidget(emptyState, 1);
        return;
    }
    for (int gi = 0; gi < groups.size(); ++gi) {
        const AppGroup &g = groups.at(gi);
        AppLog::info(QStringLiteral("rebuildContent: group %1/%2 app=%3 windows=%4")
                         .arg(gi + 1)
                         .arg(groups.size())
                         .arg(g.appName)
                         .arg(g.windows.size()));
        auto *header = new QHBoxLayout;
        auto *title = new QLabel(g.appName);
        title->setObjectName(QStringLiteral("GroupTitle"));
        header->addWidget(title);
        auto *count = new QLabel(m_i18n.windowCount(g.windows.size()));
        count->setObjectName(QStringLiteral("GroupCount"));
        header->addWidget(count);
        header->addStretch();

        auto *pinBtn = new QToolButton;
        pinBtn->setObjectName(g.pinned ? QStringLiteral("GroupActionPinned") : QStringLiteral("GroupAction"));
        pinBtn->setText(g.pinned ? m_i18n.pinned() : m_i18n.pin());
        pinBtn->setCursor(Qt::PointingHandCursor);
        connect(pinBtn, &QToolButton::clicked, this, [this, g]() {
            MainWindow::PanelAction action;
            action.type = MainWindow::PanelActionType::TogglePin;
            action.exePath = g.exePath;
            emit actionTriggered(action);
        });
        header->addWidget(pinBtn);

        auto *closeAllBtn = new QToolButton;
        closeAllBtn->setObjectName(QStringLiteral("GroupCloseAction"));
        closeAllBtn->setText(m_i18n.closeAllGroup());
        closeAllBtn->setCursor(Qt::PointingHandCursor);
        connect(closeAllBtn, &QToolButton::clicked, this, [this, g]() {
            MainWindow::PanelAction action;
            action.type = MainWindow::PanelActionType::CloseGroup;
            action.exePath = g.exePath;
            emit actionTriggered(action);
        });
        header->addWidget(closeAllBtn);
        m_contentLayout->addLayout(header);

        auto *gridHost = new QWidget;
        gridHost->setObjectName(QStringLiteral("CardGridHost"));
        gridHost->setAttribute(Qt::WA_StyledBackground, true);
        auto *rowsLayout = new QVBoxLayout(gridHost);
        rowsLayout->setContentsMargins(0, 0, 0, 0);
        rowsLayout->setSpacing(10);
        rowsLayout->setAlignment(Qt::AlignLeft | Qt::AlignTop);

        const int perRow = qMax(1, (width() - 32) / 236);
        QHBoxLayout *rowLayout = nullptr;
        int col = 0;
        for (int wi = 0; wi < g.windows.size(); ++wi) {
            if (col == 0) {
                rowLayout = new QHBoxLayout;
                rowLayout->setContentsMargins(0, 0, 0, 0);
                rowLayout->setSpacing(10);
                rowLayout->setAlignment(Qt::AlignLeft | Qt::AlignTop);
                rowsLayout->addLayout(rowLayout);
            }
            const WindowItem &item = g.windows.at(wi);
            const bool selected = m_state.selectedGroup == gi && m_state.selectedWindow == wi;
            auto *card = new WindowCard(
                item,
                m_icons.value(item.windowId),
                m_showThumbnails ? m_thumbs.value(item.windowId) : QPixmap(),
                m_showThumbnails,
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
            rowLayout->addWidget(card);
            col = (col + 1) % perRow;
        }
        m_contentLayout->addWidget(gridHost);
    }
    m_contentLayout->addStretch();
    AppLog::info(QStringLiteral("rebuildContent: done"));
}

bool SwitcherPanel::eventFilter(QObject *obj, QEvent *event) {
    if (m_filterScroll && obj == m_filterScroll->viewport() && event->type() == QEvent::Wheel) {
        auto *wheel = static_cast<QWheelEvent *>(event);
        QScrollBar *bar = m_filterScroll->horizontalScrollBar();
        if (bar && bar->maximum() > bar->minimum()) {
            const int verticalDelta = !wheel->pixelDelta().isNull()
                ? wheel->pixelDelta().y()
                : wheel->angleDelta().y();
            if (verticalDelta != 0) {
                int delta = !wheel->pixelDelta().isNull()
                    ? -verticalDelta
                    : -(verticalDelta * 48) / 120;
                if (delta == 0) {
                    delta = verticalDelta > 0 ? -1 : 1;
                }
                bar->setValue(bar->value() + delta);
                wheel->accept();
                return true;
            }
        }
    }
    if (obj == m_searchEdit && event->type() == QEvent::KeyPress) {
        auto *ke = static_cast<QKeyEvent *>(event);
        switch (ke->key()) {
        case Qt::Key_Up:
        case Qt::Key_Down:
        case Qt::Key_Return:
        case Qt::Key_Enter:
        case Qt::Key_Escape:
            keyPressEvent(ke);
            return true;
        default:
            break;
        }
    }
    return QWidget::eventFilter(obj, event);
}

void SwitcherPanel::keyPressEvent(QKeyEvent *event) {
    switch (event->key()) {
    case Qt::Key_Escape:
        if (!m_searchEdit->text().isEmpty()) {
            m_searchEdit->clear();
            break;
        }
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
        rebuildContent();
        break;
    case Qt::Key_Down:
        m_state.moveSelection(1, 0);
        rebuildContent();
        break;
    case Qt::Key_Left:
        m_state.moveSelection(0, -1);
        rebuildContent();
        break;
    case Qt::Key_Right:
        m_state.moveSelection(0, 1);
        rebuildContent();
        break;
    default:
        QWidget::keyPressEvent(event);
    }
}

void SwitcherPanel::emitAction(MainWindow::PanelAction action) {
    emit actionTriggered(action);
}
