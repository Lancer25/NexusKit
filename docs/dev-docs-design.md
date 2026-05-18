# NexusKit 开发维护文档化 — 设计文档

日期：2026-05-16

## 目标

为 NexusKit（单人维护的 C++17 跨平台组件库）补齐三层文档，提升日常开发效率和可维护性。

## 产出物

### 1. CLAUDE.md — AI 协作入口

- **路径**: 仓库根目录 `CLAUDE.md`
- **受众**: AI 助手（Claude Code）
- **规模**: 50-60 行
- **内容**:
  - 项目本质与技术栈 (3-4 行)
  - 构建命令模板：cmake --preset / cmake --build / ctest (10 行)
  - 核心规则红线：不引入私有 SDK、公共头不暴露后端头文件、用 Status/Result 处理错误、公共 API 加 Doxygen 注释 (8 行)
  - 开发工作流：直接 main 开发、分阶段 spec→plan→implement→verify、phased build 目录命名 (8 行)
  - 文档索引表：遇到 X 问题 → 看 Y 文档 (10 行)
  - 当前模块与 phase 状态 (5 行)
- **设计原则**: 不重复已有 docs 内容，用路径索引代替；构建命令只写最常用；规则只列红线不解释原因

### 2. 维护手册 — 日常操作参考

- **路径**: `docs/maintenance.md`
- **受众**: 人类开发者（单人）
- **规模**: ~120 行
- **内容**:
  - 日常开发循环：标准命令链、单模块构建、单测试运行 (15 行)
  - 依赖管理：代理配置、大依赖启用、离线构建、版本更新 SOP (20 行)
  - 分阶段验证：fresh build 时机、目录命名规范、清理判断 (15 行)
  - 版本发布清单：发布前检查、tag 规范、版本号规则 (15 行)
  - 常见问题排查：FFmpeg DLL 缺失、MSYS2 环境、代理失败、SSL 证书、测试超时、MSBuild target 找不到 (30 行)
  - 文档同步提醒：API 变更→header 注释+module doc+CHANGELOG (10 行)
- **设计原则**: 每步有 copy-paste 可用命令；排查用症状→原因→解决三段式；不解释架构背景；不写团队流程

### 3. 技能增强 — C++/CMake 分阶段开发模式

- **路径**: `~/.claude/skills/nexuskit-cpp-workflow/SKILL.md`
- **受众**: AI 助手（跨项目复用）
- **规模**: ~100 行
- **内容**:
  - 前置依赖声明：REQUIRED BACKGROUND: superpowers:executing-plans
  - Build 验证模式：fresh build 时机、命名规范、完整周期命令
  - Catch2 测试过滤：针对 test executable 跑指定 suite
  - 依赖处理：FetchContent 代理、opt-in 大依赖
  - 常见陷阱：stale build artifacts、运行时 DLL 缺失、代理失败
- **设计原则**: 不重复 superpowers 已有内容，只补充 C++/CMake 特有步骤；可跨项目复用

## 文档间关系

```
CLAUDE.md (AI 入口, ~55 行)
   ├── 引用 docs/architecture.md
   ├── 引用 docs/build.md
   ├── 引用 docs/api-style.md
   ├── 引用 docs/iteration.md
   └── 引用 docs/maintenance.md (新增)

docs/maintenance.md (人类参考, ~120 行)
   └── 引用 docs/build.md (详细构建选项)

nexuskit-cpp-workflow (AI 技能, ~100 行)
   └── 引用 superpowers:executing-plans
```

三者职责不重叠，通过路径引用而非内容复制来关联。

## 实施顺序

1. CLAUDE.md — 立即可用，提升后续 AI 辅助开发效率
2. docs/maintenance.md — 后续日常开发中持续查阅
3. nexuskit-cpp-workflow 技能 — 下次开其他 C++ 项目时自然触发
