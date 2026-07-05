# Thumbnail Capture Technical Analysis Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 产出一份独立的 Windows 缩略图抓取问题技术分析文档，解释当前实现为什么会只截到标题栏、截不全或完全截不到，并为后续优化提供技术依据。

**Architecture:** 以当前 Windows 缩略图实现代码和已确认的规格文档为输入，先建立问题现象与当前实现链路，再拆分“实现策略问题”和“平台/API 限制”，最后总结风险点与可选改进方向。整个过程只新增文档，不修改业务代码。

**Tech Stack:** Markdown、Windows 缩略图实现代码、现有 specs 文档、Trae diagnostics

---

## File Structure

### New Files

- `docs/thumbnail-capture-technical-analysis.md`
  - 责任：独立说明 Windows 缩略图抓取问题的现象、成因、风险点和改进方向。

### Reference Files

- `docs/superpowers/specs/2026-06-23-thumbnail-capture-technical-analysis-design.md`
  - 责任：定义分析文档的结构、深度、非目标与验收要求。
- `src/platform/windows/WinWindowSource.cpp`
  - 责任：当前 Windows 缩略图抓取的核心实现依据。
- `docs/project-feature-platform-analysis.md`
  - 责任：提供跨平台限制的已有背景说明，避免和已有文档冲突。
- `docs/README.md`
  - 责任：分析文档完成后用于索引接入。

## Task 1: 写技术分析文档正文

**Files:**
- Create: `docs/thumbnail-capture-technical-analysis.md`
- Reference: `docs/superpowers/specs/2026-06-23-thumbnail-capture-technical-analysis-design.md`
- Reference: `src/platform/windows/WinWindowSource.cpp`

- [ ] **Step 1: 建立文档骨架**

把下面内容写入 `docs/thumbnail-capture-technical-analysis.md`：

```md
# mySwitcher 缩略图抓取技术分析

## 1. 背景与问题现象

## 2. 当前实现路径

## 3. 为什么会只截到标题栏或截不全

## 4. 为什么有些窗口完全截不到

## 5. 当前实现的具体风险点

## 6. 可选改进方向

## 7. 不应误判为普通 bug 的场景

## 8. 结论
```

- [ ] **Step 2: 写“背景与问题现象”**

把下面内容补入 `## 1. 背景与问题现象`：

```md
## 1. 背景与问题现象

`mySwitcher` 当前在 Windows 下通过 `PrintWindow` 抓取窗口缩略图。

从使用现象看，已经出现以下典型问题：

- 有些窗口只能截到标题栏
- 有些窗口只能截到顶部一部分
- 有些窗口缩略图是纯黑或纯白
- 有些窗口缩略图为空
- 同一个窗口在不同状态下可能表现不一致

这些现象并不都来自同一个原因。它们通常由两类因素叠加造成：

- 当前实现策略本身存在风险
- Windows API 以及具体应用窗口类型存在天然限制
```

- [ ] **Step 3: 写“当前实现路径”**

把下面内容补入 `## 2. 当前实现路径`：

```md
## 2. 当前实现路径

当前 Windows 缩略图抓取逻辑位于 `src/platform/windows/WinWindowSource.cpp`，主要流程如下：

1. 使用 `GetClientRect()` 获取窗口客户区大小
2. 以该尺寸创建兼容位图
3. 调用 `PrintWindow(hwnd, memDc, PW_RENDERFULLCONTENT)` 请求窗口绘制
4. 使用 `GetDIBits()` 读回像素
5. 将 BGRA 转为 RGBA
6. 对纯黑或纯白结果做空白图过滤
7. 最后按固定上限缩放输出

从实现语义上看，当前逻辑默认假设：

- `GetClientRect()` 得到的尺寸适合作为抓图目标尺寸
- `PrintWindow()` 会把需要展示的窗口内容正确绘制到这块位图里

问题恰恰出在这两个假设并不总是成立。
```

- [ ] **Step 4: 写“只截到标题栏或截不全”的分析**

把下面内容补入 `## 3. 为什么会只截到标题栏或截不全`：

```md
## 3. 为什么会只截到标题栏或截不全

这是当前实现中最值得优先解释的问题。

### 3.1 客户区尺寸与实际绘制范围不一致

`GetClientRect()` 获取的是客户区大小，不包含标题栏和边框。

但 `PrintWindow()` 在很多窗口上并不严格等于“只绘制客户区内容”。在不同窗口实现下，它可能会：

- 绘制整个窗口
- 先绘制非客户区
- 只绘制一部分客户区
- 返回成功但实际内容不完整

如果承接位图只按客户区尺寸创建，而实际绘制范围更接近整窗，就会发生裁切。

### 3.2 为什么会出现“只剩顶部一截”

当绘制内容超出目标位图时，最容易留下的往往是顶部区域。

在可视效果上，这通常表现为：

- 只看到标题栏
- 只看到标题栏加顶部少量内容
- 整个图像像是从上方被截下来的一条

这类现象非常符合“承接位图尺寸偏小，但绘制语义偏整窗”的结果。

### 3.3 这属于什么性质的问题

这类问题首先属于当前实现策略存在明显风险：

- 使用客户区尺寸去承接 `PrintWindow()` 的输出，本身就不稳定

同时它也叠加了 API 行为不一致的问题：

- 不同应用对 `PrintWindow()` 的响应并不统一
```

- [ ] **Step 5: 写“为什么完全截不到”**

把下面内容补入 `## 4. 为什么有些窗口完全截不到`：

