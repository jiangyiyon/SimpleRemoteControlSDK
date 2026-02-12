# WebRTC 下载指南

本目录包含三个 WebRTC 下载脚本，根据不同需求选择使用。

## 方法对比

| 方法 | 适用场景 | 优点 | 缺点 | 推荐度 |
|------|---------|------|------|--------|
| `download_webrtc.bat` | 完整 WebRTC 源码 | 功能完整，可自定义 | 下载慢（10GB+），复杂 | ⭐⭐ |
| `download_webrtc_simple.bat` | 快速获取 WebRTC | 简单快速，轻量 | 功能受限 | ⭐⭐⭐ |
| `download_webrtc_vcpkg.bat` | 使用 vcpkg 包管理 | 易于集成，版本管理 | 需要配置 vcpkg | ⭐⭐⭐⭐⭐ |

## 推荐方案

### 方案 A: 使用 vcpkg (推荐) ⭐⭐⭐⭐⭐

```cmd
download_webrtc_vcpkg.bat
```

**优点:**
- 最易于 CMake 集成
- 自动管理依赖
- 版本控制简单

**安装 libdatachannel:**
- 轻量级 WebRTC 数据通道库
- 适合远程桌面应用
- CMake 友好

**安装后配置:**
```cmake
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 ^
  -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake
```

### 方案 B: 简易下载 (次选) ⭐⭐⭐

```cmd
download_webrtc_simple.bat
```

**优点:**
- 下载快速
- 使用 GitHub 镜像（国内可用）

**缺点:**
- 功能受限
- 需要手动配置 CMake

### 方案 C: 完整源码 (高级用户) ⭐⭐

```cmd
download_webrtc.bat
```

**优点:**
- 完整功能
- 可自定义编译

**缺点:**
- 下载慢（10GB+）
- 需要 depot_tools
- 编译时间长

## 快速开始

### 如果您在国内网络环境:

1. **推荐:** 使用 vcpkg + libdatachannel
   ```cmd
   download_webrtc_vcpkg.bat
   # 选择 1 (libdatachannel)
   ```

2. **备用:** 使用简易脚本
   ```cmd
   download_webrtc_simple.bat
   ```

### 如果您需要完整 WebRTC 功能:

```cmd
# 使用镜像源
download_webrtc.bat --mirror
```

## 常见问题

### Q: chromium.googlesource.com 无法访问?
**A:** 使用 vcpkg 方案或简易脚本，它们使用 GitHub 镜像。

### Q: 下载速度慢?
**A:** 
- vcpkg 方案最快
- 简易脚本中等
- 完整源码最慢

### Q: 磁盘空间不足?
**A:** 使用 vcpkg 或简易脚本，它们占用空间小。

### Q: 如何在 CMake 中使用?

**vcpkg 方案:**
```cmake
find_package(libdatachannel CONFIG REQUIRED)
target_link_libraries(your_target 
  libdatachannel::libdatachannel
)
```

**简易脚本:**
```cmake
# 手动指定路径
target_include_directories(your_target 
  PRIVATE ${CMAKE_SOURCE_DIR}/third_party/webrtc/src/include
)
target_link_libraries(your_target 
  ${CMAKE_SOURCE_DIR}/third_party/webrtc/src/lib/libwebrtc.lib
)
```

## 网络问题

如果遇到网络连接问题:

1. **使用代理:**
   ```cmd
   set HTTP_PROXY=http://127.0.0.1:7890
   set HTTPS_PROXY=http://127.0.0.1:7890
   download_webrtc_vcpkg.bat
   ```

2. **使用镜像:**
   ```cmd
   download_webrtc.bat --mirror
   ```

3. **使用国内镜像源:**
   编辑 `download_webrtc.bat`，将 URL 改为:
   - `https://hub.fastgit.xyz/`
   - `https://ghproxy.com/`

## 项目集成

下载完成后，需要在项目的 `CMakeLists.txt` 中配置:

### vcpkg 集成示例:
```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.15)
project(RemoteControlSDK)

# 使用 vcpkg 工具链
find_package(libdatachannel CONFIG REQUIRED)

add_library(mylib ...)
target_link_libraries(mylib PRIVATE libdatachannel::libdatachannel)
```

### 编译命令:
```cmd
cd build
cmake .. -G "Visual Studio 17 2022" -A x64 ^
  -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Debug
```

## 下一步

WebRTC 库下载完成后:

1. 更新 `ScreenStreamSDK/CMakeLists.txt` 添加 WebRTC 链接
2. 实现 WebRTC 管理器 (T015)
3. 实现 WebRTC 数据通道 (T017)
4. 实现 HTTP/WebSocket 信令服务器 (T018)

## 帮助

如果遇到问题:

1. 查看脚本输出的错误信息
2. 检查网络连接
3. 确认 Git 已安装 (`git --version`)
4. 确认磁盘空间充足 (至少 10GB 用于完整源码)

## 参考资源

- [libdatachannel GitHub](https://github.com/paullouisageneau/libdatachannel)
- [vcpkg 官方文档](https://vcpkg.io/)
- [WebRTC 官方网站](https://webrtc.org/)
