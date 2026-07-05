# mySwitcher 文档索引

## 文档说明

本目录用于沉淀 `mySwitcher` 的项目分析、功能规划和版本路线图，方便开发、产品、测试和管理侧快速查阅。

## 阅读顺序

如果想快速了解项目，建议按下面顺序阅读：

1. 项目总览与平台限制
2. 功能优先级与投入分析
3. 版本路线图
4. 一页纸摘要

## 文档列表

### 1. 项目功能与平台限制分析

文件：

- [project-feature-platform-analysis.md](file:///d:/code/myprj/mySwitcher-main/docs/project-feature-platform-analysis.md)

适合场景：

- 快速了解项目做了什么
- 查看 Windows、macOS、Linux X11、Wayland 的能力差异
- 了解哪些功能受平台限制难以统一实现

核心内容：

- 项目定位
- 已实现功能
- 当前边界
- 平台差异
- 可扩展功能
- 受平台机制限制的能力分析

### 2. 功能优先级与投入分析

文件：

- [project-feature-priority-matrix.md](file:///d:/code/myprj/mySwitcher-main/docs/project-feature-priority-matrix.md)

适合场景：

- 做需求优先级讨论
- 做版本排期评审
- 评估功能收益、难度和风险

核心内容：

- P0/P1/P2/P3 优先级定义
- 功能优先级矩阵
- 高风险功能说明
- 低风险高收益功能建议
- 资源投入顺序建议

### 3. 开发路线图

文件：

- [project-roadmap.md](file:///d:/code/myprj/mySwitcher-main/docs/project-roadmap.md)

适合场景：

- 做版本规划
- 做里程碑管理
- 做中短期开发路线讨论

核心内容：

- v0.2 规划
- v0.3 规划
- v1.0 规划
- Wayland 专项策略
- 里程碑总结

### 4. 一页纸摘要

文件：

- [project-one-page-summary.md](file:///d:/code/myprj/mySwitcher-main/docs/project-one-page-summary.md)

适合场景：

- 发群同步
- 做管理汇报
- 给非开发同事快速说明项目状态

核心内容：

- 当前项目状态
- 平台差异结论
- 最值得投入的方向
- 主要风险和建议

### 5. Qt 跨平台设计草案

文件：

- [2026-06-23-mySwitcher-qt-design.md](file:///d:/code/myprj/mySwitcher-main/docs/superpowers/specs/2026-06-23-mySwitcher-qt-design.md)

适合场景：

- 回看最初设计意图
- 对照当前实现和原始平台策略

### 6. 执行型产品方案

文件：

- [product-execution-spec.md](file:///d:/code/myprj/mySwitcher-main/docs/product-execution-spec.md)

适合场景：

- 开发开始排期前统一范围
- 明确 `v0.2` 做什么、不做什么
- 对齐平台分级、验收标准和降级策略

核心内容：

- `v0.2` 目标与成功标准
- 范围与非目标
- 平台能力分级
- 六个核心主题的行为定义
- 风险与降级策略

### 7. v0.2 开发任务拆解

文件：

- [v0.2-implementation-breakdown.md](file:///d:/code/myprj/mySwitcher-main/docs/v0.2-implementation-breakdown.md)

适合场景：

- 开发拆任务
- 做 issue 列表
- 做迭代排期和依赖分析

核心内容：

- 模块拆分
- 任务分组
- 任务清单
- 影响文件
- 依赖关系
- 建议开发顺序
- 测试与风险

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

## 推荐用法

### 面向开发

优先阅读：

1. `project-feature-platform-analysis.md`
2. `product-execution-spec.md`
3. `v0.2-implementation-breakdown.md`
4. `thumbnail-capture-technical-analysis.md`
5. `project-roadmap.md`

### 面向产品

优先阅读：

1. `project-feature-priority-matrix.md`
2. `project-roadmap.md`

### 面向管理或汇报

优先阅读：

1. `project-one-page-summary.md`
2. `project-feature-priority-matrix.md`

## 当前建议结论

如果只保留一句话结论，可以概括为：

- `mySwitcher` 当前最适合先把 Windows 做成标杆体验，再逐步补齐 macOS 和 Linux X11 的关键缺口，同时对 Wayland 明确采用降级和限制说明策略。
