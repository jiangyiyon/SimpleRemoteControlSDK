# Research: Third-Party Dependencies

**Feature**: 1-lan-remote-desktop
**Date**: 2025-02-11
**Status**: Draft

## Research Overview

This document evaluates third-party libraries required for the LAN Low-Latency Remote Desktop SDK implementation. All dependencies are assessed against constitutional requirements for security, licensing, maintenance status, and compatibility.

## Dependency Evaluation

### 1. libwebrtc

**Purpose**: WebRTC networking library for peer-to-peer video/audio/data transmission

**Version**: Latest stable from official repo
**License**: BSD 3-Clause
**Repository**: https://webrtc.googlesource.com/src

**Evaluation**:

| Criterion | Status | Notes |
|-----------|----------|--------|
| Functionality | ✅ Excellent | Industry-standard WebRTC implementation, full feature set |
| Performance | ✅ Excellent | Highly optimized, hardware acceleration support |
| Cross-platform | ✅ Excellent | Windows, macOS, Linux, mobile platforms |
| Documentation | ⚠️ Moderate | API documentation exists but scattered, examples limited |
| Maintenance | ✅ Active | Google actively maintained, regular updates |
| Community | ✅ Large | Large community, many production deployments |
| Build Complexity | ⚠️ High | Complex build system (GN), large dependency tree |
| Binary Size | ⚠️ Large | ~50MB static library |
| Learning Curve | ⚠️ Steep | Complex API, requires WebRTC expertise |

**Alternative Considered**: [DELETED - Security/Constitution] libdatachannel (lighter weight but less mature)

**Recommendation**: ✅ **USE libwebrtc**

**Rationale**:
- Industry standard with proven production reliability
- Hardware acceleration for H.264 encoding/decoding
- Active maintenance and security updates
- Constitution requirement to use official WebRTC native APIs
- Performance is critical for 30ms latency target

**Integration Notes**:
- Use libwebrtc native C++ API (not JavaScript wrapper)
- Requires careful memory management (RAII patterns required)
- Build via GN/Ninja toolchain
- Consider dynamic linking to reduce binary size if multiple apps share library

**Risk Mitigation**:
- Abstract WebRTC complexity behind wrapper layer (src/transport/webrtc_manager.cpp)
- Create custom video source adapter for screen capture integration
- Extensive testing on Windows 10/11 target platforms

---

### 2. spdlog

**Purpose**: Fast C++ logging library

**Version**: 1.13.0 or later
**License**: MIT
**Repository**: https://github.com/gabime/spdlog

**Evaluation**:

| Criterion | Status | Notes |
|-----------|----------|--------|
| Functionality | ✅ Excellent | Header-only or compiled, multiple sinks, formatting |
| Performance | ✅ Excellent | Asynchronous logging, zero-copy where possible |
| Cross-platform | ✅ Excellent | Windows, macOS, Linux |
| Documentation | ✅ Excellent | Comprehensive README and examples |
| Maintenance | ✅ Active | Regular releases, responsive maintainers |
| Community | ✅ Large | Popular in C++ community, many production uses |
| Build Complexity | ✅ Low | Header-only option available |
| Binary Size | ✅ Small | Minimal overhead |

**Alternative Considered**: [DELETED] glog (Google logging, but deprecated and larger footprint)

**Recommendation**: ✅ **USE spdlog**

**Rationale**:
- MIT license (constitution-compatible)
- Header-only option reduces build complexity
- Asynchronous logging critical for performance (non-blocking)
- Active maintenance and excellent documentation
- Meets constitution requirement for structured logging

**Integration Notes**:
- Use header-only mode for fastest build times
- Configure async logger with multi-threaded support
- Create structured JSON logs for parsing by monitoring tools
- Implement log rotation to prevent disk space issues

**Configuration**:
- Log levels: TRACE, DEBUG, INFO, WARN, ERROR (default: INFO)
- Output: File (logs/screensdk.log) + Console (debug mode)
- Format: Timestamp [ThreadID] [Level] Message
- Async queue: 8192 messages, 2 flush threads

---

### 3. nlohmann/json

**Purpose**: Modern C++ JSON library for configuration and data exchange

**Version**: 3.11.2 or later
**License**: MIT
**Repository**: https://github.com/nlohmann/json

**Evaluation**:

| Criterion | Status | Notes |
|-----------|----------|--------|
| Functionality | ✅ Excellent | Full JSON standard support, intuitive API |
| Performance | ✅ Excellent | Fast parsing and generation |
| Cross-platform | ✅ Excellent | Header-only, standard C++11+ |
| Documentation | ✅ Excellent | Comprehensive, many examples |
| Maintenance | ✅ Active | Regular updates, responsive maintainers |
| Community | ✅ Very Large | One of most popular C++ JSON libraries |
| Build Complexity | ✅ Minimal | Header-only, single file |
| Binary Size | ✅ Negligible | Minimal runtime overhead |

**Alternative Considered**: [DELETED] RapidJSON (faster but less intuitive API, harder to maintain)

**Recommendation**: ✅ **USE nlohmann/json**

**Rationale**:
- MIT license (constitution-compatible)
- Intuitive syntax reduces bugs and improves developer productivity
- Header-only simplifies build and deployment
- Industry-standard in modern C++ projects
- Excellent error messages for debugging

**Integration Notes**:
- Use for configuration file parsing (config/default.json)
- Serialize/deserialize internal data structures
- Use JSON for signaling protocol (SDP exchange)
- Implement schema validation for configuration

**Usage Patterns**:
```cpp
// Configuration parsing
#include <nlohmann/json.hpp>
using json = nlohmann::json;

json config = json::parse(config_file);
int port = config["server"]["port"].get<int>();
bool hardware_encode = config["video"]["hardware_encode"].get<bool>();
```

---

## Open Source Library Evaluation Reports

### Library 1: libwebrtc (Level 1 - Critical Dependency)

## 开源库引入申请报告

### 📦 库基本信息
- **库名称**: libwebrtc
- **GitHub地址**: https://webrtc.googlesource.com/src
- **最新版本**: Latest stable (branch main)
- **许可证类型**: BSD 3-Clause
- **维护状态**: 极度活跃（Google官方维护，每日提交）

### 🤔 引入必要性评估
**解决的问题**:
1. 核心功能需求: WebRTC协议实现（P2P视频传输、信令、ICE协商），这是远程桌面SDK的核心网络层
2. 开发效率: 从零实现WebRTC需要数年工作，使用libwebrtc可立即获得经过数百万用户验证的稳定实现
3. 质量保证: Google团队持续维护，经过严格安全审计和性能优化

**替代方案对比**:
- 方案A（推荐库 libwebrtc）: ✅ 成熟稳定、硬件加速、跨平台、长期维护。劣势：构建复杂、二进制体积大
- 方案B（自实现）: 🚫 技术复杂度极高、需要数年时间、难以达到安全/性能标准
- 方案C（其他库 libdatachannel）: ⚠️ 较轻量但功能不完整、社区较小、未经大规模生产验证

### ⚠️ 风险评估
**技术风险**: 构建系统复杂（GN/Ninja），需要深入WebRTC专业知识，API学习曲线陡峭
**法律风险**: BSD 3-Clause许可证完全兼容Apache 2.0项目要求
**维护风险**: 风险极低，Google官方长期维护承诺

### 📊 成本效益分析
**引入成本**: 高（学习成本2-3周、构建系统集成1周、测试2周）
**长期收益**: 极高（核心功能可靠、性能达标、社区支持、持续安全更新）

### 🔧 集成方案建议
**引入方式**: 源码集成（自定义编译）
**集成步骤**:
1. 设置GN构建环境（Python 3.10+, depot_tools）
2. 编译Windows静态库（仅需要的组件：PeerConnection, VideoTrack, DataChannel）
3. 创建包装层（src/transport/webrtc_manager.cpp）隐藏复杂API
4. 实现自定义视频源适配器连接DXGI采集
**回滚计划**: 移除编译输出和wrapper层，重新编译不依赖WebRTC的代码

### ✅ 最终建议
- **推荐指数**: ⭐️⭐️⭐️⭐️⭐️ (5/5星)
- **建议版本**: Latest stable (track main branch)
- **引入时机**: 立即（Level 1关键依赖）

---

### Library 2: spdlog (Level 2 - 辅助工具)

## 开源库引入申请报告

### 📦 库基本信息
- **库名称**: spdlog
- **GitHub地址**: https://github.com/gabime/spdlog
- **最新版本**: 1.13.0
- **许可证类型**: MIT
- **维护状态**: 活跃（定期发布，响应社区问题）

