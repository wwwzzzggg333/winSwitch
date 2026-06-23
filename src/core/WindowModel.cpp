#include "core/WindowModel.h"

#include <QHash>
#include <algorithm>

namespace {

QString exeFileName(const QString &exePath) {
    const int slash = qMax(exePath.lastIndexOf('/'), exePath.lastIndexOf('\\'));
    return exePath.mid(slash + 1).toLower();
}

} // namespace

QVector<AppGroup> buildGroups(
    const QList<RawWindow> &raws,
    const QStringList &pinned,
    const QStringList &excluded) {
    QStringList excludedLc;
    for (const QString &e : excluded) {
        excludedLc.append(e.toLower());
    }
    QStringList pinnedLc;
    for (const QString &p : pinned) {
        pinnedLc.append(p.toLower());
    }

    QStringList order;
    QHash<QString, AppGroup> map;

    for (const RawWindow &r : raws) {
        if (!r.isListable()) {
            continue;
        }
        const QString fname = exeFileName(r.exePath);
        if (excludedLc.contains(fname)) {
            continue;
        }
        if (!map.contains(r.exePath)) {
            order.append(r.exePath);
            AppGroup group;
            group.exePath = r.exePath;
            group.appName = r.appName;
            group.pinned = pinnedLc.contains(fname);
            map.insert(r.exePath, group);
        }
        WindowItem item;
        item.windowId = r.windowId;
        item.title = r.title;
        item.folderPath = r.folderPath;
        map[r.exePath].windows.append(item);
    }

    QVector<AppGroup> groups;
    for (const QString &key : order) {
        AppGroup g = map.take(key);
        if (exeFileName(g.exePath) == QStringLiteral("explorer.exe")) {
            std::sort(g.windows.begin(), g.windows.end(), [](const WindowItem &a, const WindowItem &b) {
                const QString ka = a.folderPath.isEmpty() ? a.title : a.folderPath;
                const QString kb = b.folderPath.isEmpty() ? b.title : b.folderPath;
                return ka.compare(kb, Qt::CaseInsensitive) < 0;
            });
        }
        groups.append(g);
    }

    std::stable_sort(groups.begin(), groups.end(), [](const AppGroup &a, const AppGroup &b) {
        if (a.pinned != b.pinned) {
            return a.pinned > b.pinned;
        }
        return false;
    });
    return groups;
}

AppState AppState::create(const QVector<AppGroup> &groups, const Filter &filter) {
    AppState state;
    state.groups = groups;
    state.filter = filter;
    state.selectedGroup = 0;
    state.selectedWindow = 0;
    return state;
}

QVector<const AppGroup *> AppState::visibleGroups() const {
    QVector<const AppGroup *> out;
    for (const AppGroup &g : groups) {
        if (filter.kind == FilterKind::All) {
            out.append(&g);
        } else if (filter.exePath == g.exePath) {
            out.append(&g);
        }
    }
    return out;
}

qint64 AppState::selectedWindowId() const {
    const QVector<const AppGroup *> vis = visibleGroups();
    if (vis.isEmpty()) {
        return 0;
    }
    const int gi = qBound(0, selectedGroup, vis.size() - 1);
    const AppGroup *g = vis.at(gi);
    if (g->windows.isEmpty()) {
        return 0;
    }
    const int wi = qBound(0, selectedWindow, g->windows.size() - 1);
    return g->windows.at(wi).windowId;
}

void AppState::removeWindow(qint64 windowId) {
    for (AppGroup &g : groups) {
        g.windows.erase(
            std::remove_if(g.windows.begin(), g.windows.end(),
                           [windowId](const WindowItem &w) { return w.windowId == windowId; }),
            g.windows.end());
    }
    groups.erase(
        std::remove_if(groups.begin(), groups.end(),
                       [](const AppGroup &g) { return g.windows.isEmpty(); }),
        groups.end());
    clampSelection();
}

void AppState::removeGroup(const QString &exePath) {
    groups.erase(
        std::remove_if(groups.begin(), groups.end(),
                       [&exePath](const AppGroup &g) { return g.exePath == exePath; }),
        groups.end());
    if (filter.kind == FilterKind::OnlyApp && filter.exePath == exePath) {
        filter.kind = FilterKind::All;
        filter.exePath.clear();
    }
    clampSelection();
}

void AppState::setPinned(const QStringList &pinned) {
    QStringList pinnedLc;
    for (const QString &p : pinned) {
        pinnedLc.append(p.toLower());
    }
    for (AppGroup &g : groups) {
        g.pinned = pinnedLc.contains(exeFileName(g.exePath));
    }
    std::stable_sort(groups.begin(), groups.end(), [](const AppGroup &a, const AppGroup &b) {
        if (a.pinned != b.pinned) {
            return a.pinned > b.pinned;
        }
        return false;
    });
    clampSelection();
}

void AppState::clampSelection() {
    const QVector<const AppGroup *> vis = visibleGroups();
    if (vis.isEmpty()) {
        selectedGroup = 0;
        selectedWindow = 0;
        return;
    }
    selectedGroup = qBound(0, selectedGroup, vis.size() - 1);
    const int wlen = vis.at(selectedGroup)->windows.size();
    selectedWindow = wlen == 0 ? 0 : qBound(0, selectedWindow, wlen - 1);
}

void AppState::moveSelection(int dy, int dx) {
    const QVector<const AppGroup *> vis = visibleGroups();
    if (vis.isEmpty()) {
        return;
    }
    int gi = qBound(0, selectedGroup, vis.size() - 1);
    int wi = selectedWindow;

    if (dx != 0) {
        const int len = vis.at(gi)->windows.size();
        if (len > 0) {
            wi = ((wi + dx) % len + len) % len;
        }
    }
    if (dy > 0) {
        if (wi + 1 < vis.at(gi)->windows.size()) {
            wi += 1;
        } else if (gi + 1 < vis.size()) {
            gi += 1;
            wi = 0;
        }
    } else if (dy < 0) {
        if (wi > 0) {
            wi -= 1;
        } else if (gi > 0) {
            gi -= 1;
            wi = qMax(0, vis.at(gi)->windows.size() - 1);
        }
    }
    selectedGroup = gi;
    selectedWindow = wi;
}
