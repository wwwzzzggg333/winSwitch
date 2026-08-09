# Windows UI Light Polish Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Polish the existing Windows 11 interface without changing its search/filter/group/card structure, while adding tests for sizing and visible UI states.

**Architecture:** Keep the current `core / app / platform / ui` layering and QWidget composition. Extract screen-safe sizing into a pure helper, derive empty and preview states from existing inputs, and isolate the native Windows title-bar request inside `MainWindow.cpp`.

**Tech Stack:** C++17, Qt 6.8 Widgets/Core/Gui/Test, CMake 3.16+, MSVC 2022, Win32 DWM API.

## Global Constraints

- Preserve the top search, horizontal app filters, group headers, and card grid.
- Do not add a sidebar, third-party UI library, animation framework, or theme switcher.
- Group close remains immediate and must not show a confirmation dialog.
- Do not change window enumeration, search matching, MRU, capture, activation, or close APIs.
- All new user-visible text must be provided by `I18n` in Chinese and English.
- Dark-title-bar failure logs and falls back; it never blocks startup or display.
- Keep `deleteLater()` in `SwitcherPanel::clearLayout()` to avoid regressing the rebuild crash.
- Preserve unrelated changes in `docs/README.md`, `docs/project-maintenance-guide.md`, and `cursor-recovered-chats/`.

---

## File Structure

**Create:**

- `src/ui/UiSizing.h`: declares a pure panel-size calculation function.
- `src/ui/UiSizing.cpp`: implements screen-safe size calculation.
- `tests/test_ui_sizing.cpp`: covers exact size results and bounds.
- `tests/test_switcher_panel.cpp`: covers card fallbacks, empty states, and filter scrolling.

**Modify:**

- `CMakeLists.txt`: registers `UiSizing.cpp` and Qt Test/CTest targets.
- `src/ui/MainWindow.cpp`: uses the size helper and requests dark native decoration.
- `src/ui/SwitcherPanel.h/.cpp`: empty states and horizontal wheel routing.
- `src/ui/WindowCard.h/.cpp`: distinct disabled/unavailable preview states.
- `src/core/I18n.h/.cpp`: bilingual preview and empty-state copy.
- `resources/styles/app.qss`: typography, state, filter, action, and card polish.

---

### Task 1: Test Foundation and Screen-Safe Sizing

**Files:**
- Create: `src/ui/UiSizing.h`
- Create: `src/ui/UiSizing.cpp`
- Create: `tests/test_ui_sizing.cpp`
- Modify: `CMakeLists.txt:10-39,52-68`
- Modify: `src/ui/MainWindow.cpp:72-87`

**Interfaces:**
- Consumes: `QSize availableSize`, `QSize configuredSize`.
- Produces: `QSize calculatePanelSize(const QSize &, const QSize &)`.
- Output dimensions are at least 1 and do not exceed the screen-safe area.

- [ ] **Step 1: Register Qt Test and write the failing test**

Update Qt discovery and testing setup:

```cmake
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Test)
include(CTest)
```

Create `tests/test_ui_sizing.cpp`:

```cpp
#include "ui/UiSizing.h"
#include <QTest>

class UiSizingTest : public QObject {
    Q_OBJECT
private slots:
    void keepsConfiguredDefaultOn1280x720() {
        QCOMPARE(calculatePanelSize({1280, 720}, {960, 600}), QSize(960, 600));
    }
    void clampsHeightOnShortScreen() {
        QCOMPARE(calculatePanelSize({1280, 654}, {960, 600}), QSize(960, 558));
    }
    void usesResponsiveTargetOnFullHd() {
        QCOMPARE(calculatePanelSize({1920, 1080}, {960, 600}), QSize(1280, 669));
    }
    void honorsLargeConfigWithinSafeArea() {
        QCOMPARE(calculatePanelSize({3840, 2160}, {2000, 1200}), QSize(2000, 1200));
    }
    void survivesTinyArea() {
        QCOMPARE(calculatePanelSize({320, 240}, {960, 600}), QSize(256, 144));
    }
    void sanitizesInvalidInputs() {
        QCOMPARE(calculatePanelSize({}, {-1, -1}), QSize(1, 1));
    }
};

QTEST_APPLESS_MAIN(UiSizingTest)
#include "test_ui_sizing.moc"
```

