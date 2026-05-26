# 维护手册

NexusKit 单人日常开发的操作参考。架构和构建细节见 `docs/architecture.md` 和 `docs/build.md`。

## 开发准则

开发新组件或功能时，优先使用 NexusKit 现有接口（`nexus::common::Thread`、`nexus::common::Event`、`nexus::Status`、`nexus::Result<T>`、`nexus::common::string` 等），避免引入新的第三方抽象层或手写重复实现。这样做的好处：

- 减少重复代码和潜在的 bug
- 统一错误处理和线程安全模型
- 后续维护只需关注一处实现

## 日常开发循环

```powershell
# 1. 拉取最新（直接在 main 上开发）
git pull

# 2. 配置 + 构建
cmake --preset windows-msvc-debug
cmake --build --preset windows-msvc-debug

# 3. 运行全量测试
ctest --preset windows-msvc-debug

# 可选：只构建单个模块
cmake --build build/windows-msvc-debug --target nexus_media --config Debug

# 可选：只运行匹配的测试
./build/windows-msvc-debug/bin/Debug/nexus_media_tests.exe "Media encoder*"
```

## 依赖管理

### 代理配置

```powershell
$env:HTTP_PROXY="http://proxy.example.com:port"
$env:HTTPS_PROXY="http://proxy.example.com:port"
```

Git 自身也需要配置代理（FetchContent 克隆时使用）：
```powershell
git config --global http.proxy http://proxy.example.com:port
git config --global https.proxy http://proxy.example.com:port
git config --global http.sslBackend openssl
```

### 启用大依赖

```powershell
# FFmpeg（需要 MSYS2 UCRT64 + NASM）
cmake -S . -B build/ffmpeg -DNEXUS_BUILD_FFMPEG=ON -DNEXUS_ENABLE_MEDIA=ON

# hidapi
cmake -S . -B build/hidapi -DNEXUS_BUILD_HIDAPI=ON -DNEXUS_ENABLE_HID=ON

# OpenSSL（需要 Perl + nmake）
cmake -S . -B build/openssl -DNEXUS_BUILD_OPENSSL=ON

# PortAudio
cmake -S . -B build/portaudio -DNEXUS_BUILD_PORTAUDIO=ON
```

### 离线构建

设置 `NEXUS_BUILD_DEPS=OFF` 并通过 CMake package discovery 提供依赖：
```powershell
cmake -S . -B build/deps-off -DNEXUS_BUILD_DEPS=OFF -DNEXUS_BUILD_TESTS=ON
```

### 更新依赖版本

依赖配方在 `cmake/deps/` 目录下。修改对应 `.cmake` 文件中的 `GIT_TAG` 后重新配置即可。

## 分阶段验证

### 何时使用 fresh build 目录

- 公共 API 变更
- CMake 构建面变更（新增 target、新依赖）
- 后端集成变更（FFmpeg 版本、platform SDK）
- 怀疑构建缓存污染

### 目录命名

```
build/phase<编号>-<简短短语>
```

示例：
```
build/phase9d-media-timestamps
build/phase8m-thread
build/phase8k-screen-xrandr-check
```

### 验证完成后

- 该 fresh build 目录可以保留作为参考，也可以删除
- 后续开发继续使用主 build 目录 `build/windows-msvc-debug`
- 不要在 fresh 目录中开始新的无关工作

## 版本发布清单

发布前确认：

- [ ] 全量测试通过：`ctest --preset windows-msvc-debug`
- [ ] Linux 测试也通过（如有条件）
- [ ] `CHANGELOG.md` 已更新
- [ ] `README.md` 模块状态已同步
- [ ] `docs/architecture.md` 模块描述已同步
- [ ] `docs/modules/*.md` 已更新
- [ ] 公开头文件 Doxygen 注释完整
- [ ] 无 `git status` 未预期的变更

版本号规则：遵循 `CMakeLists.txt` 中的 `VERSION` 字段（当前 0.1.0），Tag 格式 `v<major>.<minor>.<patch>`。

## 常见问题排查

### FFmpeg DLL 找不到

**症状**: 运行 nexus_media_tests.exe 报找不到 DLL（libavutil-*.dll 等）<br>
**原因**: FFmpeg DLL 不在 PATH 中
**解决**:
```powershell
$env:PATH = "<build-dir>\deps\ffmpeg\bin;$env:PATH"
./build/<preset>/bin/Debug/nexus_media_tests.exe
```

### MSYS2 环境问题

**症状**: `NEXUS_BUILD_FFMPEG=ON` 时配置失败，找不到 bash/make/nasm<br>
**原因**: MSYS2 UCRT64 未安装或不在 PATH
**解决**: 安装 MSYS2 并执行：
```powershell
C:\msys64\usr\bin\bash.exe -lc "pacman -Sy --needed --noconfirm mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-pkgconf make nasm"
```

### FetchContent 下载失败

**症状**: CMake 配置阶段 Git 克隆超时<br>
**原因**: 代理未设置或 SSL 后端不对
**解决**:
```powershell
git config --global http.proxy http://proxy.example.com:port
git config --global https.proxy http://proxy.example.com:port
git config --global http.sslBackend openssl
```

### Git SSL 证书错误

**症状**: `SSL certificate problem: unable to get local issuer certificate`
**解决**:
```powershell
git config --global http.sslVerify false   # 仅临时解决
# 或配置正确的 CA 证书包
```

### 测试超时

**症状**: 某些测试（特别是媒体编码器测试）执行超时<br>
**原因**: 编码器测试计算量大，默认 CTest timeout 不满足
**解决**: 直接运行测试可执行文件而非通过 ctest，或增加超时：
```powershell
ctest --test-dir build/windows-msvc-debug --timeout 120
```

### MSBuild 找不到 target

**症状**: `cmake --build --target nexus_xxx` 报 target 不存在<br>
**原因**: 该模块未启用（如 nexus_media 默认 OFF）
**解决**: 重新配置时加上对应选项：
```powershell
cmake -S . -B build/windows-msvc-debug -DNEXUS_ENABLE_MEDIA=ON
```

## 文档同步提醒

每次变更后按需更新：

| 变更类型 | 需更新 |
|---------|--------|
| 公共 API 新增/修改 | header `///` 注释 + `docs/modules/<module>.md` + `CHANGELOG.md` |
| 新增模块 | `README.md` + `docs/architecture.md` + `docs/modules/<module>.md` |
| 构建系统变更 | `docs/build.md` |
| 迭代流程/路线图变更 | `docs/iteration.md` |
| 行为变更（非 API） | `CHANGELOG.md` + 相关模块文档 |

## Documentation quality checks

Run the unified documentation hygiene gate before publishing docs or release
guidance changes:

```powershell
python scripts/check_docs.py --dir .
```

This runs UTF-8 text encoding checks, release documentation invariants, and
public header Doxygen coverage checks.
