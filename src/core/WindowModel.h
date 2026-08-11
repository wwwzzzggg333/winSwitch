#pragma once

#include "platform/WindowInfo.h"

#include <QHash>
#include <QString>
#include <QVector>

struct WindowItem {
    qint64 windowId = 0;
    QString title;
    QString folderPath;
};

struct AppGroup {
    QString exePath;
    QString appName;
    QVector<WindowItem> windows;
};

enum class FilterKind {
    All,
    OnlyApp
};

struct Filter {
    FilterKind kind = FilterKind::All;
    QString exePath;
};

struct AppState {
    QVector<AppGroup> groups;
    Filter filter;
    QString searchText;
    int selectedGroup = 0;
    int selectedWindow = 0;

    static AppState create(const QVector<AppGroup> &groups, const Filter &filter);

    QVector<AppGroup> visibleGroups() const;
    qint64 selectedWindowId() const;

    void setSearchText(const QString &text);
    void removeWindow(qint64 windowId);
    void removeGroup(const QString &exePath);
    void moveSelection(int dy, int dx);

private:
    void clampSelection();
};

QVector<AppGroup> buildGroups(
    const QList<RawWindow> &raws,
    const QStringList &excluded,
    const QHash<QString, qint64> &groupMru = {},
    const QHash<qint64, qint64> &windowMru = {});