Register the test target:

```cmake
if(BUILD_TESTING)
    qt_add_executable(test_ui_sizing
        tests/test_ui_sizing.cpp
        src/ui/UiSizing.cpp
    )
    target_include_directories(test_ui_sizing PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(test_ui_sizing PRIVATE Qt6::Core Qt6::Test)
    add_test(NAME ui_sizing COMMAND test_ui_sizing)
endif()
```

- [ ] **Step 2: Verify the test is red**

Run:

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:\Qt\6.8.0\msvc2022_64"
cmake --build build --config Debug --target test_ui_sizing
```

Expected: compilation fails because `UiSizing.h/.cpp` do not yet define the function.

- [ ] **Step 3: Add the minimal implementation**

`src/ui/UiSizing.h`:

```cpp
#pragma once
#include <QSize>

QSize calculatePanelSize(const QSize &availableSize, const QSize &configuredSize);
```

`src/ui/UiSizing.cpp`:

```cpp
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
```

- [ ] **Step 4: Run the focused test**

```powershell
cmake --build build --config Debug --target test_ui_sizing
ctest --test-dir build -C Debug -R ui_sizing --output-on-failure
```

Expected: `ui_sizing` passes all six slots.

- [ ] **Step 5: Integrate the helper**

Add `src/ui/UiSizing.cpp` to `UI_SOURCES`, include `ui/UiSizing.h`, and replace `panelSize()` with:

```cpp
QSize MainWindow::panelSize() const {
    QScreen *screen = targetScreen();
    const QSize available = screen ? screen->availableGeometry().size() : QSize(1920, 1080);
    const QSize configured(
        qMax(1, static_cast<int>(m_config.panelWidth)),
        qMax(1, static_cast<int>(m_config.panelHeight)));
    return calculatePanelSize(available, configured);
}
```

- [ ] **Step 6: Build, test, and commit**

```powershell
cmake --build build --config Debug --target winSwitch test_ui_sizing
ctest --test-dir build -C Debug -R ui_sizing --output-on-failure
git add CMakeLists.txt src/ui/UiSizing.h src/ui/UiSizing.cpp src/ui/MainWindow.cpp tests/test_ui_sizing.cpp
git commit -m "test: add screen-safe panel sizing"
```

---

### Task 2: Windows Dark Native Title Bar

**Files:**
- Modify: `src/ui/MainWindow.cpp:1-65`

**Interfaces:**
- Internal Windows-only function: `bool applyWindowsDarkTitleBar(QWidget *)`.
- Failure returns `false`; the caller logs once and continues.

- [ ] **Step 1: Capture the light-title-bar baseline**

Run the current Debug executable, show the panel, and save a screenshot outside the repository or under ignored `build/` output. Confirm the title bar is light and native controls work.

- [ ] **Step 2: Add the Windows-only helper**

```cpp
#if defined(Q_OS_WIN)
#include <dwmapi.h>
#include <windows.h>
#endif

namespace {
#if defined(Q_OS_WIN)
bool applyWindowsDarkTitleBar(QWidget *window) {
    if (!window) {
        return false;
    }
    const HWND hwnd = reinterpret_cast<HWND>(window->winId());
    const BOOL enabled = TRUE;
    constexpr DWORD currentAttribute = 20;
    HRESULT result = DwmSetWindowAttribute(hwnd, currentAttribute, &enabled, sizeof(enabled));
    if (FAILED(result)) {
        constexpr DWORD legacyAttribute = 19;
        result = DwmSetWindowAttribute(hwnd, legacyAttribute, &enabled, sizeof(enabled));
    }
    return SUCCEEDED(result);
}
#endif
} // namespace
```

- [ ] **Step 3: Call it without changing startup behavior**

At the end of the constructor:

```cpp
#if defined(Q_OS_WIN)
    if (!applyWindowsDarkTitleBar(this)) {
        AppLog::warn(QStringLiteral("Windows dark title bar is unavailable"));
    }
