# Docs Execution Package Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 产出两份面向开发执行的正式文档，分别定义 `v0.2` 的执行型产品规格和工程任务拆解，供后续直接进入开发排期与实施。

**Architecture:** 以现有分析文档和已确认的设计规格为输入，先写正式规格文档，再写 `v0.2` 任务拆解文档，最后统一校验两份文档与既有路线图、优先级分析之间的口径一致性。整个过程只修改 `docs` 目录，不触碰产品代码。

**Tech Stack:** Markdown、现有 `docs` 文档体系、Trae diagnostics

---

## File Structure

### New Files

- `docs/product-execution-spec.md`
  - 责任：定义开发执行视角下的正式产品方案，明确背景、目标、范围、非目标、平台约束、核心功能方案、验收标准与风险策略。
- `docs/v0.2-implementation-breakdown.md`
  - 责任：把 `v0.2` 范围拆到模块和任务层级，明确影响文件、依赖关系、建议顺序和完成标准。

### Reference Files

- `docs/superpowers/specs/2026-06-23-docs-execution-package-design.md`
  - 责任：作为本计划的上游规格，限定两份文档的结构、范围和 `v0.2` 主题。
- `docs/project-feature-platform-analysis.md`
  - 责任：提供项目现状、平台限制和能力分级基础。
- `docs/project-feature-priority-matrix.md`
  - 责任：提供功能优先级、收益、风险依据。
- `docs/project-roadmap.md`
  - 责任：提供 `v0.2 / v0.3 / v1.0` 阶段边界。
- `docs/README.md`
  - 责任：完成后作为入口校验新文档是否需要被索引。

## Task 1: 写正式产品方案文档

**Files:**
- Create: `docs/product-execution-spec.md`
- Reference: `docs/superpowers/specs/2026-06-23-docs-execution-package-design.md`
- Reference: `docs/project-feature-platform-analysis.md`
- Reference: `docs/project-feature-priority-matrix.md`
- Reference: `docs/project-roadmap.md`

- [ ] **Step 1: 起草文档骨架**

把下面内容写入 `docs/product-execution-spec.md`，先建立完整结构：

```md
# winSwitch 执行型产品方案

## 1. 文档目的

## 2. 背景与现状

## 3. 问题定义

## 4. 目标与成功标准

## 5. 目标用户与使用场景

## 6. 范围定义

## 7. 非目标

## 8. 平台约束与能力分级

## 9. 核心功能方案

## 10. v0.2 范围说明

## 11. 验收标准

## 12. 风险与降级策略
```

- [ ] **Step 2: 填充“背景、问题、目标”部分**

把下面内容补入 `docs/product-execution-spec.md` 对应章节：

```md
## 2. 背景与现状

`winSwitch` 当前已具备托盘常驻、单实例、窗口分组、基础切换、关闭窗口、缩略图展示、设置持久化等能力。

从现状看：

- Windows 已接近完整可用
- macOS 属于基础支持
- Linux X11 属于基础支持但主入口不完整
- Linux Wayland 受平台机制限制，很多能力无法作为普通应用直接实现

## 3. 问题定义

当前项目的主要问题不是“完全没有功能”，而是“主流程效率和跨平台闭环还不够完整”。

具体表现为：

- 没有关键字搜索
- 没有最近使用排序
- 多屏幕体验不足
- 热键配置与提示能力不足
- 用户无法快速判断当前平台支持哪些能力
- 配置迁移能力不足

## 4. 目标与成功标准

本阶段目标是：在不扩大高风险平台范围的前提下，把 `v0.2` 做成一个明显更好用、可直接进入开发执行的增强版本定义。

成功标准：

- 明确 `v0.2` 的范围与非目标
- 明确六个核心主题的行为变化
- 明确平台差异和降级策略
- 让开发可以据此开始模块拆解和实施
```

- [ ] **Step 3: 填充“用户、范围、非目标”部分**

把下面内容补入对应章节：

