#include "test_framework.hpp"

#include <paho-mqtt-cpp/MQPPAsync.hpp>
#include <paho-mqtt-cpp/MQPPSync.hpp>

#include <cstring>

TEST(AsyncMessage_DefaultStateAndSetters)
{
	PMPC::AsyncMessage msg;
	char payload[] = "test";

	CHECK_EQ(msg.PayloadLen(), 0);

	msg.SetPayload(payload, 4);
	msg.SetQOS(2);
	msg.SetRetained(1);

	CHECK(msg.Payload() == payload);
	CHECK_EQ(msg.PayloadLen(), 4);

	const MQTTAsync_message* raw = (MQTTAsync_message*)msg;
	CHECK_EQ(raw->qos, 2);
	CHECK_EQ(raw->retained, 1);
}

TEST(AsyncMessage_OwnsCallbackDeliveredMessage)
{
	///messageArrived hands over ownership: it is freed with MQTTAsync_freeMessage
	///(payload included) when the RAII wrapper goes out of scope
	MQTTAsync_message* raw = (MQTTAsync_message*)MQTTAsync_malloc(sizeof(MQTTAsync_message));
	*raw = MQTTAsync_message_initializer;
	raw->payload = MQTTAsync_malloc(4);
	memcpy(raw->payload, "test", 4);
	raw->payloadlen = 4;

	{
		PMPC::AsyncMessage msg(raw);
		CHECK_EQ(msg.PayloadLen(), 4);
	}
}

TEST(AsyncMessage_SharedOwnershipAcrossCopies)
{
	PMPC::AsyncMessage msg;
	msg.SetQOS(1);

	PMPC::AsyncMessage copy = msg;

	CHECK_EQ(((MQTTAsync_message*)copy)->qos, 1);
}

TEST(SyncMessage_DefaultStateAndSetters)
{
	PMPC::SyncMessage msg;
	char payload[] = "test";

	CHECK_EQ(msg.PayloadLen(), 0);

	msg.SetPayload(payload, 4);
	msg.SetQOS(2);
	msg.SetRetained(1);

	CHECK(msg.Payload() == payload);
	CHECK_EQ(msg.PayloadLen(), 4);

	const MQTTClient_message* raw = (MQTTClient_message*)msg;
	CHECK_EQ(raw->qos, 2);
	CHECK_EQ(raw->retained, 1);
}

TEST(SyncMessage_OwnsCallbackDeliveredMessage)
{
	MQTTClient_message* raw = (MQTTClient_message*)MQTTClient_malloc(sizeof(MQTTClient_message));
	*raw = MQTTClient_message_initializer;
	raw->payload = MQTTClient_malloc(4);
	memcpy(raw->payload, "test", 4);
	raw->payloadlen = 4;

	{
		PMPC::SyncMessage msg(raw);
		CHECK_EQ(msg.PayloadLen(), 4);
	}
}

TEST(SyncMessage_SharedOwnershipAcrossCopies)
{
	PMPC::SyncMessage msg;
	msg.SetQOS(1);

	PMPC::SyncMessage copy = msg;

	CHECK_EQ(((MQTTClient_message*)copy)->qos, 1);
}
