#include "ui/AppMessageBox.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QStyle>
#include <QVBoxLayout>

void showAppMessage(QWidget *parent, const QString &title, const QString &text, AppMessageIcon icon) {
    QDialog dlg(parent);
    dlg.setObjectName(QStringLiteral("AppMessageBox"));
    dlg.setWindowTitle(title);
    dlg.setModal(true);
    dlg.setMinimumWidth(360);
    dlg.setAttribute(Qt::WA_StyledBackground, true);

    auto *root = new QVBoxLayout(&dlg);
    root->setContentsMargins(20, 18, 20, 16);
    root->setSpacing(16);

    auto *row = new QHBoxLayout;
    row->setSpacing(14);
    row->setContentsMargins(0, 0, 0, 0);

    auto *iconLabel = new QLabel;
    iconLabel->setObjectName(QStringLiteral("AppMessageIcon"));
    const QStyle::StandardPixmap pixmap = icon == AppMessageIcon::Warning ? QStyle::SP_MessageBoxWarning
                                                                          : QStyle::SP_MessageBoxInformation;
    iconLabel->setPixmap(dlg.style()->standardIcon(pixmap).pixmap(32, 32));
    iconLabel->setFixedSize(32, 32);
    iconLabel->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    auto *textLabel = new QLabel(text);
    textLabel->setObjectName(QStringLiteral("AppMessageText"));
    textLabel->setWordWrap(true);
    textLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    textLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    row->addWidget(iconLabel, 0, Qt::AlignTop);
    row->addWidget(textLabel, 1);
    root->addLayout(row);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok);
    buttons->setObjectName(QStringLiteral("AppMessageButtons"));
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    root->addWidget(buttons, 0, Qt::AlignRight);

    dlg.exec();
}