#endif
```

- [ ] **Step 4: Build, manually verify, and commit**

```powershell
cmake --build build --config Debug --target winSwitch
git add src/ui/MainWindow.cpp
git commit -m "fix: match Windows title bar to dark theme"
```

Expected manual result: panel and settings keep native drag/minimize/restore/close behavior and use dark decoration when DWM accepts the request.

---

### Task 3: Preview-Unavailable Card State

**Files:**
- Create: `tests/test_switcher_panel.cpp`
- Modify: `CMakeLists.txt`
- Modify: `src/core/I18n.h/.cpp`
- Modify: `src/ui/WindowCard.h/.cpp`
- Modify: `src/ui/SwitcherPanel.cpp:233-247`

**Interfaces:**
- Add `bool thumbnailsEnabled` before `bool selected` in the `WindowCard` constructor.
- Add `QString I18n::previewUnavailable() const`.
- Stable child names: `ThumbnailImage`, `ThumbnailStatus`, `ThumbnailFallbackGlyph`.

- [ ] **Step 1: Write failing card tests**

Create `tests/test_switcher_panel.cpp`:

```cpp
#include "ui/WindowCard.h"
#include <QLabel>
#include <QTest>

namespace {
I18n zhI18n() {
    Config cfg;
    cfg.language = QStringLiteral("zh");
    return I18n::fromConfig(cfg);
}
WindowItem sampleItem() {
    return WindowItem{1, QStringLiteral("Sample window"), {}};
}
} // namespace

class SwitcherPanelTest : public QObject {
    Q_OBJECT
private slots:
    void reportsUnavailablePreviewWhenCaptureWasEnabled() {
        QPixmap icon(64, 64);
        icon.fill(Qt::green);
        WindowCard card(sampleItem(), icon, {}, true, false, zhI18n());
        auto *status = card.findChild<QLabel *>(QStringLiteral("ThumbnailStatus"));
        QVERIFY(status != nullptr);
        QCOMPARE(status->text(), QStringLiteral("窗口预览不可用"));
    }
    void doesNotReportFailureWhenThumbnailsWereDisabled() {
        QPixmap icon(64, 64);
        icon.fill(Qt::green);
        WindowCard card(sampleItem(), icon, {}, false, false, zhI18n());
        QVERIFY(card.findChild<QLabel *>(QStringLiteral("ThumbnailStatus")) == nullptr);
    }
    void usesGenericFallbackWhenNoTextureExists() {
        WindowCard card(sampleItem(), {}, {}, true, false, zhI18n());
        QVERIFY(card.findChild<QLabel *>(QStringLiteral("ThumbnailFallbackGlyph")) != nullptr);
    }
};

