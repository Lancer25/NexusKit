# Build Guide

## Windows

Use MSVC 2022:

```powershell
cmake --preset windows-msvc-debug
cmake --build --preset windows-msvc-debug
ctest --preset windows-msvc-debug
```

Build output is written under `build/windows-msvc-debug`.

## Linux

Use the Linux preset from a shell with CMake and Ninja installed:

```bash
cmake --preset linux-debug
cmake --build --preset linux-debug
ctest --preset linux-debug
```

## Proxy

When fetching dependencies from GitHub, use the local proxy:

```powershell
$env:HTTP_PROXY="http://127.0.0.1:7897"
$env:HTTPS_PROXY="http://127.0.0.1:7897"
```

For Git directly:

```powershell
git config --global http.proxy http://127.0.0.1:7897
git config --global https.proxy http://127.0.0.1:7897
```

Windows Git may need the OpenSSL TLS backend when fetching through the proxy:

```powershell
git config --global http.sslBackend openssl
```

## Dependency Fetching

Dependency recipes live under `cmake/deps`.

By default, `NEXUS_BUILD_DEPS=ON`, so NexusKit may fetch or build declared dependencies from source. The current Phase 2A recipe is Catch2 for tests.

To disable source fetching, configure with `-DNEXUS_BUILD_DEPS=OFF` and provide the required packages through CMake package discovery:

```powershell
cmake -S . -B build/deps-off -DNEXUS_BUILD_DEPS=OFF -DNEXUS_BUILD_TESTS=ON
```

When dependency fetching is enabled, Git-based recipes use `NEXUS_GIT_CONFIG_ARGS`. The default is:

```cmake
http.sslBackend=openssl
```

## Install Layout

Install prefixes default to `build/install/<preset>`.
Runtime artifacts are produced under `build/<preset>/bin`.
Libraries are produced under `build/<preset>/lib`.
