# paho.mqtt.cpp (lightweight)

[English](README.md) | [简体中文](README.zh-CN.md)

A lightweight, **header-only** C++ wrapper for the [Eclipse Paho MQTT C](https://github.com/eclipse-paho/paho.mqtt.c) client library.

It is *not* a re-implementation — every call maps 1:1 onto the underlying C API, so anything you know about Paho C applies here. The wrapper adds:

- **RAII** around raw Paho resources (`MQTTAsync_free` / `MQTTAsync_freeMessage` etc. never appear in user code)
- **`std::function` callbacks** instead of function-pointer + `void* context` pairs
- **Setter-style options objects** (no zero-initialized C structs, no initializer-macro juggling)
- Both the **asynchronous** (`MQTTAsync_*`) and **synchronous** (`MQTTClient_*`) C APIs, in `MQPPAsync` and `MQPPSync` respectively
- MQTT 5 support (connect/subscribe/publish variants, v5 callback signatures)

No dependencies beyond Paho C itself and a C++11 compiler.

## Contents

```
include/paho-mqtt-cpp/   header-only library
  MQPPCommon.hpp         shared enums (persistence type, ...)
  MQPPAsync.hpp          async client  (wraps MQTTAsync.h)
  MQPPSync.hpp           sync client   (wraps MQTTClient.h)
test/
  main.cpp               interactive example (connect / sub / pub console loop)
  unit/                  unit tests (no broker required)
```

## Requirements

- C++11 or later
- CMake ≥ 3.2
- [paho.mqtt.c](https://github.com/eclipse-paho/paho.mqtt.c) 1.3.x — either as a source checkout, or installed
- OpenSSL (optional, for TLS)

## Building

The default configuration builds Paho C as a static library from a source tree located next to this repository:

```
<parent>/
  paho.mqtt.c/paho.mqtt.c-1.3.16/    # Paho C source
  paho.mqtt.cpp/                     # this repository
```

```bash
cmake -B build
cmake --build build
```

Options:

| CMake option | Default | Description |
|---|---|---|
| `PAHO_MQTT_C_USE_SUBDIR` | `ON` | `add_subdirectory` the Paho C source tree instead of `find_package` |
| `PAHO_WITH_SSL` | `OFF` | Enable TLS. Runs `find_package(OpenSSL)` (via Paho C) and defines `OPENSSL` |
| `PAHO_MQTT_CPP_BUILD_EXAMPLE` | `ON` | Build the interactive example (`test/main.cpp`) |
| `PAHO_MQTT_CPP_BUILD_TESTS` | `ON` | Build the unit tests (`test/unit/`) |

To consume the library from another project:

```cmake
if(NOT TARGET paho-mqtt-cpp)
  set(PAHO_WITH_SSL ON)   # optional
  add_subdirectory(thirdparty/paho.mqtt.cpp)
endif()

target_link_libraries(your-app PRIVATE paho-mqtt-cpp)
```

## Usage

### Async client

```cpp
#include <paho-mqtt-cpp/MQPPAsync.hpp>

PMPC::MQPPAsync client("tcp://broker.example.com:1883", "client-1");

client.SetCallbacks(
    /* connection lost */
    [](char* cause) { std::cout << "connection lost: " << (cause ? cause : "") << "\n"; },
    /* message arrived — topic and message are freed automatically (RAII) */
    [](PMPC::AsyncString&& topic, PMPC::AsyncMessage&& msg) {
        std::cout << topic << ": "
                  << std::string((char*)msg.Payload(), msg.PayloadLen()) << "\n";
        return 1;  // TRUE: message accepted
    },
    /* delivery complete */
    [](MQTTAsync_token token) { std::cout << "delivered " << token << "\n"; });

PMPC::AsyncConnectOptions opt(
    /* connect succeeded - CONNACK received, the session is up: subscribe here */
    [&client](MQTTAsync_successData*) { client.Subscribe("my/topic/#", 1); },
    [](MQTTAsync_failureData* f) { /* on failure */ });

opt.SetAutomaticReconnect(true);
opt.SetKeepAliveInterval(30);

///NOTE: Connect() returning SUCCESS only means the CONNECT command was queued -
///the handshake has not finished yet. Subscribing right after Connect() returns
///MQTTASYNC_DISCONNECTED (the subscription is silently lost if the return code
///is ignored). Subscribe from the onSuccess callback, or from the SetConnected
///callback - which also covers automatic reconnects
if (client.Connect(opt) != MQTTASYNC_SUCCESS) {
    /* synchronous failure: bad options, connect already in flight, ... */
}
```

Publishing can start once the connection is established (e.g. from the `onSuccess` callback above, or after `SetConnected` has fired):

```cpp
PMPC::AsyncMessage msg;
msg.SetQOS(1);
msg.SetPayload(buffer, len);
client.PublishMessage("my/topic", msg);
```

See [`test/main.cpp`](test/main.cpp) for a complete interactive example (connect / disconnect / subscribe / publish console loop).

### Sync client

```cpp
#include <paho-mqtt-cpp/MQPPSync.hpp>

PMPC::MQPPSync client("tcp://broker.example.com:1883", "client-1");
client.SetCallbacks(
    [](char* cause) {},
    [](PMPC::SyncString&& topic, PMPC::SyncMessage&& msg) { return 1; },
    [](MQTTClient_deliveryToken) {});

PMPC::SyncConnectOptions opt;       // opt(true) for MQTT 5
opt.SetKeepAliveInterval(30);

if (client.Connect(opt) == MQTTCLIENT_SUCCESS) {
    client.Subscribe("my/topic", 1);
    client.Publish("my/topic", 5, "hello", 1, 0);
    client.Disconnect(1000);
}
```

### TLS / SSL

1. Turn on `PAHO_WITH_SSL` when including this project (it enables OpenSSL in the Paho C build).
2. Use an `ssl://` or `mqtts://` server URI.
3. Configure SSL options on the connect options — see the `#ifdef OPENSSL` block in [`test/main.cpp`](test/main.cpp):

```cpp
PMPC::AsyncSSLOptions ssl;
ssl.SetTrustStore("path/to/cacert.pem");  // certificate authority
ssl.SetEnableServerCertAuth(1);           // leave OFF only for debugging
PMPC::AsyncConnectOptions opt(/* onSuccess */, /* onFailure */);
opt.SetSSL(ssl);                          // AsyncConnectOptions::SetSSL
```

> `SetSSL` / `SetWill` / `SetUserPass` store raw pointers into Paho C that stay valid for the whole connection (including automatic reconnect) — the option objects must outlive the client.

### Lifetime notes

- Objects passed **by rvalue into a callback** (`AsyncString&&`, `AsyncMessage&&`) are RAII-owned and freed when the callback returns. Do not store the pointers.
- Connect/disconnect/response **options with callbacks are one-shot**: never resubmit the same object after a successful submit — the callbacks have already been handed to Paho C and delete themselves after firing.
- `MQPPAsync` / `MQPPSync` are non-copyable and non-movable (Paho C holds the object's address as callback context).

## Tests

```bash
cmake -B build
cmake --build build
./build/mqtt-unit-tests          # or build\mqtt-unit-tests.exe on Windows
```

Unit tests run **without a broker** (they cover the RAII wrappers, options objects, callback adapters, and error paths against an unreachable address). If a broker is available, set `MQTT_TEST_BROKER` to run an additional publish/subscribe round-trip test:

```bash
MQTT_TEST_BROKER=tcp://127.0.0.1:1883 ./build/mqtt-unit-tests
```

## License

[Eclipse Public License - v 2.0](LICENSE), same as Eclipse Paho.