```md
## 5. 目标用户与使用场景

目标用户：

- 高频桌面多任务用户
- 多窗口办公用户
- 多屏用户
- 对跨平台桌面工具有连续使用需求的用户

典型场景：

- 通过热键打开面板并快速切换窗口
- 在窗口很多时通过搜索快速定位目标
- 在多屏场景中就近打开切换面板
- 在平台受限时快速知道哪些能力不可用

## 6. 范围定义

`v0.2` 纳入范围：

1. 搜索能力
2. 最近使用排序（MRU）
3. 多屏幕感知
4. 热键增强
5. 平台能力诊断页
6. 配置导入导出

## 7. 非目标

以下内容不属于 `v0.2` 交付承诺：

- macOS 全局热键实现
- macOS 关闭窗口实现
- Linux X11 全局热键实现
- Wayland 深度适配
- 大规模视觉重做
- 复杂窗口管理能力
```

- [ ] **Step 4: 填充“平台约束与核心功能方案”部分**

把下面内容补入对应章节：

```md
## 8. 平台约束与能力分级

能力分为三层：

- 共性能力：搜索、排序、多屏策略、配置导入导出、诊断页
- 平台增强能力：热键底层实现、窗口关闭、图标与缩略图能力
- 平台受限能力：Wayland 下的全局窗口枚举、全局热键、跨应用窗口截图

平台策略：

- Windows：维持完整体验并优先打磨
- macOS：先明确权限与缺口，不在 `v0.2` 承诺高风险能力
- Linux X11：保留基础支持，不在 `v0.2` 承诺补齐热键底层
- Wayland：采用检测、提示、禁用的降级策略

## 9. 核心功能方案

### 搜索能力

- 在面板顶部增加搜索输入框
- 支持按窗口标题、应用名、路径匹配
- 搜索结果实时更新
- 搜索状态下保留现有分组信息

### 最近使用排序

- 记录窗口最近激活时间
- 提供 MRU 排序能力
- 与置顶规则协同，默认保持置顶优先、组内窗口按最近使用排序

### 多屏幕感知

- 默认按鼠标所在屏幕显示面板
- 若获取失败，回退到主屏幕
- 为后续跟随前台窗口所在屏幕预留扩展点

### 热键增强

- 扩大热键录入支持范围
- 改善错误提示
- 对平台不支持的情形给出明确说明

### 平台能力诊断页

- 展示平台、会话类型、热键状态、缩略图能力、图标能力、日志路径

### 配置导入导出

- 支持导出当前配置
- 支持导入配置并做基础校验
```

- [ ] **Step 5: 填充“v0.2 范围、验收、风险”部分**

把下面内容补入对应章节：

```md
## 10. v0.2 范围说明

`v0.2` 聚焦低风险高收益项，目标是强化主流程体验，不在本版本扩张高风险平台能力。

## 11. 验收标准

- 用户可在面板中输入关键字搜索窗口
- 窗口列表能体现最近使用顺序
- 面板可优先出现在鼠标所在屏幕
- 热键配置失败时能给出明确提示
- 用户可以在界面中看到平台能力状态
- 用户可以导入和导出配置文件

## 12. 风险与降级策略

- 搜索、分组、MRU 的协同必须有明确优先级规则
- 平台受限能力必须通过 UI 明示，不以“静默失败”方式处理
- Wayland 继续采用功能禁用和限制说明策略
```

- [ ] **Step 6: 检查文档是否覆盖规格要求**

逐项核对 `docs/superpowers/specs/2026-06-23-docs-execution-package-design.md` 中以下要求是否在文档中出现：

```txt
背景与现状
问题定义
目标与成功标准
目标用户与使用场景
范围定义
非目标
平台约束与能力分级
核心功能方案
v0.2 范围说明
验收标准
风险与降级策略
```

预期：`docs/product-execution-spec.md` 中 11 个章节全部存在，且没有缺项。

## Task 2: 写 v0.2 开发任务拆解文档

**Files:**
- Create: `docs/v0.2-implementation-breakdown.md`
- Reference: `docs/superpowers/specs/2026-06-23-docs-execution-package-design.md`
- Reference: `docs/product-execution-spec.md`
- Reference: `docs/project-roadmap.md`

