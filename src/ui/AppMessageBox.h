#pragma once

#include <QString>

class QWidget;

enum class AppMessageIcon { Information, Warning };

void showAppMessage(QWidget *parent, const QString &title, const QString &text, AppMessageIcon icon);