QTEST_MAIN(SwitcherPanelTest)
#include "test_switcher_panel.moc"
```

Register a test target with `WindowCard.cpp`, `I18n.cpp`, and `Config.cpp`, linked to Qt Core/Gui/Widgets/Test.

- [ ] **Step 2: Verify the missing API fails compilation**

```powershell
cmake --build build --config Debug --target test_switcher_panel
```

Expected: constructor and `previewUnavailable()` are absent.

- [ ] **Step 3: Add the bilingual text**

```cpp
QString I18n::previewUnavailable() const {
    return m_locale == Locale::Zh
        ? QStringLiteral("窗口预览不可用")
        : QStringLiteral("Preview unavailable");
}
```

- [ ] **Step 4: Implement the three preview states**

Update the constructor signature and replace the preview content branch with:

```cpp
auto *thumbLabel = new QLabel;
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
```

Pass `m_showThumbnails` from `SwitcherPanel` before `selected`.

- [ ] **Step 5: Test, build, and commit**

```powershell
cmake --build build --config Debug --target test_switcher_panel winSwitch
ctest --test-dir build -C Debug -R switcher_panel --output-on-failure
git add CMakeLists.txt tests/test_switcher_panel.cpp src/core/I18n.h src/core/I18n.cpp src/ui/WindowCard.h src/ui/WindowCard.cpp src/ui/SwitcherPanel.cpp
git commit -m "feat: clarify unavailable window previews"
```

---

### Task 4: Empty and No-Match States

**Files:**
- Modify: `tests/test_switcher_panel.cpp`
- Modify: `CMakeLists.txt`
- Modify: `src/core/I18n.h/.cpp`
- Modify: `src/ui/SwitcherPanel.cpp:182-278`
- Modify: `resources/styles/app.qss`

**Interfaces:**
- Add `noSwitchableWindows()`, `noMatchingWindows()`, `emptyWindowsHint()`, and `noMatchingWindowsHint()` to `I18n`.
- Stable child names: `EmptyState`, `EmptyStateTitle`, `EmptyStateHint`.

- [ ] **Step 1: Add panel dependencies and failing tests**

Add `SwitcherPanel.cpp`, `WindowModel.cpp`, and `AppLog.cpp` to `test_switcher_panel`. Add these slots:

```cpp
void showsEmptyStateWithoutWindows() {
    SwitcherPanel panel(zhI18n());
    panel.setData(AppState::create({}, Filter{}), {}, {}, true);
    auto *title = panel.findChild<QLabel *>(QStringLiteral("EmptyStateTitle"));
    QVERIFY(title != nullptr);
    QCOMPARE(title->text(), QStringLiteral("当前没有可切换窗口"));
}

