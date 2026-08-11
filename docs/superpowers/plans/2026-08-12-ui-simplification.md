# winSwitch UI Simplification Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Remove the pinning feature end to end, simplify the settings page, and render a vertically centered card close icon.

**Architecture:** Remove pinning from the configuration, window model, controller, and panel so no hidden state remains. Flatten `SettingsDialog` to a single page while retaining normal configuration persistence. Replace the font glyph used by `WindowCard` with a standard Qt close icon centered by `QToolButton`.

**Tech Stack:** C++17, Qt 6 Widgets, CMake

## Global Constraints

- Ignore the legacy `pinned` JSON field and stop writing it on future saves.
- Keep configuration file loading and saving for all remaining options.
- Do not change window enumeration, activation, closing, search, filtering, or shortcut behavior.
- Remove only the diagnostics UI; retain logging and platform capability implementations.
- Per user request, do not add or run automated tests. Update existing test call sites only when required for compilation.
- Preserve unrelated uncommitted workspace changes.

---

### Task 1: Remove Pinning from Configuration and Window State

**Files:**
- Modify: `src/core/Config.h`
- Modify: `src/core/Config.cpp`
- Modify: `src/core/WindowModel.h`
- Modify: `src/core/WindowModel.cpp`
- Modify: `tests/test_switcher_panel.cpp`

**Interfaces:**
- Produces: `buildGroups(const QList<RawWindow> &, const QStringList &excluded, const QHash<QString, qint64> &groupMru = {}, const QHash<qint64, qint64> &windowMru = {})`
- Removes: `Config::pinned`, `AppGroup::pinned`, and `AppState::setPinned(const QStringList &)`

- [ ] **Step 1: Remove pinned configuration persistence**

Delete the `QStringList pinned` member from `Config`, delete the `pinned` array from JSON serialization/deserialization, and remove it from `Config::operator==`. Unknown `pinned` data in older JSON remains harmless because Qt JSON object readers ignore unrequested keys.

- [ ] **Step 2: Simplify the window model interface**

Change the declaration and definition to:

```cpp
QVector<AppGroup> buildGroups(
    const QList<RawWindow> &raws,
    const QStringList &excluded,
    const QHash<QString, qint64> &groupMru = {},
    const QHash<qint64, qint64> &windowMru = {});
```

Delete `AppGroup::pinned`, all pinned-name normalization and assignments, and `AppState::setPinned`. Keep existing window-count sorting unchanged.

- [ ] **Step 3: Update the existing test call site for the new signature**

Change:

```cpp
buildGroups({raw}, {}, {})
```

to:

```cpp
buildGroups({raw}, {})
```

Do not otherwise alter the user's existing test edits.

### Task 2: Remove Pin Actions from the Panel and Controller

**Files:**
- Modify: `src/ui/MainWindow.h`
- Modify: `src/ui/SwitcherPanel.cpp`
- Modify: `src/app/Application.cpp`
- Modify: `src/core/I18n.h`
- Modify: `src/core/I18n.cpp`
- Modify: `resources/styles/app.qss`

**Interfaces:**
- Consumes: the simplified `buildGroups` signature from Task 1
- Removes: `MainWindow::PanelActionType::TogglePin` and pin-related translations/styles

- [ ] **Step 1: Remove the group pin button**

Delete creation, signal connection, and layout insertion of `pinBtn` in `SwitcherPanel::rebuildContent`. Leave the group title, count, stretch, and “close all” button intact.

- [ ] **Step 2: Remove controller handling**

Pass only `m_config.excluded`, MRU maps, and raw windows to `buildGroups`. Delete the `TogglePin` branch from `ApplicationController::onPanelAction` and remove `TogglePin` from the action enum.

- [ ] **Step 3: Remove unused presentation resources**

Delete the `pin()`, `pinned()`, and `pinnedAppsLabel()` declarations and definitions. Delete both `GroupAction` and `GroupActionPinned` styling because removing the pin button leaves neither selector in use.

### Task 3: Flatten and Simplify the Settings Page

**Files:**
- Modify: `src/ui/SettingsDialog.h`
- Modify: `src/ui/SettingsDialog.cpp`
- Modify: `resources/styles/app.qss`

**Interfaces:**
- Preserves: `SettingsDialog::setConfig(const Config &)` and `saved(const Config &)`
- Removes: diagnostics page construction, import/export UI handlers, and pinned editor state

- [ ] **Step 1: Flatten the layout**

Rename `buildGeneralTab()` to `buildGeneralPage()`. Make `buildUi()` add that page directly to the dialog layout. Remove `QTabWidget`, the diagnostics tab, and the `SettingsTitle` label.

- [ ] **Step 2: Remove hidden settings features**

Delete the pinned editor, import/export button row, `buildDiagnosticsTab`, `onImportConfig`, and `onExportConfig`. Remove their unused Qt and project includes, including platform capability UI dependencies. Delete the now-unused general-tab, diagnostics, capability, and import/export translation methods from `I18n`; retain `settingsTitle()` because it supplies the native settings window title.

- [ ] **Step 3: Preserve remaining save behavior**

Keep hotkey, thumbnail, MRU, language, and excluded-app collection unchanged. `collectFromUi()` must start from `m_config` and update only these remaining fields before emitting `saved`.

- [ ] **Step 4: Remove obsolete settings styles**

Delete the `SettingsTitle` and tab-widget/tab-bar style blocks. Keep shared form, checkbox, group-box, text-edit, and button styles used by the flattened page.

### Task 4: Center the Window Card Close Icon

**Files:**
- Modify: `src/ui/WindowCard.cpp`

**Interfaces:**
- Preserves: `WindowCard::closeRequested()` and object name `CardClose`

- [ ] **Step 1: Replace the baseline-dependent glyph**

Replace:

```cpp
closeBtn->setText(QStringLiteral("×"));
```

with:

```cpp
closeBtn->setIcon(style()->standardIcon(QStyle::SP_TitleBarCloseButton));
closeBtn->setIconSize(QSize(10, 10));
```

Retain the fixed 22 × 22 button styling so Qt centers the icon in both axes.

### Task 5: Static Verification and Handoff

**Files:**
- Inspect: all files modified in Tasks 1–4

**Interfaces:**
- Confirms: no pinning or removed settings UI references remain

- [ ] **Step 1: Scan removed feature references**

Run:

```powershell
rg -n "pinned|TogglePin|GroupActionPinned|buildDiagnosticsTab|onImportConfig|onExportConfig|SettingsTitle" src resources tests
```

Expected: no production-code hits for removed features; documentation and intentional backward-compatibility commentary may remain.

- [ ] **Step 2: Check patch integrity**

Run:

```powershell
git diff --check
git status --short
```

Expected: no whitespace errors and no unrelated files added by this implementation.

- [ ] **Step 3: Hand off manual validation**

Report the modified files and ask the user to verify the main panel, centered close icon, simplified settings page, and remaining configuration save behavior.
