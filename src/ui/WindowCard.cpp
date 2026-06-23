#include "ui/WindowCard.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
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
    if (selected) {
        setProperty("selected", true);
    }

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(4);

    auto *thumbBox = new QWidget;
    thumbBox->setFixedSize(210, 118);
    thumbBox->setObjectName(QStringLiteral("ThumbBox"));
    auto *thumbLayout = new QVBoxLayout(thumbBox);
    thumbLayout->setContentsMargins(0, 0, 0, 0);
    auto *thumbLabel = new QLabel;
    thumbLabel->setAlignment(Qt::AlignCenter);
    if (!thumbnail.isNull()) {
        thumbLabel->setPixmap(thumbnail.scaled(210, 118, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    } else if (!icon.isNull()) {
        thumbLabel->setPixmap(icon.scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    thumbLayout->addWidget(thumbLabel);

    auto *closeBtn = new QToolButton(thumbBox);
    closeBtn->setText(QStringLiteral("×"));
    closeBtn->setToolTip(i18n.closeWindowTooltip());
    closeBtn->setObjectName(QStringLiteral("CardClose"));
    closeBtn->move(184, 4);
    connect(closeBtn, &QToolButton::clicked, this, &WindowCard::closeRequested);

    layout->addWidget(thumbBox);

    auto *title = new QLabel(item.title);
    title->setWordWrap(false);
    title->setObjectName(QStringLiteral("CardTitle"));
    layout->addWidget(title);

    if (!item.folderPath.isEmpty()) {
        auto *path = new QLabel(item.folderPath);
        path->setObjectName(QStringLiteral("CardPath"));
        layout->addWidget(path);
    }

    setFixedWidth(222);
}

void WindowCard::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        emit activateRequested();
    }
    QWidget::mousePressEvent(event);
}