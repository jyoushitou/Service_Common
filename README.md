# Common — C++ 公共库

> **微服务架构的基石**：为所有 C++ 微服务提供统一的网络通信、线程池、消息协议和工具模块。

![C++](https://img.shields.io/badge/C++-17-%2300599C?style=flat-square&logo=c%2B%2B)
![Boost.Asio](https://img.shields.io/badge/Boost.Asio-1.83-%23F7901E?style=flat-square&logo=boost)
![Protobuf](https://img.shields.io/badge/Protobuf-6.x-%23FF6C37?style=flat-square&logo=protocol-buffers)

---

## 📖 概述

### 由于一些历史性依赖问题，请拉取了这个仓库后，重命名为common，且本仓库的所有common均指此仓库

`common` 是 WebServer 微服务架构中 **所有 C++ 微服务的公共依赖库**，提供以下核心能力：

| 模块           | 说明                                                         |
| -------------- | ------------------------------------------------------------ |
| **网络模块**   | 基于 Boost.Asio 的 TCP 连接/服务器封装，异步 I/O、消息编解码 |
| **线程池**     | 通用任务队列，`enques` 提交异步任务，返回 `std::future`      |
| **消息协议**   | 自定义 TCP 帧协议（8 字节消息 ID + 4 字节长度 + 消息体）     |
| **服务标识**   | 统一定义微服务 ID 常量与映射（16 个服务）                    |
| **工具模块**   | 控制台日志输出（`Out_Msg`/`Out_Err`/`Out_Net_Msg`）          |
| **Proto 消息** | Protobuf 生成的 `header`/`error`/`goal`/`Empty` 消息体       |

---

## 🏗️ 目录结构

```
common/                                 # [子仓库] 公共库 (C++)
├── include/                            # 公共头文件
│   ├── ThreadPool.h                    # 线程池（任务队列、条件变量）
│   ├── NetServer.h                     # TCP 服务器（accept、会话管理、消息队列）
│   ├── NetConnection.h                 # TCP 连接封装（异步读写、发送队列）
│   ├── Message.h                       # 服务 ID 常量 + 网络协议常量
│   ├── Utils.h                         # 日志/初始化工具
│   └── Common.pb.h                     # Protobuf 生成的消息定义
├── body/                               # 源码实现
│   ├── ThreadPool.cpp                  # 线程池实现
│   ├── NetServer.cpp                   # TCP 服务器实现
│   ├── NetConnection.cpp               # TCP 连接封装实现
│   ├── Utils.cpp                       # 日志工具实现
│   └── Common.pb.cc                    # Protobuf 生成的消息实现
├── LICENSE                             # 开源协议
└── README.md                           # 本文件
```

---

## 🧩 模块详解

### 1️⃣ 线程池 (`ThreadPool.h`)

基于 C++11 标准库实现的高性能线程池：

| 特性           | 说明                                                                 |
| -------------- | -------------------------------------------------------------------- |
| **任务提交**   | `enques(F&& f, Arg&&... arg)` 提交任意可调用对象，返回 `std::future` |
| **默认线程数** | 4（可通过构造函数指定）                                              |
| **优雅退出**   | 析构时通知所有工作线程，等待队列任务完成                             |
| **线程安全**   | 互斥锁 + 条件变量，支持从任意线程提交任务                            |

```cpp
#include "ThreadPool.h"

threadpool::ThreadPool pool(8);  // 创建 8 线程的线程池

auto future = pool.enques([](int a, int b) { return a + b; }, 3, 4);
int result = future.get();  // result == 7
```

### 2️⃣ 网络模块 (`NetConnection.h` / `NetServer.h`)

基于 **Boost.Asio** 的异步 TCP 网络框架：

```
┌─────────────────────────────────────────────┐
│              Net::Server                     │
│   ┌─────────────────────────────────────┐   │
│   │          Acceptor (监听)             │   │
│   └───────────────┬─────────────────────┘   │
│                   │ accept                  │
│   ┌───────────────▼─────────────────────┐   │
│   │         Session (每个连接)           │   │
│   │  ┌─────────────┐  ┌─────────────┐   │   │
│   │  │ RecvNode    │  │ SendNode    │   │   │
│   │  │ (异步读取)   │  │ (发送队列)   │   │   │
│   │  └─────────────┘  └─────────────┘   │   │
│   └───────────────┬─────────────────────┘   │
│                   │ ToWork()                │
│   ┌───────────────▼─────────────────────┐   │
│   │       消息队列 (消费者模型)          │   │
│   │  WaitForMessage() / HasMessage()    │   │
│   └─────────────────────────────────────┘   │
└─────────────────────────────────────────────┘
```

#### TCP 帧协议格式

```
┌─────────────────┬─────────────────┬─────────────────┐
│  消息 ID (8字节) │  消息长度 (4字节) │   消息体         │
│  Big-Endian     │  Big-Endian     │   (≤ 1MB)       │
│  uint64_t       │  uint32_t       │   std::string   │
└─────────────────┴─────────────────┴─────────────────┘
```

| 常量              | 值  | 说明                 |
| ----------------- | --- | -------------------- |
| `HEAD_ID_LENGTH`  | 8   | 消息 ID 字节长度     |
| `HEAD_LEN_LENGTH` | 4   | 消息长度字段字节长度 |
| `HEAD_LENGTH`     | 12  | 消息头部总长度       |
| `MAX_LENGTH`      | 1MB | 消息体最大长度       |

#### Net::Connection（连接基类）

| API                   | 说明                                  |
| --------------------- | ------------------------------------- |
| `Start()`             | 启动异步读取循环                      |
| `ToSend(msg)`         | 发送消息（自动分配消息 ID）           |
| `ToSend(msg_id, msg)` | 发送消息（显式指定 ID，用于日志追踪） |
| `ToWork(msg_id, msg)` | 收到消息回调（派生类实现业务逻辑）    |
| `ToClosed()`          | 连接关闭回调（可重写）                |
| `Close()`             | 优雅关闭（发送完队列中剩余消息）      |

#### Net::Server（TCP 服务器）

| API                | 说明                                             |
| ------------------ | ------------------------------------------------ |
| `StartAccept()`    | 开始异步接受连接                                 |
| `Stop()`           | 停止监听并关闭所有会话                           |
| `WaitForMessage()` | 阻塞等待一条消息，返回 `{session, msg_id, 内容}` |
| `HasMessage()`     | 非阻塞检查是否有消息                             |

#### Net::Server::Session（连接会话）

| API                  | 说明                         |
| -------------------- | ---------------------------- |
| `Stop()`             | 停止会话                     |
| `Reply(msg_id, msg)` | 主线程调用：向客户端回复消息 |

### 3️⃣ 服务标识 (`Message.h`)

统一定义微服务架构中的服务 ID 与名称映射：

| ID  | 服务名          | ID  | 服务名         |
| --- | --------------- | --- | -------------- |
| 1   | RPCGateway      | 9   | ServiceConsole |
| 2   | SQL             | 10  | AdminConsole   |
| 3   | Registry        | 11  | User           |
| 4   | ConfigCenter    | 12  | Article        |
| 5   | MonitorService  | 13  | Blog           |
| 6   | SecurityService | 14  | Image          |
| 7   | CertService     | 15  | Video          |
| 8   | TracingService  | 16  | Search         |

### 4️⃣ 工具模块 (`Utils.h`)

| API                                   | 说明                                      |
| ------------------------------------- | ----------------------------------------- |
| `init()`                              | 初始化（Windows 下设置 UTF-8 控制台输出） |
| `Out_Msg(msg, serviceID)`             | 输出普通信息（格式：`[服务名]消息`）      |
| `Out_Err(msg, serviceID)`             | 输出错误信息到 stderr                     |
| `Out_Net_Msg(msg_id, msg, serviceID)` | 输出带消息 ID 的网络日志                  |

### 5️⃣ Proto 消息 (`Common.pb.h` / `Common.pb.cc`)

由 `common/Common.proto` 生成的 Protobuf 消息：

| 消息     | 字段                                      | 用途           |
| -------- | ----------------------------------------- | -------------- |
| `header` | `MSGID` (uint32), `MSGLen` (uint64)       | 消息头部       |
| `error`  | `head` (header), `ServiceID` (uint32)     | 错误消息       |
| `goal`   | `head` (header), `ServiceGoalID` (uint32) | 目标服务消息   |
| `Empty`  | -                                         | 空消息（占位） |

---

## 🔧 构建说明

### 前置依赖

| 依赖       | 版本  | 说明                                     |
| ---------- | ----- | ---------------------------------------- |
| C++ 编译器 | C++17 | 核心语言标准                             |
| Boost      | 1.83+ | 网络模块依赖 Boost.Asio                  |
| Protobuf   | 3.15+ | 序列化库（生成代码基于 Protobuf 6.33.4） |

### 编译器要求

`common` 作为源码级公共库，直接与各微服务项目一起编译。各微服务通过自己的构建系统（CMake/IDE）引入：

```cmake
# 在微服务的 CMakeLists.txt 中
target_include_directories(your_service PRIVATE common/include)
target_sources(your_service PRIVATE
    common/body/ThreadPool.cpp
    common/body/NetConnection.cpp
    common/body/NetServer.cpp
    common/body/Utils.cpp
    common/body/Common.pb.cc
)
```

### Windows 注意事项

- 网络模块自动处理 Winsock2 包含顺序（`WIN32_LEAN_AND_MEAN`）
- 日志输出使用 UTF-8 编码控制台

---

## 📋 依赖关系

```
common (源码级公共库)
├── Boost.Asio        → 网络模块 (NetConnection, NetServer)
├── Protobuf          → 消息定义 (Common.pb.h/cc)
└── C++ STL           → 线程池 (thread, future, condition_variable)
```

---

## 🔗 相关仓库

| 仓库                                                         | 说明                 |
| ------------------------------------------------------------ | -------------------- |
| [WebServer](https://github.com/jyoushitou/WebServer)         | 总仓库（Git 根仓库） |
| [WebServer_cpp](https://github.com/jyoushitou/WebServer_cpp) | 单体架构归档版本     |
| [proto](https://github.com/jyoushitou/WebServer/proto)       | Proto 接口定义子仓库 |
| [common](https://github.com/jyoushitou/WebServer/common)     | 本仓库（公共库）     |

---

- 项目维护者：[jyoushitou]
- 邮箱：[xzt98948364@outlook.com]
- 博客地址：[https://jyoushitou.github.io/](https://jyoushitou.github.io/)

---

## 📌 完成状态

| 模块               | 状态 | 备注                               |
| ------------------ | ---- | ---------------------------------- |
| ThreadPool         | ✅    | 完成，支持 `enques` 异步任务提交   |
| Net::Connection    | ✅    | 完成，异步读写 + 发送队列          |
| Net::Server        | ✅    | 完成，accept + 消息队列            |
| Utils 日志工具     | ✅    | 完成，控制台输出                   |
| Message 服务常量   | ✅    | 完成，16 个服务 ID                 |
| Common.pb          | ✅    | 完成，Protobuf 消息定义            |
| Logger（文件日志） | ❌    | 未实现，当前仅控制台输出           |
| Config（配置解析） | ❌    | 未实现                             |
| RPC 模块           | ❌    | 已移除（历史版本有，v0.3 前删除）  |
| 定时器 (Timer)     | ❌    | 未实现                             |
| 缓冲区动态扩容     | ❌    | 未实现（当前使用固定大小 MsgNode） |