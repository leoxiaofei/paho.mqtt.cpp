# paho.mqtt.cpp（轻量封装）

[English](README.md) | [简体中文](README.zh-CN.md)

对 [Eclipse Paho MQTT C](https://github.com/eclipse-paho/paho.mqtt.c) 客户端库的轻量级、**纯头文件** C++ 封装。

它**不是**重新实现——每一次调用都与底层 C API 一一对应，你对 Paho C 的了解在这里全部适用。封装提供：

- 原始 Paho 资源的 **RAII** 管理（用户代码中不会出现 `MQTTAsync_free` / `MQTTAsync_freeMessage` 等）
- 用 **`std::function` 回调**取代 函数指针 + `void* context` 的组合
- **链式 Setter 风格的选项对象**（无需手写 C 结构体初始化、无需记忆 initializer 宏）
- 同时封装**异步**（`MQTTAsync_*`）与**同步**（`MQTTClient_*`）两套 C API，对应 `MQPPAsync` 与 `MQPPSync`
- 支持 MQTT 5（connect/subscribe/publish 变体、v5 回调签名）

除 Paho C 本身外无任何依赖，C++11 即可编译。

## 目录结构

```
include/paho-mqtt-cpp/   纯头文件库
  MQPPCommon.hpp         公共枚举（持久化类型等）
  MQPPAsync.hpp          异步客户端（封装 MQTTAsync.h）
  MQPPSync.hpp           同步客户端（封装 MQTTClient.h）
test/
  main.cpp               交互式示例（连接 / 订阅 / 发布 的控制台循环）
  unit/                  单元测试（无需 broker）
```

## 环境要求

- C++11 及以上
- CMake ≥ 3.2
- [paho.mqtt.c](https://github.com/eclipse-paho/paho.mqtt.c) 1.3.x —— 源码目录或已安装均可
- OpenSSL（可选，用于 TLS）

## 编译

默认配置会把本仓库旁边的 Paho C 源码树以静态库方式编入：

```
<parent>/
  paho.mqtt.c/paho.mqtt.c-1.3.16/    # Paho C 源码
  paho.mqtt.cpp/                     # 本仓库
```

```bash
cmake -B build
cmake --build build
```

选项：

| CMake 选项 | 默认值 | 说明 |
|---|---|---|
| `PAHO_MQTT_C_USE_SUBDIR` | `ON` | 用 `add_subdirectory` 加载 Paho C 源码，而不是 `find_package` |
| `PAHO_WITH_SSL` | `OFF` | 启用 TLS。会执行 `find_package(OpenSSL)`（经由 Paho C），并定义 `OPENSSL` 宏 |
| `PAHO_MQTT_CPP_BUILD_EXAMPLE` | `ON` | 编译交互式示例（`test/main.cpp`） |
| `PAHO_MQTT_CPP_BUILD_TESTS` | `ON` | 编译单元测试（`test/unit/`） |

在其他工程中引用：

```cmake
if(NOT TARGET paho-mqtt-cpp)
  set(PAHO_WITH_SSL ON)   # 可选
  add_subdirectory(thirdparty/paho.mqtt.cpp)
endif()

target_link_libraries(your-app PRIVATE paho-mqtt-cpp)
```

## 使用示例

### 异步客户端

```cpp
#include <paho-mqtt-cpp/MQPPAsync.hpp>

PMPC::MQPPAsync client("tcp://broker.example.com:1883", "client-1");

client.SetCallbacks(
    /* 连接断开 */
    [](char* cause) { std::cout << "connection lost: " << (cause ? cause : "") << "\n"; },
    /* 消息到达 —— topic 与 message 均为 RAII 自动释放 */
    [](PMPC::AsyncString&& topic, PMPC::AsyncMessage&& msg) {
        std::cout << topic << ": "
                  << std::string((char*)msg.Payload(), msg.PayloadLen()) << "\n";
        return 1;  // TRUE：表示消息已接受
    },
    /* 消息投递完成 */
    [](MQTTAsync_token token) { std::cout << "delivered " << token << "\n"; });

/* 每次"连接成功"都会触发--首次连接以及之后的每次自动重连，
   所以在这里（重新）订阅。AsyncConnectOptions 的 onSuccess 回调
   只在首次连接请求时触发一次，覆盖不到自动重连 */
client.SetConnected([&client](char*) {
    client.Subscribe("my/topic/#", 1);
});

PMPC::AsyncConnectOptions opt(
    [](MQTTAsync_successData*) {},
    [](MQTTAsync_failureData* f) { /* 首次连接失败 */ });

opt.SetAutomaticReconnect(true);
opt.SetKeepAliveInterval(30);

///注意：Connect() 返回 SUCCESS 只表示 CONNECT 命令已入队，握手尚未完成。
///紧跟着调用 Subscribe() 会返回 MQTTASYNC_DISCONNECTED（若忽略返回值，
///订阅会被静默丢弃）
if (client.Connect(opt) != MQTTASYNC_SUCCESS) {
    /* 同步失败：参数非法、已有连接流程在进行中…… */
}
```

连接建立后（例如在 SetConnected 回调里，或确认连接已建立后）即可发布：

```cpp
PMPC::AsyncMessage msg;
msg.SetQOS(1);
msg.SetPayload(buffer, len);
client.PublishMessage("my/topic", msg);
```

完整交互式示例（连接 / 断开 / 订阅 / 发布 的控制台循环）见 [`test/main.cpp`](test/main.cpp)。

### 同步客户端

```cpp
#include <paho-mqtt-cpp/MQPPSync.hpp>

PMPC::MQPPSync client("tcp://broker.example.com:1883", "client-1");
client.SetCallbacks(
    [](char* cause) {},
    [](PMPC::SyncString&& topic, PMPC::SyncMessage&& msg) { return 1; },
    [](MQTTClient_deliveryToken) {});

PMPC::SyncConnectOptions opt;       // MQTT 5 用 opt(true)
opt.SetKeepAliveInterval(30);

if (client.Connect(opt) == MQTTCLIENT_SUCCESS) {
    client.Subscribe("my/topic", 1);
    client.Publish("my/topic", 5, "hello", 1, 0);
    client.Disconnect(1000);
}
```

### TLS / SSL

1. 引入本工程时打开 `PAHO_WITH_SSL`（它会在 Paho C 构建中启用 OpenSSL）。
2. 服务器地址使用 `ssl://` 或 `mqtts://` 前缀。
3. 连接前配置 SSL 选项——见 [`test/main.cpp`](test/main.cpp) 中的 `#ifdef OPENSSL` 段：

```cpp
PMPC::AsyncSSLOptions ssl;
ssl.SetTrustStore("path/to/cacert.pem");  // CA 证书
ssl.SetEnableServerCertAuth(1);           // 仅调试时可关闭
PMPC::AsyncConnectOptions opt(/* onSuccess */, /* onFailure */);
opt.SetSSL(ssl);                          // AsyncConnectOptions::SetSSL
```

> `SetSSL` / `SetWill` / `SetUserPass` 存的是裸指针，Paho C 在整个连接期间（包括自动重连）都会持有它——这些选项对象必须比客户端活得更久。

### 生命周期注意事项

- 以**右值传入回调**的对象（`AsyncString&&`、`AsyncMessage&&`）由 RAII 持有，回调返回时即释放。不要把其中的指针存下来。
- 带**回调的连接/断开/响应选项是一次性的**：提交成功后不要再复用同一对象——回调已移交给 Paho C，触发后会自我删除。
- `MQPPAsync` / `MQPPSync` 不可拷贝、不可移动（Paho C 以对象自身地址作为回调上下文）。

## 测试

```bash
cmake -B build
cmake --build build
./build/mqtt-unit-tests          # Windows 下为 build\mqtt-unit-tests.exe
```

单元测试**无需 broker**（覆盖 RAII 封装、选项对象、回调适配器，以及针对不可达地址的错误路径）。如果本地有 broker，可设置 `MQTT_TEST_BROKER` 额外执行一次发布/订阅回环测试：

```bash
MQTT_TEST_BROKER=tcp://127.0.0.1:1883 ./build/mqtt-unit-tests
```

## 许可证

[Eclipse Public License - v 2.0](LICENSE)，与 Eclipse Paho 一致。