```md
## 4. 为什么有些窗口完全截不到

并不是所有窗口都适合通过 `PrintWindow` 稳定抓取。

以下类型的窗口更容易失败：

- Electron / Chromium 窗口
- GPU 硬件加速窗口
- DirectX / OpenGL 渲染窗口
- 最小化窗口
- 被遮挡或后台状态特殊的窗口
- 高权限或受保护窗口
- 某些现代 UI 框架窗口

在这些场景下，即使 `PrintWindow()` 返回成功，也可能出现：

- 纯黑图
- 纯白图
- 空图
- 只绘出边框或标题栏
- 绘出过期内容

这说明“抓不到”并不总是实现写错。很多时候是 API 与应用窗口实现之间天然不稳定。
```

- [ ] **Step 6: 写“当前实现风险点”和“可选改进方向”**

把下面内容补入 `## 5. 当前实现的具体风险点` 和 `## 6. 可选改进方向`：

```md
## 5. 当前实现的具体风险点

当前实现至少存在以下风险点：

- 使用 `GetClientRect()` 作为尺寸来源，存在语义偏差风险
- `PrintWindow()` 即使返回成功，也不代表内容完整
- 当前只过滤纯黑/纯白图，无法识别“半错图”
- 没有对不同窗口类型制定降级策略
- 没有后备抓图路径

其中最直接的问题是尺寸来源风险；它最容易导致“只截到标题栏”或“只截到顶部一部分”。

## 6. 可选改进方向

后续如果要优化，可以优先考虑以下方向：

- 改用整窗尺寸而不是客户区尺寸
- 评估 `GetWindowRect()` 与 `DWMWA_EXTENDED_FRAME_BOUNDS`
- 明确后续到底是要截“整窗”还是“客户区”
- 对抓图失败窗口降级为图标 + 标题
- 把问题区分为“可优化问题”和“平台不可保证问题”

需要强调的是：

- 这些方向能提升稳定性
- 但不能承诺让所有窗口都变成稳定可抓
```

- [ ] **Step 7: 写“非普通 bug 场景”和“结论”**

把下面内容补入 `## 7. 不应误判为普通 bug 的场景` 和 `## 8. 结论`：

```md
## 7. 不应误判为普通 bug 的场景

以下场景不应简单归类为“普通实现 bug”：

- 某些窗口即使修正当前尺寸策略后，仍然抓不到完整内容
- 某些应用类型天然就不稳定
- 某些窗口在最小化、后台、被遮挡或高权限状态下失败

这些问题更多属于平台能力边界，而不是单纯代码缺陷。

## 8. 结论

当前缩略图问题可以归纳为两大类：

第一类是实现策略问题：

- 当前使用 `GetClientRect()` 配合 `PrintWindow()`，很容易产生绘制范围与位图尺寸不一致的问题

第二类是平台与窗口类型限制：

- 某些窗口天然不适合通过 `PrintWindow()` 稳定抓取

因此：

- “只截到标题栏”或“只截到顶部一部分”最值得优先从尺寸策略上修正
- “完全抓不到”则需要接受一部分属于平台天然限制
- 后续优化应优先做尺寸语义修正和失败降级，而不是承诺全量窗口都能稳定抓取
```

- [ ] **Step 8: 检查正文是否覆盖规格要求**

逐项核对 `docs/superpowers/specs/2026-06-23-thumbnail-capture-technical-analysis-design.md` 的核心要求：

```txt
背景与问题现象
当前实现路径
为什么会只截到标题栏或截不全
为什么有些窗口完全截不到
当前实现的具体风险点
可选改进方向
不应误判为普通 bug 的场景
结论
```

预期：8 个章节都存在，并且正文明确区分“实现策略问题”和“平台/API 限制”。

## Task 2: 接入文档索引

**Files:**
- Modify: `docs/README.md`
- Reference: `docs/thumbnail-capture-technical-analysis.md`

- [ ] **Step 1: 在 docs 索引中新增技术分析文档入口**

在 `docs/README.md` 的“文档列表”中新增一节，内容如下：

```md
### 8. 缩略图抓取技术分析

文件：

- [thumbnail-capture-technical-analysis.md](file:///d:/code/myprj/mySwitcher-main/docs/thumbnail-capture-technical-analysis.md)

适合场景：

- 分析为什么缩略图只截到标题栏或截不全
- 为后续 Windows 缩略图优化做技术准备
- 区分实现问题和平台限制

核心内容：

- 当前实现路径
- 标题栏截断问题分析
- 平台/API 限制
- 风险点与改进方向
```

- [ ] **Step 2: 更新阅读路径中的开发视角推荐**

如果 `docs/README.md` 的“面向开发”阅读路径没有这份文档，则补充为：

```md
1. `project-feature-platform-analysis.md`
2. `product-execution-spec.md`
3. `v0.2-implementation-breakdown.md`
4. `thumbnail-capture-technical-analysis.md`
5. `project-roadmap.md`
```

预期：开发阅读路径中可以看到该技术分析文档。

## Task 3: 文档自检和诊断检查

**Files:**
- Reference: `docs/thumbnail-capture-technical-analysis.md`
- Reference: `docs/README.md`

- [ ] **Step 1: 搜索占位词**

对以下文件执行占位词检查：

```txt
docs/thumbnail-capture-technical-analysis.md
docs/README.md
```

检查模式：

```txt
TODO
TBD
待补充
待定
```

预期：无匹配结果。

- [ ] **Step 2: 运行 diagnostics**

对以下文件运行 diagnostics：

```txt
docs/thumbnail-capture-technical-analysis.md
docs/README.md
```

预期：无诊断错误。

- [ ] **Step 3: 一致性核对**

确认以下几点：

```txt
文档没有扩展成跨平台截图总报告
文档没有直接给出实现 patch
文档明确区分“实现策略问题”和“平台限制”
文档结论与当前 Windows 抓图实现相符
```

预期：文档保持技术分析定位，不越界到实现承诺。
