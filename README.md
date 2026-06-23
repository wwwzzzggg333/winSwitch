# mySwitcher

[![Build](https://github.com/OWNER/mySwitcher/actions/workflows/build.yml/badge.svg)](https://github.com/OWNER/mySwitcher/actions/workflows/build.yml)

跨平台窗口切换管理器（C++ / Qt 6），功能对标 [WindowsSwitchEx](../WindowsSwitchEx/README.md)：

- 全局快捷键唤出/隐藏切换面板
- 按应用程序分组展示窗口
- 窗口缩略图与图标
- 过滤、置顶、关闭单个/整组窗口
- 键盘与鼠标导航
- 系统托盘常驻与配置界面
- 单实例运行

## 支持平台

| 平台 | 状态 | 说明 |
|------|------|------|
| Windows 10/11 | 完整支持 | Win32 枚举、PrintWindow 缩略图、RegisterHotKey |
| macOS 11+ | 基础支持 | CGWindowList 枚举与缩略图；关闭窗口需后续增强 |
| Linux (X11) | 基础支持 | EWMH `_NET_CLIENT_LIST`；Wayland 下全局热键与窗口枚举受限 |

## 构建

**依赖**：Qt 6.2+（Widgets）、CMake 3.16+；Linux 还需 `libx11-dev`。

### 本地构建

```bash
cmake -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x/gcc_64
cmake --build build --config Release
```

Windows 示例：

```powershell
cmake -B build -DCMAKE_PREFIX_PATH="C:\Qt\6.7.0\msvc2019_64"
cmake --build build --config Release
```

### GitHub Actions 自动构建

推送到 `main` / `master` 分支后，会在 **Windows / macOS / Linux** 三端自动编译。构建产物可在 Actions 页面的 **Artifacts** 中下载：

- `mySwitcher-windows`
- `mySwitcher-macos`
- `mySwitcher-linux`

> 将 README 顶部的 `OWNER/mySwitcher` 替换为你的 GitHub 用户名/组织名后，CI 徽章才会正确显示。

## 配置

配置文件：`config.json`（与 exe 同目录，或 `%APPDATA%/mySwitcher/` / `~/.config/mySwitcher/`）。

字段：`hotkey`、`thumbnail`、`panel_width`、`panel_height`、`language`（auto/zh/en）、`pinned`、`excluded`。

修改快捷键或语言后需重启。

## 架构

```
src/
  core/       配置、分组模型、i18n
  platform/   IWindowSource / 图标 / 缩略图（Win / macOS / Linux）
  app/        托盘、热键、单实例、应用编排
  ui/         主窗口、切换面板、设置页
```

## 与 Rust 版差异

- 配置格式为 JSON（非 TOML）
- macOS / Linux 的「关闭窗口」「Finder/Explorer 路径排序」等功能仍在完善中
- 分发需携带 Qt 运行库（`windeployqt` / `macdeployqt` / 系统包）

## 已知限制

- **Wayland**：多数桌面环境下无法注册全局热键，窗口列表也可能不完整
- **macOS 关闭窗口**：当前仅激活应用，关闭操作待 Accessibility API 实现
- **Linux 缩略图**：XGetImage 对部分 compositor 可能失败
