CMAKE_MINIMUM_REQUIRED(VERSION 3.2)

PROJECT(paho-mqtt-cpp)

option(PAHO_MQTT_C_USE_SUBDIR "Use add_subdirectory to load paho.mqtt.c instead of find_package" ON)
option(PAHO_MQTT_CPP_BUILD_EXAMPLE "Build the interactive example (test/main.cpp)" ON)
option(PAHO_MQTT_CPP_BUILD_TESTS "Build the unit tests (test/unit)" ON)

if(PAHO_MQTT_C_USE_SUBDIR)
    set(PAHO_HIGH_PERFORMANCE ON CACHE BOOL "Enable high performance mode" FORCE)
    set(LIB_ANL OFF CACHE BOOL "Disable ANL" FORCE)
    set(PAHO_BUILD_STATIC ON CACHE BOOL "Build static libraries" FORCE)
    set(PAHO_BUILD_SHARED OFF CACHE BOOL "Build shared libraries" FORCE)
    set(PAHO_ENABLE_CPACK OFF CACHE BOOL "Disable CPack" FORCE)
    set(PAHO_ENABLE_TESTING OFF CACHE BOOL "Disable tests" FORCE)
    add_subdirectory(../paho.mqtt.c/paho.mqtt.c-1.3.16 ./paho.mqtt.c EXCLUDE_FROM_ALL)
else()
    if(WIN32)
        set(MQTT_CMAKE ${CMAKE_HOST_SYSTEM_NAME}/lib/cmake/eclipse-paho-mqtt-c)
    else()
        set(MQTT_CMAKE ${CMAKE_HOST_SYSTEM_NAME}/lib64/cmake/eclipse-paho-mqtt-c)
    endif()

    set(CMAKE_PREFIX_PATH
        ${CMAKE_CURRENT_SOURCE_DIR}/../paho.mqtt.c/${MQTT_CMAKE}
        ${CMAKE_PREFIX_PATH})

    find_package(eclipse-paho-mqtt-c REQUIRED)
endif()

file(GLOB_RECURSE MQTTCPP_INC_FILE include/*.h*)

if(${CMAKE_VERSION} VERSION_LESS "3.19.0")
    add_library(${PROJECT_NAME} INTERFACE)
else()
    add_library(${PROJECT_NAME} INTERFACE ${MQTTCPP_INC_FILE})
endif()

message("PAHO_WITH_SSL: ${PAHO_WITH_SSL}")

if(PAHO_WITH_SSL)
    target_link_libraries(${PROJECT_NAME} INTERFACE eclipse-paho-mqtt-c::paho-mqtt3as-static eclipse-paho-mqtt-c::paho-mqtt3cs-static)
    target_compile_definitions(${PROJECT_NAME} INTERFACE OPENSSL)
else()
    target_link_libraries(${PROJECT_NAME} INTERFACE eclipse-paho-mqtt-c::paho-mqtt3a-static eclipse-paho-mqtt-c::paho-mqtt3c-static)
endif()

target_include_directories(${PROJECT_NAME} INTERFACE
    include)

# 示例工程
if(PAHO_MQTT_CPP_BUILD_EXAMPLE)
    add_executable(mqtt-test test/main.cpp)
    target_link_libraries(mqtt-test PRIVATE ${PROJECT_NAME})
endif()

# 单元测试（无需 broker；设置 MQTT_TEST_BROKER 环境变量可额外执行收发回环测试）
if(PAHO_MQTT_CPP_BUILD_TESTS)
    find_package(Threads REQUIRED)

    file(GLOB MQTTCPP_TEST_SRC test/unit/*.cpp)

    add_executable(mqtt-unit-tests ${MQTTCPP_TEST_SRC})
    target_link_libraries(mqtt-unit-tests PRIVATE ${PROJECT_NAME} Threads::Threads)

    enable_testing()
    add_test(NAME unit-tests COMMAND mqtt-unit-tests)
endif()
