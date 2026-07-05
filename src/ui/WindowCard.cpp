#include "ui/WindowCard.h"

#include <QFontMetrics>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>

WindowCard::WindowCard(
    const WindowItem &item,
    const QPixmap &icon,
    const QPixmap &thumbnail,
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

    auto *thumbBox = new QWidget;
    thumbBox->setFixedSize(210, 118);
    thumbBox->setObjectName(QStringLiteral("ThumbBox"));
    thumbBox->setAttribute(Qt::WA_StyledBackground, true);
    auto *thumbGrid = new QGridLayout(thumbBox);
    thumbGrid->setContentsMargins(0, 0, 0, 0);
    thumbGrid->setSpacing(0);

    auto *thumbLabel = new QLabel;
    thumbLabel->setAlignment(Qt::AlignCenter);
    thumbLabel->setScaledContents(false);
    if (!thumbnail.isNull()) {
        thumbLabel->setPixmap(thumbnail.scaled(210, 118, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    } else if (!icon.isNull()) {
        thumbLabel->setPixmap(icon.scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    thumbGrid->addWidget(thumbLabel, 0, 0);

    auto *closeBar = new QWidget(thumbBox);
    closeBar->setAttribute(Qt::WA_TransparentForMouseEvents, false);
    auto *closeBarLayout = new QHBoxLayout(closeBar);
    closeBarLayout->setContentsMargins(0, 4, 4, 0);
    closeBarLayout->addStretch();
    auto *closeBtn = new QToolButton;
    closeBtn->setText(QStringLiteral("×"));
    closeBtn->setToolTip(i18n.closeWindowTooltip());
    closeBtn->setObjectName(QStringLiteral("CardClose"));
    closeBtn->setCursor(Qt::PointingHandCursor);
    connect(closeBtn, &QToolButton::clicked, this, &WindowCard::closeRequested);
    closeBarLayout->addWidget(closeBtn);
    thumbGrid->addWidget(closeBar, 0, 0, Qt::AlignTop | Qt::AlignRight);

    layout->addWidget(thumbBox);

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

void WindowCard::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        emit activateRequested();
    }
    QWidget::mousePressEvent(event);
}