void showsNoMatchStateForSearch() {
    RawWindow raw{7, QStringLiteral("Terminal"), QStringLiteral("C:/terminal.exe"),
                  QStringLiteral("Terminal"), {}};
    AppState state = AppState::create(buildGroups({raw}, {}, {}), Filter{});
    SwitcherPanel panel(zhI18n());
    panel.setData(state, {}, {}, true);
    auto *search = panel.findChild<QLineEdit *>(QStringLiteral("SearchEdit"));
    QVERIFY(search != nullptr);
    search->setText(QStringLiteral("not-found"));
    auto *title = panel.findChild<QLabel *>(QStringLiteral("EmptyStateTitle"));
    QVERIFY(title != nullptr);
    QCOMPARE(title->text(), QStringLiteral("没有匹配的窗口"));
}
```

Include `QLineEdit` in the test. Simulating text entry is required because `setData()` resets the visible search control when a panel session starts.

- [ ] **Step 2: Verify both tests fail**

```powershell
cmake --build build --config Debug --target test_switcher_panel
ctest --test-dir build -C Debug -R switcher_panel --output-on-failure
```

Expected: `EmptyStateTitle` is not found.

- [ ] **Step 3: Add exact bilingual methods**

```cpp
QString I18n::noSwitchableWindows() const {
    return m_locale == Locale::Zh ? QStringLiteral("当前没有可切换窗口")
                                  : QStringLiteral("No switchable windows");
}
QString I18n::noMatchingWindows() const {
    return m_locale == Locale::Zh ? QStringLiteral("没有匹配的窗口")
                                  : QStringLiteral("No matching windows");
}
QString I18n::emptyWindowsHint() const {
    return m_locale == Locale::Zh ? QStringLiteral("尝试打开一个窗口，或检查排除应用设置。")
                                  : QStringLiteral("Open a window or check the excluded-app settings.");
}
QString I18n::noMatchingWindowsHint() const {
    return m_locale == Locale::Zh ? QStringLiteral("请尝试其他关键词。")
                                  : QStringLiteral("Try a different search term.");
}
```

- [ ] **Step 4: Render one derived empty-state block**

After computing `groups` in `rebuildContent()`:

```cpp
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
```

- [ ] **Step 5: Add empty-state QSS**

```css
QWidget#EmptyState { background-color: #1a1b20; }
QLabel#EmptyStateGlyph { color: #667083; font-size: 34px; }
QLabel#EmptyStateTitle { color: #e1e5ec; font-size: 15px; font-weight: 600; }
QLabel#EmptyStateHint { color: #8b919e; font-size: 12px; }
```

- [ ] **Step 6: Test, build, and commit**

```powershell
cmake --build build --config Debug --target test_switcher_panel winSwitch
ctest --test-dir build -C Debug -R switcher_panel --output-on-failure
git add CMakeLists.txt tests/test_switcher_panel.cpp src/core/I18n.h src/core/I18n.cpp src/ui/SwitcherPanel.cpp resources/styles/app.qss
git commit -m "feat: show switcher empty states"
```

---

### Task 5: Filters, Actions, Cards, and Typography Polish

**Files:**
- Modify: `tests/test_switcher_panel.cpp`
- Modify: `src/ui/SwitcherPanel.cpp:15-67,120-180,279-340`
- Modify: `resources/styles/app.qss:1-274`

**Interfaces:**
- Filter viewport converts vertical wheel input to horizontal movement only when overflow exists.
- Group close still emits `PanelActionType::CloseGroup` immediately.

- [ ] **Step 1: Write the failing wheel test**

```cpp
void wheelScrollsLongFilterRowHorizontally() {
    QVector<AppGroup> groups;
    for (int i = 0; i < 12; ++i) {
        AppGroup group;
        group.exePath = QStringLiteral("C:/apps/application-%1.exe").arg(i);
        group.appName = QStringLiteral("application-%1").arg(i);
        group.windows.append(WindowItem{i + 1, QStringLiteral("window"), {}});
        groups.append(group);
    }
    SwitcherPanel panel(zhI18n());
    panel.resize(420, 500);
    panel.setData(AppState::create(groups, Filter{}), {}, {}, false);
    panel.show();
    QTest::qWait(1);
    auto *scroll = panel.findChild<QScrollArea *>(QStringLiteral("FilterScroll"));
    QVERIFY(scroll != nullptr);
    QVERIFY(scroll->horizontalScrollBar()->maximum() > 0);
    const int before = scroll->horizontalScrollBar()->value();
    QWheelEvent wheel(QPointF(20, 20), QPointF(20, 20), {}, QPoint(0, -120),
                      Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
    QApplication::sendEvent(scroll->viewport(), &wheel);
    QVERIFY(scroll->horizontalScrollBar()->value() > before);
}
```

- [ ] **Step 2: Verify the test fails before routing**

```powershell
cmake --build build --config Debug --target test_switcher_panel
ctest --test-dir build -C Debug -R switcher_panel --output-on-failure
```

- [ ] **Step 3: Route wheel events horizontally**

Install the event filter on `m_filterScroll->viewport()`. Add before the search branch in `eventFilter()`:

```cpp
if (obj == m_filterScroll->viewport() && event->type() == QEvent::Wheel) {
    auto *wheel = static_cast<QWheelEvent *>(event);
    QScrollBar *bar = m_filterScroll->horizontalScrollBar();
    if (bar && bar->maximum() > bar->minimum()) {
        const int delta = !wheel->pixelDelta().isNull()
            ? -wheel->pixelDelta().y()
            : -(wheel->angleDelta().y() / 120) * 48;
        bar->setValue(bar->value() + delta);
        wheel->accept();
        return true;
    }
}
```

- [ ] **Step 4: Apply the agreed QSS and object names**

Add global typography and state styles:

```css
QWidget { font-family: "Microsoft YaHei UI", "Segoe UI", sans-serif; }
QScrollArea#FilterScroll { min-height: 48px; max-height: 48px; }
QWidget#ThumbnailFallback { background-color: #181c22; }
QLabel#ThumbnailFallbackGlyph { color: #8d96a6; font-size: 28px; }
QLabel#ThumbnailStatus { color: #7f8999; font-size: 11px; }
QToolButton#GroupCloseAction:hover {
    background-color: #4a2528;
    border-color: #a74349;
    color: #ffe9ea;
}
QWidget#WindowCard[selected="true"] {
    border: 1px solid #4a90d9;
    background-color: #2a3545;
}
```

Set only the group-header “关闭全部” object name to `GroupCloseAction`. Set content spacing to 12. Keep chip/card close behavior, card width 226, and preview size 210×118.

- [ ] **Step 5: Test direct close behavior manually and run automation**

```powershell
cmake --build build --config Debug --target test_switcher_panel winSwitch
ctest --test-dir build -C Debug --output-on-failure
```

Manual expectations: overflowing filters wheel-scroll; chip `×` and “关闭全部” still close directly without a dialog; selected cards do not shift.

- [ ] **Step 6: Commit Task 5**

```powershell
git add tests/test_switcher_panel.cpp src/ui/SwitcherPanel.cpp resources/styles/app.qss
git commit -m "style: polish switcher hierarchy and filters"
```

---

### Task 6: Full Verification and Maintenance Documentation

**Files:**
- Modify: `docs/project-maintenance-guide.md`
- Modify: `docs/README.md` only if its current-baseline wording needs adjustment.

**Interfaces:**
- Produces passing Debug/Release builds, passing CTest targets, and a current maintenance baseline.

- [ ] **Step 1: Run the full Debug suite**

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:\Qt\6.8.0\msvc2022_64"
cmake --build build --config Debug --parallel
ctest --test-dir build -C Debug --output-on-failure
```

