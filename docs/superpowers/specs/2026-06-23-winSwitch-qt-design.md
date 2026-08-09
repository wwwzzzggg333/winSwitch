# winSwitch Qt 跨平台设计

## 目标

用 C++ / Qt 6 重写 WindowsSwitchEx，并在 Windows、macOS、Linux 上提供一致的窗口切换体验。

## 架构

- **core**：配置（JSON）、分组模型、i18n
- **platform**：`IWindowSource` / `IIconCapture` / `IThumbnailCapture` 三接口，按 OS 分实现
- **app**：单实例、全局热键、托盘、`ApplicationController` 编排
- **ui**：`MainWindow` + `SwitcherPanel` + `SettingsDialog`

## 平台策略

| 能力 | Windows | macOS | Linux X11 | Linux Wayland |
|------|---------|-------|-----------|---------------|
| 窗口列表 | EnumWindows | CGWindowListCopyWindowInfo | _NET_CLIENT_LIST | 受限 |
| 激活 | SetForegroundWindow | NSRunningApplication | _NET_ACTIVE_WINDOW | 受限 |
| 关闭 | WM_CLOSE | 待实现 (AX) | WM_DELETE_WINDOW | 受限 |
| 缩略图 | PrintWindow | CGWindowListCreateImage | XGetImage | 受限 |
| 全局热键 | RegisterHotKey | Carbon HotKey (待完善) | XGrabKey (待完善) | 不可用 |

## 配置

`config.json`，字段与 Rust 版语义对齐；默认热键 `Alt+``。

## 分期

1. **M1（当前）**：Windows 完整 + UI/模型/托盘；macOS/Linux 基础枚举与缩略图
2. **M2**：macOS 热键与关闭；Linux X11 热键
3. **M3**：Wayland portal / 文档化限制
