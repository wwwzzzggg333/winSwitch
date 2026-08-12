#include "ui/WindowCard.h"
#include "ui/UiIcons.h"

#include <QEvent>
#include <QFontMetrics>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QSize>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>

WindowCard::WindowCard(
    const WindowItem &item,
    const QPixmap &icon,
    const QPixmap &thumbnail,
    bool thumbnailsEnabled,
    bool selected,
    I18n i18n,
    QWidget *parent)
    : QWidget(parent) {
    setObjectName(QStringLiteral("WindowCard"));
    setAttribute(Qt::WA_StyledBackground, true);
    if (selected) {
        setProperty("selected", true);
    }
    style()->unpolish(this);
    style()->polish(this);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(6);

    m_thumbnailBox = new QWidget;
    m_thumbnailBox->setFixedSize(210, 118);
    m_thumbnailBox->setObjectName(QStringLiteral("ThumbBox"));
    m_thumbnailBox->setAttribute(Qt::WA_StyledBackground, true);
    m_thumbnailBox->installEventFilter(this);
    auto *thumbGrid = new QGridLayout(m_thumbnailBox);
    thumbGrid->setContentsMargins(0, 0, 0, 0);
    thumbGrid->setSpacing(0);

    auto *thumbLabel = new QLabel(m_thumbnailBox);
    thumbLabel->setObjectName(QStringLiteral("ThumbnailImage"));
    thumbLabel->setAlignment(Qt::AlignCenter);
    if (!thumbnail.isNull()) {
        thumbLabel->setPixmap(thumbnail.scaled(
            210, 118, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
        thumbGrid->addWidget(thumbLabel, 0, 0);
    } else {
        auto *fallback = new QWidget;
        fallback->setObjectName(QStringLiteral("ThumbnailFallback"));
        auto *fallbackLayout = new QVBoxLayout(fallback);
        fallbackLayout->setAlignment(Qt::AlignCenter);
        fallbackLayout->setSpacing(6);

        auto *glyph = new QLabel;
        glyph->setObjectName(QStringLiteral("ThumbnailFallbackGlyph"));
        glyph->setAlignment(Qt::AlignCenter);
        if (icon.isNull()) {
            glyph->setText(QStringLiteral("▣"));
        } else {
            glyph->setPixmap(icon.scaled(60, 60, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
        fallbackLayout->addWidget(glyph);

        if (thumbnailsEnabled) {
            auto *status = new QLabel(i18n.previewUnavailable());
            status->setObjectName(QStringLiteral("ThumbnailStatus"));
            status->setAlignment(Qt::AlignCenter);
            fallbackLayout->addWidget(status);
        }
        thumbGrid->addWidget(fallback, 0, 0);
    }

    auto *closeBar = new QWidget(m_thumbnailBox);
    closeBar->setAttribute(Qt::WA_TransparentForMouseEvents, false);
    auto *closeBarLayout = new QHBoxLayout(closeBar);
    closeBarLayout->setContentsMargins(0, 4, 4, 0);
    closeBarLayout->addStretch();
    m_closeButton = new QToolButton;
    m_closeButton->setIcon(UiIcons::closeIcon());
    m_closeButton->setIconSize(QSize(12, 12));
    m_closeButton->setToolTip(i18n.closeWindowTooltip());
    m_closeButton->setObjectName(QStringLiteral("CardClose"));
    m_closeButton->setCursor(Qt::PointingHandCursor);
    m_closeButton->hide();
    connect(m_closeButton, &QToolButton::clicked, this, &WindowCard::closeRequested);
    closeBarLayout->addWidget(m_closeButton);
    thumbGrid->addWidget(closeBar, 0, 0, Qt::AlignTop | Qt::AlignRight);

    layout->addWidget(m_thumbnailBox);

    auto *title = new QLabel;
    title->setObjectName(QStringLiteral("CardTitle"));
    title->setFixedWidth(210);
    title->setToolTip(item.title);
    const QFontMetrics fm(title->font());
    title->setText(fm.elidedText(item.title, Qt::ElideRight, 210));
    layout->addWidget(title);

    if (!item.folderPath.isEmpty()) {
        auto *path = new QLabel;
        path->setObjectName(QStringLiteral("CardPath"));
        path->setFixedWidth(210);
        path->setToolTip(item.folderPath);
        path->setText(fm.elidedText(item.folderPath, Qt::ElideRight, 210));
        layout->addWidget(path);
    }

    setFixedWidth(226);
}

bool WindowCard::eventFilter(QObject *watched, QEvent *event) {
    if (watched == m_thumbnailBox) {
        if (event->type() == QEvent::Enter) {
            m_closeButton->show();
        } else if (event->type() == QEvent::Leave) {
            m_closeButton->hide();
        }
    }
    return QWidget::eventFilter(watched, event);
}

void WindowCard::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        emit activateRequested();
    }
    QWidget::mousePressEvent(event);
}
