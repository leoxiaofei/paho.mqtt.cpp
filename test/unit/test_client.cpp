#define _CRT_SECURE_NO_WARNINGS

#include "test_framework.hpp"

#include <paho-mqtt-cpp/MQPPAsync.hpp>
#include <paho-mqtt-cpp/MQPPSync.hpp>

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <future>
#include <stdexcept>
#include <string>
#include <utility>

namespace
{
	///port 1 is not a listening service port on any common platform:
	///connecting there exercises the failure paths without needing a broker
	const char* const DEAD_SERVER = "tcp://127.0.0.1:1";
}

TEST(AsyncClient_CreateSucceedsWithValidURI)
{
	PMPC::MQPPAsync client("tcp://127.0.0.1:1883", "unit-async-create");

	CHECK(client);
}

TEST(AsyncClient_CreateRejectsUnsupportedProtocol)
{
	PMPC::MQPPAsync client("foo://127.0.0.1:1883", "unit-async-bad-proto");

	CHECK(!client);
}

TEST(AsyncClient_SetCallbacksThenDestroy)
{
	///the client registers its own address as callback context: destroying it
	///right after setting callbacks must not crash (no copy/move involved)
	PMPC::MQPPAsync client("tcp://127.0.0.1:1883", "unit-async-callbacks");

	REQUIRE(client);

	client.SetCallbacks(
		[](char* cause) {},
		[](PMPC::AsyncString&& topic, PMPC::AsyncMessage&& message) { return 1; },
		[](MQTTAsync_token token) {});

	client.SetConnected([](char* cause) {});
	client.SetDisconnected([](MQTTProperties* properties, MQTTReasonCodes reasonCode) {});
}

TEST(AsyncClient_ConnectWithoutValidHandleFailsSynchronously)
{
	PMPC::MQPPAsync client("foo://127.0.0.1:1883", "unit-async-bad-proto");

	if (client)
	{
		testfw::Skip("client creation unexpectedly succeeded for unsupported protocol");
		return;
	}

	PMPC::AsyncConnectOptions opt(
		[](MQTTAsync_successData*) {},
		[](MQTTAsync_failureData*) {});

	///synchronous failure: the options object reclaims its callback adapter on
	///destruction - no crash, no leak
	CHECK(client.Connect(opt) != MQTTASYNC_SUCCESS);
}

TEST(AsyncClient_ConnectToClosedPortFiresOnFailure)
{
	PMPC::MQPPAsync client(DEAD_SERVER, "unit-async-dead-port");

	REQUIRE(client);

	std::promise<int> codePromise;
	auto codeFuture = codePromise.get_future();

	PMPC::AsyncConnectOptions opt(
		[&codePromise](MQTTAsync_successData*) { codePromise.set_value(0); },
		[&codePromise](MQTTAsync_failureData* response) { codePromise.set_value(response ? response->code : 0); });

	opt.SetConnectTimeout(3);

	CHECK_EQ(client.Connect(opt), MQTTASYNC_SUCCESS);

	///the one-shot callback adapter is owned by paho from here on and
	///self-deletes when the failure fires
	REQUIRE(codeFuture.wait_for(std::chrono::seconds(60)) == std::future_status::ready);
	CHECK(codeFuture.get() != MQTTASYNC_SUCCESS);
}

TEST(AsyncClient_TraceCallbackStored)
{
	PMPC::MQPPAsync::SetTraceCallback([](MQTTASYNC_TRACE_LEVELS level, char* message) {});
	PMPC::MQPPAsync::SetTraceLevel(MQTTASYNC_TRACE_PROTOCOL);

	CHECK(PMPC::MQPPAsync::GetCbTrace());
}

TEST(SyncClient_CreateSucceedsWithValidURI)
{
	PMPC::MQPPSync client("tcp://127.0.0.1:1883", "unit-sync-create");

	CHECK(client);
}

TEST(SyncClient_CreateRejectsUnsupportedProtocol)
{
	PMPC::MQPPSync client("foo://127.0.0.1:1883", "unit-sync-bad-proto");

	CHECK(!client);
}

