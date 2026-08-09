#include "ui/UiSizing.h"
#include <QtGlobal>

QSize calculatePanelSize(const QSize &availableSize, const QSize &configuredSize) {
    const int availableWidth = qMax(1, availableSize.width());
    const int availableHeight = qMax(1, availableSize.height());
    const int maxWidth = qMax(1, availableWidth - 64);
    const int maxHeight = qMax(1, availableHeight - 96);
    const int autoWidth = qBound(720, static_cast<int>(availableWidth * 0.70), 1280);
    const int autoHeight = qBound(480, static_cast<int>(availableHeight * 0.62), 820);
    const int desiredWidth = qMax(qMax(1, configuredSize.width()), autoWidth);
    const int desiredHeight = qMax(qMax(1, configuredSize.height()), autoHeight);
    return {qMin(desiredWidth, maxWidth), qMin(desiredHeight, maxHeight)};
}