- [ ] **Step 1: 起草文档骨架**

把下面内容写入 `docs/v0.2-implementation-breakdown.md`：

```md
# winSwitch v0.2 开发任务拆解

## 1. 版本目标

## 2. 范围边界

## 3. 模块拆分

## 4. 任务分组

## 5. 任务清单

## 6. 影响文件

## 7. 依赖关系

## 8. 建议开发顺序

## 9. 测试与验收

## 10. 风险与阻塞项
```

- [ ] **Step 2: 填充“版本目标、范围边界、模块拆分”**

把下面内容补入对应章节：

```md
## 1. 版本目标

`v0.2` 的目标是提升主流程效率和可解释性，不扩张高风险平台能力。

## 2. 范围边界

纳入：

- 搜索能力
- 最近使用排序
- 多屏幕感知
- 热键增强
- 平台能力诊断页
- 配置导入导出

不纳入：

- macOS 热键底层实现
- macOS 关闭窗口实现
- Linux X11 热键底层实现
- Wayland 深度适配

## 3. 模块拆分

- `core`：搜索状态、排序状态、配置结构扩展
- `ui`：搜索输入、诊断页入口、设置页增强、多屏显示策略相关展示
- `app`：窗口激活后更新 MRU、热键失败提示、配置导入导出编排
- `platform`：能力探测信息暴露，供诊断页使用
```

- [ ] **Step 3: 填充“任务分组”**

把下面内容补入 `## 4. 任务分组`：

```md
## 4. 任务分组

### A. 搜索能力

- 增加搜索输入状态
- 增加过滤逻辑
- 增加搜索结果展示和高亮

### B. MRU 排序

- 增加最近激活时间记录
- 增加排序逻辑
- 明确与置顶规则的优先级

### C. 多屏幕感知

- 增加目标屏幕选择逻辑
- 更新主窗口显示位置策略

### D. 热键增强

- 扩充录入能力
- 对齐解析能力
- 增强提示语

### E. 平台能力诊断页

- 定义诊断信息结构
- 增加设置页入口或独立视图
- 展示平台能力状态

### F. 配置导入导出

- 增加文件选择流程
- 增加 JSON 校验
- 增加导入后回写与提示
```

- [ ] **Step 4: 填充“任务清单”**

把下面内容补入 `## 5. 任务清单`：