TEST(SyncClient_ConnectToClosedPortFails)
{
	PMPC::MQPPSync client(DEAD_SERVER, "unit-sync-dead-port");

	REQUIRE(client);
	CHECK_EQ(client.IsConnected(), 0);

	PMPC::SyncConnectOptions opt;
	opt.SetConnectTimeout(2);

	CHECK(client.Connect(opt) != MQTTCLIENT_SUCCESS);
	CHECK_EQ(client.IsConnected(), 0);
}

TEST(SyncClient_SetCallbacksThenDestroy)
{
	PMPC::MQPPSync client("tcp://127.0.0.1:1883", "unit-sync-callbacks");

	REQUIRE(client);

	client.SetCallbacks(
		[](char* cause) {},
		[](PMPC::SyncString&& topic, PMPC::SyncMessage&& message) { return 1; },
		[](MQTTClient_deliveryToken token) {});

	client.SetDisconnected([](MQTTProperties* properties, MQTTReasonCodes reasonCode) {});
	client.SetPublished([](int dt, int packet_type, MQTTProperties* properties, MQTTReasonCodes reasonCode) {});
}

///full round-trip against a real broker; only runs when MQTT_TEST_BROKER is set
///(e.g. MQTT_TEST_BROKER=tcp://broker-cn.emqx.io:1883)
TEST(Integration_AsyncPublishSubscribeRoundtrip)
{
	const char* broker = std::getenv("MQTT_TEST_BROKER");
	if (broker == nullptr || *broker == '\0')
	{
		testfw::Skip("MQTT_TEST_BROKER not set");
		return;
	}

	const std::string topic = "paho-mqtt-cpp/unit/test";
	const std::string payload = "unit-test-payload";

	PMPC::MQPPAsync client(broker, "paho-mqtt-cpp-unit");

	REQUIRE(client);

	std::promise<void> connected;
	std::promise<void> subscribed;
	std::promise<void> received;
	auto connectedFuture = connected.get_future();
	auto subscribedFuture = subscribed.get_future();
	auto receivedFuture = received.get_future();

	std::atomic<bool> messageOk(false);
	std::atomic<bool> receivedSet(false);

	client.SetCallbacks(
		[](char* cause) {},
		[&](PMPC::AsyncString&& topicName, PMPC::AsyncMessage&& message)
		{
			if (topicName.size() == (int)topic.size()
				&& memcmp(topicName.data(), topic.data(), topic.size()) == 0
				&& message.PayloadLen() == (int)payload.size()
				&& memcmp(message.Payload(), payload.data(), payload.size()) == 0)
			{
				messageOk = true;
			}

			if (!receivedSet.exchange(true))
			{
				received.set_value();
			}

			return 1;
		},
		[](MQTTAsync_token token) {});

	PMPC::AsyncConnectOptions opt(
		[&](MQTTAsync_successData*) { connected.set_value(); },
		[&](MQTTAsync_failureData*) { connected.set_exception(std::make_exception_ptr(std::runtime_error("connect failed"))); });

	opt.SetConnectTimeout(10);

	CHECK_EQ(client.Connect(opt), MQTTASYNC_SUCCESS);

	REQUIRE(connectedFuture.wait_for(std::chrono::seconds(30)) == std::future_status::ready);

	bool connectOk = false;
	try
	{
		connectedFuture.get();
		connectOk = true;
	}
	catch (const std::exception&)
	{
		CHECK(false && "connect failed");
	}
	REQUIRE(connectOk);

	PMPC::AsyncResponseOptions subscribeResponse(
		[&](MQTTAsync_successData*) { subscribed.set_value(); },
		[](MQTTAsync_failureData*) {});

	CHECK_EQ(client.Subscribe(topic.c_str(), 1, subscribeResponse), MQTTASYNC_SUCCESS);
	REQUIRE(subscribedFuture.wait_for(std::chrono::seconds(30)) == std::future_status::ready);

	PMPC::AsyncMessage message;
	message.SetQOS(1);
	message.SetPayload(const_cast<char*>(payload.data()), (int)payload.size());

	CHECK_EQ(client.PublishMessage(topic.c_str(), message), MQTTASYNC_SUCCESS);
	REQUIRE(receivedFuture.wait_for(std::chrono::seconds(30)) == std::future_status::ready);
	CHECK(messageOk);

	PMPC::AsyncDisconnectOptions disconnectOptions;
	CHECK_EQ(client.Disconnect(disconnectOptions), MQTTASYNC_SUCCESS);
}