Expected: `ui_sizing` and `switcher_panel` pass with zero failed tests.

- [ ] **Step 2: Build Release**

```powershell
cmake --build build --config Release --parallel
```

Expected: `build/Release/winSwitch.exe` exists.

- [ ] **Step 3: Run the Windows 11 visual matrix**

Verify 1280×720 and 1920×1080; 100%, 125%, and 150% scaling where available; dark title bar; four preview states; empty/no-match/normal states; long filters; arrows/Enter/Esc; pin; card close; chip close; direct group close; and 30 rapid show/hide cycles while textures load.

- [ ] **Step 4: Update the maintenance baseline**

In `docs/project-maintenance-guide.md`:

- Mark dark title bar, safe sizing, preview placeholder, empty states, and wheel scrolling implemented.
- Record both new CTest targets and the latest pass count.
- Remove group-close confirmation from the backlog; retain close-result synchronization and failure feedback.
- Record the verification date and display matrix.

- [ ] **Step 5: Check scope and whitespace**

```powershell
git diff --check
git status --short
git diff --stat
```

Expected: no whitespace errors; `cursor-recovered-chats/` remains unstaged.

- [ ] **Step 6: Commit documentation and reverify**

```powershell
git add docs/project-maintenance-guide.md docs/README.md
git commit -m "docs: update Windows UI maintenance baseline"
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
git status --short
```

Expected: Release succeeds, every registered test passes, and only intentionally untracked user files remain.

---

### Task 7: Deploy Qt Runtime Beside Windows Build Outputs

**Execution order:** Run this task immediately after Task 2, before Task 3.

**Files:**
- Create: `tests/verify_windows_deployment.ps1`
- Modify: `CMakeLists.txt`

**Interfaces:**
- Consumes: the `windeployqt` executable belonging to the discovered Qt 6 installation.
- Produces: Debug and Release output directories containing the matching Qt Core/Gui/Widgets DLLs and `platforms/qwindows*.dll`.
- Existing CI packaging remains valid; its later `windeployqt` invocation may safely refresh the copied package.

- [ ] **Step 1: Write the failing deployment verification script**

Create `tests/verify_windows_deployment.ps1`:

```powershell
param(
    [Parameter(Mandatory = $true)]
    [string]$ExePath,
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration
)

$resolvedExe = (Resolve-Path -LiteralPath $ExePath).Path
$outputDir = Split-Path -Parent $resolvedExe
$suffix = if ($Configuration -eq 'Debug') { 'd' } else { '' }
$required = @(
    "Qt6Core$suffix.dll",
    "Qt6Gui$suffix.dll",
    "Qt6Widgets$suffix.dll",
    "platforms/qwindows$suffix.dll"
)
$missing = @($required | Where-Object {
    -not (Test-Path -LiteralPath (Join-Path $outputDir $_))
})
if ($missing.Count -gt 0) {
    throw "Missing deployed Qt runtime files: $($missing -join ', ')"
}
Write-Output "Qt runtime deployment verified for $Configuration at $outputDir"
```

- [ ] **Step 2: Run the script against the current Debug output and verify RED**

```powershell
pwsh -NoProfile -File tests/verify_windows_deployment.ps1 `
  -ExePath build/Debug/winSwitch.exe -Configuration Debug
```

Expected: FAIL listing `Qt6Cored.dll`, `Qt6Guid.dll`, `Qt6Widgetsd.dll`, and `platforms/qwindowsd.dll` as missing.

- [ ] **Step 3: Add one Windows post-build deployment command**

After `qt_add_executable(winSwitch ...)`, add:

```cmake
if(WIN32)
    get_target_property(QT_QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(QT_BIN_DIR "${QT_QMAKE_EXECUTABLE}" DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE
        NAMES windeployqt
        HINTS "${QT_BIN_DIR}"
        REQUIRED
    )
    add_custom_command(TARGET winSwitch POST_BUILD
        COMMAND "${WINDEPLOYQT_EXECUTABLE}"
            --$<IF:$<CONFIG:Debug>,debug,release>
            --no-translations
            --no-opengl-sw
            --no-system-d3d-compiler
            "$<TARGET_FILE:winSwitch>"
        COMMENT "Deploying Qt runtime dependencies beside winSwitch"
        VERBATIM
    )
endif()
```

Do not copy DLLs with hard-coded Qt paths. Do not deploy Qt beside test executables; their CTest runtime PATH remains the test mechanism.

- [ ] **Step 4: Force a Debug relink and verify GREEN**

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:\Qt\6.8.0\msvc2022_64"
cmake --build build --config Debug --target winSwitch --clean-first
pwsh -NoProfile -File tests/verify_windows_deployment.ps1 `
  -ExePath build/Debug/winSwitch.exe -Configuration Debug
```

Expected: build output contains `Deploying Qt runtime dependencies beside winSwitch`; the script passes.

- [ ] **Step 5: Build and verify Release deployment**

```powershell
cmake --build build --config Release --target winSwitch --clean-first
pwsh -NoProfile -File tests/verify_windows_deployment.ps1 `
  -ExePath build/Release/winSwitch.exe -Configuration Release
```

Expected: release DLLs and `platforms/qwindows.dll` exist and the script passes.

- [ ] **Step 6: Verify direct launch without a Qt bin PATH**

Temporarily stop any existing winSwitch instance, launch `build/Release/winSwitch.exe` from a shell whose PATH does not contain a Qt directory, wait two seconds, and verify the process remains running. Stop the test process and restore the user's original instance afterward.

- [ ] **Step 7: Commit Task 7**

```powershell
git add CMakeLists.txt tests/verify_windows_deployment.ps1
git commit -m "build: deploy Qt runtime beside Windows executable"
```

---

## Completion Criteria

- Native title bar matches the dark surface or safely falls back.
- Panel size stays inside tested screen-safe bounds.
- Preview-unavailable and thumbnails-disabled states are distinct.
- Empty/no-match states show correct Chinese and English copy.
- Overflowing filters support wheel-based horizontal scrolling.
- Search, selection, pin, activation, card close, chip close, and group close do not regress.
- Group close shows no confirmation dialog.
- Debug and Release builds succeed; all registered Qt tests pass.
- Debug and Release winSwitch output directories pass `verify_windows_deployment.ps1` and launch without a Qt bin PATH.
- Maintenance documentation reflects the implemented state.
