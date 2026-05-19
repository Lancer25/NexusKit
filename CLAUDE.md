# CLAUDE.md — NexusKit AI 协作指南

## 项目本质

NexusKit 是一个跨平台 C++17 组件库（Windows/Linux），面向设备通信、网络、音视频和屏幕采集。通过 CMake + CTest 构建，依赖通过 FetchContent 管理。

## 构建命令

```powershell
# 配置
cmake --preset windows-msvc-debug

# 构建
cmake --build --preset windows-msvc-debug

# Release 构建
cmake --preset windows-msvc-release
cmake --build --preset windows-msvc-release

# 全量测试
ctest --preset windows-msvc-debug

# 运行单个测试套件
./build/windows-msvc-debug/bin/Debug/nexus_media_tests.exe

# Release 测试
ctest --preset windows-msvc-release
./build/windows-msvc-release/bin/Release/nexus_media_tests.exe

# 运行匹配的测试用例 (Catch2 通配符)
./build/windows-msvc-debug/bin/Debug/nexus_media_tests.exe "Media encoder*"
```

需要代理时设置 HTTP_PROXY / HTTPS_PROXY 环境变量。

Windows FFmpeg 源构建需要 MSYS2 UCRT64 + NASM。详见 `docs/build.md`。

## 核心规则

1. 公共头文件 (`include/nexus/**/*.h`) 不得暴露平台头文件或第三方后端头文件
2. 可恢复的错误返回 `nexus::Status` 或 `nexus::Result<T>`
3. 拥有资源的类型使用 RAII
4. 公共 API 必须在头文件中加 `///` Doxygen 注释
5. 不引入私有 SDK 依赖
6. 开发新组件或功能时，优先使用 NexusKit 现有接口（`nexus::common::Thread`、`nexus::common::Event`、`nexus::Status` 等），避免引入新的第三方抽象或手写重复实现
7. 继续在 `main` 分支直接开发，除非用户明确要求创建分支

## 开发工作流

每次实现阶段应小、可测试、有文档：

1. 查看 `git status`、最近 `git log`、相关的公共头文件、实现代码、测试和模块文档
2. 需要设计时先写 spec → plan（设计规范 → 实施计划）
3. 先写失败的测试或文档检查
4. 用最小改动使测试通过
5. 更新相关模块文档、README、CHANGELOG
6. 运行聚焦测试验证
7. 构建或接口变化时使用 fresh 目录验证
8. 提交到 main

阶段性构建目录命名：`build/phase<编号>-<描述>`，如 `build/phase9d-media-timestamps`

## 文档索引

| 遇到问题 | 查看文档 |
|---------|---------|
| 模块架构、依赖边界 | `docs/architecture.md` |
| 构建选项、代理、依赖 | `docs/build.md` |
| API 注释规范、错误码 | `docs/api-style.md` |
| 迭代流程、近期路线图 | `docs/iteration.md` |
| 日常维护操作 | `docs/maintenance.md` |
| 各模块 API 详情 | `docs/modules/<module>.md` |
| 变更记录 | `CHANGELOG.md` |

## 当前状态

- 主开发分支：`main`
- 当前阶段：Phase 9D — Media encoder/muxer timestamp semantics
- 默认启用模块：nexus_core, nexus_log, nexus_common, nexus_net, nexus_screen
- 可选模块：nexus_usb, nexus_hid, nexus_media（需 `NEXUS_ENABLE_*=ON`）
- 测试框架：Catch2
