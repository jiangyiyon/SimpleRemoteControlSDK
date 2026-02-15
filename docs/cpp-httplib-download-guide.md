# cpp-httplib 下载指南

## 下载信息

**库名称**: cpp-httplib
**版本**: v0.15.3
**许可证**: MIT License
**类型**: 单文件头库（header-only）

## 下载方式

### 方式 1: 直接下载（推荐）

**下载地址**:
```
https://github.com/yhirose/cpp-httplib/releases/download/v0.15.3/httplib.h
```

**保存位置**:
```
e:\TestWebRTC\RemoteControlSDK\third_party\cpp-httplib\httplib.h
```

### 方式 2: 从 GitHub releases 页面下载

1. 访问: https://github.com/yhirose/cpp-httplib/releases
2. 找到 v0.15.3 版本（2024年1月发布）
3. 下载 `httplib.h` 文件

### 方式 3: 克隆仓库后提取

```bash
git clone --depth 1 --branch v0.15.3 https://github.com/yhirose/cpp-httplib.git temp_cpp_httplib
copy temp_cpp_httplib\httplib.h e:\TestWebRTC\RemoteControlSDK\third_party\cpp-httplib\httplib.h
rmdir /s /q temp_cpp_httplib
```

## 验证下载

下载完成后，请验证：

1. **文件大小**: 约 600-700 KB
2. **文件路径**: 确认文件位于 `third_party/cpp-httplib/httplib.h`
3. **文件内容**: 打开文件，第一行应该包含版本信息

```cpp
// httplib.h - Copyright (c) 2024 Yuji Hirose
```

## 后续步骤

下载完成后，下一步操作：

1. 告诉我下载已完成
2. 我将更新 CMakeLists.txt
3. 创建测试程序验证编译
4. 开始实现 HTTP 服务器

## 注意事项

- **不要修改 httplib.h 文件**
- **确保文件编码为 UTF-8**
- **Windows 平台**: 确保使用 Windows 换行符（CRLF）

## 技术规格

cpp-httplib v0.15.3 支持以下功能：

- ✅ HTTP 1.1 协议
- ✅ WebSocket (RFC 6455)
- ✅ 静态文件服务
- ✅ SSL/TLS 支持（OpenSSL）
- ✅ 线程安全
- ✅ CORS 支持
- ✅ 请求路由
- ✅ 表单数据处理
- ✅ JSON 数据处理

这些功能完全满足我们的需求。

## 依赖项

cpp-httplib 的依赖项：

- **必需**:
  - C++11 或更高版本（项目使用 C++20，完全兼容）

- **可选**:
  - OpenSSL (用于 HTTPS/SSL)
  - Brotli (用于压缩)
  - zlib (用于压缩)

**注意**: 我们不需要 OpenSSL，因为 HTTP 服务器只在局域网内使用，不需要 HTTPS 加密。

## 项目集成

下载后的文件将被集成到 CMake 构建系统中：

```cmake
# 在 ScreenStreamSDK/CMakeLists.txt 中
target_include_directories(screensdk PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/cpp-httplib
)
```

---

**下载完成后，请回复"下载完成"，我将继续下一步操作。**