```md
## 5. 任务清单

### 任务 1：搜索能力

- 目标：支持在面板中按标题、应用名、路径实时搜索
- 改动点：`core/WindowModel`、`ui/SwitcherPanel`、`ui/MainWindow`
- 完成标准：输入关键字后，列表实时收敛到匹配结果

### 任务 2：MRU 排序

- 目标：记录窗口最近使用时间并参与排序
- 改动点：`app/Application`、`core/WindowModel`
- 完成标准：激活窗口后再次打开面板，顺序体现最近使用结果

### 任务 3：多屏幕感知

- 目标：按鼠标所在屏幕显示面板
- 改动点：`ui/MainWindow`
- 完成标准：多屏下打开面板时优先出现在鼠标所在屏幕

### 任务 4：热键增强

- 目标：扩展录入支持并改善错误提示
- 改动点：`ui/HotkeyEdit`、`app/HotkeyManager_*`、`core/I18n`
- 完成标准：用户能录入更多热键，失败时看到明确原因

### 任务 5：平台能力诊断页

- 目标：把当前平台能力可视化
- 改动点：`platform/*`、`ui/SettingsDialog` 或新增诊断视图
- 完成标准：用户可查看平台、会话类型、热键、缩略图、图标能力状态

### 任务 6：配置导入导出

- 目标：支持配置备份与恢复
- 改动点：`core/Config`、`ui/SettingsDialog`、`app/Application`
- 完成标准：用户可导出配置，也可导入合法 JSON 配置
```

- [ ] **Step 5: 填充“影响文件、依赖关系、开发顺序”**

把下面内容补入对应章节：

```md
## 6. 影响文件

重点影响文件预计包括：

- `src/core/Config.h`
- `src/core/Config.cpp`
- `src/core/WindowModel.h`
- `src/core/WindowModel.cpp`
- `src/core/I18n.h`
- `src/core/I18n.cpp`
- `src/app/Application.h`
- `src/app/Application.cpp`
- `src/ui/MainWindow.h`
- `src/ui/MainWindow.cpp`
- `src/ui/SwitcherPanel.h`
- `src/ui/SwitcherPanel.cpp`
- `src/ui/SettingsDialog.h`
- `src/ui/SettingsDialog.cpp`
- `src/ui/HotkeyEdit.h`
- `src/ui/HotkeyEdit.cpp`
- `src/platform/IWindowSource.h`
- `src/platform/windows/*`
- `src/platform/macos/*`
- `src/platform/linux/*`

## 7. 依赖关系

- 搜索能力依赖 `WindowModel` 的过滤结构扩展
- MRU 排序依赖窗口激活后的状态写回
- 多屏幕感知主要依赖 `MainWindow` 显示逻辑
- 诊断页依赖平台能力探测接口先定义完成
- 配置导入导出依赖 `Config` 结构稳定

## 8. 建议开发顺序

1. 先扩展 `core/Config` 和 `core/WindowModel`
2. 再实现搜索和 MRU
3. 再处理多屏显示逻辑
4. 再补热键增强
5. 再补平台能力诊断页
6. 最后做配置导入导出与交互收尾
```

- [ ] **Step 6: 填充“测试与验收、风险与阻塞项”**

把下面内容补入对应章节：

```md
## 9. 测试与验收

建议按以下方式验证：

- 搜索：验证标题、应用名、路径匹配是否生效
- MRU：验证窗口激活后排序是否更新
- 多屏：验证面板是否显示在预期屏幕
- 热键增强：验证录入结果、失败提示、平台提示
- 诊断页：验证平台与能力状态显示是否正确
- 配置导入导出：验证合法与非法 JSON 的处理路径

## 10. 风险与阻塞项

- 搜索与现有分组模型结合后，展示规则可能需要反复调整
- MRU 与置顶规则冲突时，需要尽早定口径
- 诊断页的数据接口若设计过重，会拖慢 UI 层推进
- 热键增强容易因平台差异产生前后端不一致
```

- [ ] **Step 7: 检查拆解文档是否可直接执行**

核对以下几点：

```txt
是否清楚列出 6 个 v0.2 主题
是否明确不纳入范围的高风险平台项
是否明确每个任务的目标、改动点和完成标准
是否列出主要影响文件和依赖关系
是否给出开发顺序和测试建议
```

预期：研发拿到文档后，不需要再回头找分析文档才能理解 `v0.2` 的工作分解。

## Task 3: 一致性校验和索引检查

**Files:**
- Modify: `docs/README.md`（仅在缺少新文档链接时修改）
- Reference: `docs/product-execution-spec.md`
- Reference: `docs/v0.2-implementation-breakdown.md`
- Reference: `docs/project-roadmap.md`

- [ ] **Step 1: 检查 docs 索引是否需要新增条目**

对照下面目标列表检查 `docs/README.md` 是否已经包含新文档：

```txt
docs/product-execution-spec.md
docs/v0.2-implementation-breakdown.md
```

如果缺少任何一个，则在“文档列表”中新增条目。

- [ ] **Step 2: 运行文档自检**

使用下列检查项逐条验证：

```txt
两份新文档是否存在
是否都使用了明确标题
是否与设计规格中的范围一致
是否没有把 macOS/Linux X11 热键底层实现误写进 v0.2 正文
是否没有 TODO/TBD/待补充 等占位文字
```

预期：两份新文档能独立阅读，且与既有路线图不冲突。

- [ ] **Step 3: 运行诊断检查**

对以下文件运行 diagnostics：

```txt
docs/product-execution-spec.md
docs/v0.2-implementation-breakdown.md
docs/README.md
```

预期：无诊断错误。