### 🤔 引入必要性评估
**解决的问题**:
1. 核心功能需求: 结构化日志记录（宪法要求），异步非阻塞日志输出
2. 开发效率: 避免重复实现日志库，节省1-2周开发时间
3. 质量保证: 经过大规模生产使用，性能优化完善

**替代方案对比**:
- 方案A（推荐库 spdlog）: ✅ 异步日志、header-only、高性能、文档完善
- 方案B（自实现）: ⚠️ 可行但浪费时间，异步日志实现需要仔细处理线程安全和性能
- 方案C（其他库 glog）: 🚫 已废弃、体积大、配置复杂

### ⚠️ 风险评估
**技术风险**: 极低（header-only模式，无外部依赖）
**法律风险**: MIT许可证完全兼容Apache 2.0
**维护风险**: 极低（活跃社区，许多生产部署）

### 📊 成本效益分析
**引入成本**: 低（集成时间2-3天，学习成本<1天）
**长期收益**: 高（性能达标、维护成本降低、代码整洁）

### 🔧 集成方案建议
**引入方式**: Header-only（无需编译）
**集成步骤**:
1. 复制单文件头到third_party/spdlog/
2. 配置异步日志（8192消息队列，2个flush线程）
3. 创建包装器（src/utils/logger.cpp）统一日志接口
4. 实现文件轮转防止磁盘溢出
**回滚计划**: 删除third_party/spdlog/目录，移除logger.cpp中的spdlog引用

### ✅ 最终建议
- **推荐指数**: ⭐️⭐️⭐️⭐️⭐️ (5/5星)
- **建议版本**: 1.13.0或更新
- **引入时机**: 立即（Level 2辅助工具）

---

### Library 3: nlohmann/json (Level 2 - 辅助工具)

## 开源库引入申请报告

### 📦 库基本信息
- **库名称**: nlohmann/json
- **GitHub地址**: https://github.com/nlohmann/json
- **最新版本**: 3.11.2
- **许可证类型**: MIT
- **维护状态**: 活跃（定期发布，响应式维护）

### 🤔 引入必要性评估
**解决的问题**:
1. 核心功能需求: JSON配置文件解析、信令协议数据交换
2. 开发效率: 直观API减少bug，提高开发效率30%+
3. 质量保证: 行业标准，经过大规模生产验证

**替代方案对比**:
- 方案A（推荐库 nlohmann/json）: ✅ API直观、header-only、性能优秀、文档完善
- 方案B（自实现）: ⚠️ 可行但易出错，JSON规范复杂，边缘情况多
- 方案C（其他库 RapidJSON）: ⚠️ 性能更好但API复杂，维护成本高

### ⚠️ 风险评估
**技术风险**: 极低（header-only，标准C++11，无外部依赖）
**法律风险**: MIT许可证完全兼容Apache 2.0
**维护风险**: 极低（最流行C++ JSON库，社区庞大）

### 📊 成本效益分析
**引入成本**: 极低（集成时间<1天，学习成本<1小时）
**长期收益**: 高（开发效率提升、代码可读性提高、维护成本降低）

### 🔧 集成方案建议
**引入方式**: Header-only（单文件）
**集成步骤**:
1. 复制json.hpp头文件到third_party/nlohmann/
2. 用于配置解析（config/default.json）
3. 用于信令协议序列化/反序列化
4. 实现JSON schema验证（可选，增强健壮性）
**回滚计划**: 删除third_party/nlohmann/目录，替换为简单键值对配置

### ✅ 最终建议
- **推荐指数**: ⭐️⭐️⭐️⭐️⭐️ (5/5星)
- **建议版本**: 3.11.2或更新
- **引入时机**: 立即（Level 2辅助工具）

---

## Summary

**Approved Dependencies**:
1. ✅ **libwebrtc** (Level 1 - Critical)
2. ✅ **spdlog** (Level 2 - Approved)
3. ✅ **nlohmann/json** (Level 2 - Approved)

**All dependencies**:
- Licensed under MIT or BSD (constitution-compatible with Apache 2.0)
- Actively maintained with regular security updates
- Proven in production environments
- Meet constitutional requirements for performance and observability

**Next Steps**:
1. Create detailed data models (data-model.md)
2. Define API contracts (contracts/)
3. Write quickstart guide (quickstart.md)
4. Proceed to task planning (/speckit.tasks)
