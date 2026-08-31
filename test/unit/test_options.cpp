#include "test_framework.hpp"

#include <paho-mqtt-cpp/MQPPAsync.hpp>
#include <paho-mqtt-cpp/MQPPSync.hpp>

#include <string>

TEST(AsyncConnectOptions_SettersWriteThroughToStruct)
{
	PMPC::AsyncConnectOptions opt(
		[](MQTTAsync_successData*) {},
		[](MQTTAsync_failureData*) {});

	opt.SetUserPass("user", "pass");
	opt.SetAutomaticReconnect(true);
	opt.SetRetryInterval(5);
	opt.SetReconnectRetryInterval(1, 60);
	opt.SetConnectTimeout(10);
	opt.SetKeepAliveInterval(30);

	const MQTTAsync_connectOptions* raw = (MQTTAsync_connectOptions*)opt;

	CHECK_EQ(std::string(raw->username), std::string("user"));
	CHECK_EQ(std::string(raw->password), std::string("pass"));
	CHECK_EQ(raw->automaticReconnect, 1);
	CHECK_EQ(raw->retryInterval, 5);
	CHECK_EQ(raw->minRetryInterval, 1);
	CHECK_EQ(raw->maxRetryInterval, 60);
	CHECK_EQ(raw->connectTimeout, 10);
	CHECK_EQ(raw->keepAliveInterval, 30);
}

TEST(AsyncConnectOptions_InstallsCallbacksWhenProvided)
{
	PMPC::AsyncConnectOptions opt(
		[](MQTTAsync_successData*) {},
		[](MQTTAsync_failureData*) {});

	const MQTTAsync_connectOptions* raw = (MQTTAsync_connectOptions*)opt;

	CHECK(raw->onSuccess != nullptr);
	CHECK(raw->onFailure != nullptr);
	CHECK(raw->context != nullptr);
}

TEST(AsyncResponseOptions_WithoutCallbacksInstallsNothing)
{
	///no adapter is created when no callback is given: paho would crash
	///invoking callbacks with a NULL context
	PMPC::AsyncResponseOptions opt;

	const MQTTAsync_responseOptions* raw = (MQTTAsync_responseOptions*)opt;

	CHECK(raw->onSuccess == nullptr);
	CHECK(raw->onFailure == nullptr);
	CHECK(raw->context == nullptr);
}

TEST(AsyncResponseOptions_WithCallbacksInstallsAdapter)
{
	PMPC::AsyncResponseOptions opt(
		[](MQTTAsync_successData*) {},
		[](MQTTAsync_failureData*) {});

	const MQTTAsync_responseOptions* raw = (MQTTAsync_responseOptions*)opt;

	CHECK(raw->onSuccess != nullptr);
	CHECK(raw->onFailure != nullptr);
	CHECK(raw->context != nullptr);
}

TEST(AsyncDisconnectOptions_WithCallbacksInstallsAdapter)
{
	PMPC::AsyncDisconnectOptions opt(
		[](MQTTAsync_successData*) {},
		[](MQTTAsync_failureData*) {});

	const MQTTAsync_disconnectOptions* raw = (MQTTAsync_disconnectOptions*)opt;

	CHECK(raw->onSuccess != nullptr);
	CHECK(raw->onFailure != nullptr);
	CHECK(raw->context != nullptr);
}

TEST(AsyncWillOptions_Setters)
{
	PMPC::AsyncWillOptions will;
	char payload[] = "bye";

	will.SetTopicName("will/topic");
	will.SetMessage("gone");
	will.SetQOS(1);
	will.SetRetained(1);
	will.SetPayload(payload, (int)sizeof(payload));

	const MQTTAsync_willOptions* raw = (MQTTAsync_willOptions*)will;

	CHECK_EQ(std::string(raw->topicName), std::string("will/topic"));
	CHECK_EQ(std::string(raw->message), std::string("gone"));
	CHECK_EQ(raw->qos, 1);
	CHECK_EQ(raw->retained, 1);
	CHECK(raw->payload.data == payload);
	CHECK_EQ(raw->payload.len, (int)sizeof(payload));
}

TEST(AsyncSSLOptions_Setters)
{
	PMPC::AsyncSSLOptions ssl;

	ssl.SetTrustStore("cacert.pem");
	ssl.SetKeyStore("client.pem");
	ssl.SetPrivateKey("key.pem");
	ssl.SetPrivateKeyPassword("secret");
	ssl.SetEnableServerCertAuth(0);
	ssl.SetVerify(1);

	const MQTTAsync_SSLOptions* raw = (MQTTAsync_SSLOptions*)ssl;

	CHECK_EQ(std::string(raw->trustStore), std::string("cacert.pem"));
	CHECK_EQ(std::string(raw->keyStore), std::string("client.pem"));
	CHECK_EQ(std::string(raw->privateKey), std::string("key.pem"));
	CHECK_EQ(std::string(raw->privateKeyPassword), std::string("secret"));
	CHECK_EQ(raw->enableServerCertAuth, 0);
	CHECK_EQ(raw->verify, 1);
}

TEST(SyncConnectOptions_SettersWriteThroughToStruct)
{
	PMPC::SyncConnectOptions opt;

	opt.SetUserPass("user", "pass");
	opt.SetKeepAliveInterval(30);
	opt.SetCleansession(1);
	opt.SetConnectTimeout(10);
	opt.SetMQTTVersion(MQTTVERSION_3_1_1);
	opt.SetMaxInflightMessages(20);

	const MQTTClient_connectOptions* raw = (MQTTClient_connectOptions*)opt;

	CHECK_EQ(std::string(raw->username), std::string("user"));
	CHECK_EQ(std::string(raw->password), std::string("pass"));
	CHECK_EQ(raw->keepAliveInterval, 30);
	CHECK_EQ(raw->cleansession, 1);
	CHECK_EQ(raw->connectTimeout, 10);
	CHECK_EQ(raw->MQTTVersion, MQTTVERSION_3_1_1);
	CHECK_EQ(raw->maxInflightMessages, 20);
}

TEST(SyncConnectOptions_MQTT5Initializer)
{
	PMPC::SyncConnectOptions opt3;
	PMPC::SyncConnectOptions opt5(true);

	CHECK_EQ(((MQTTClient_connectOptions*)opt3)->MQTTVersion, MQTTVERSION_DEFAULT);
	CHECK_EQ(((MQTTClient_connectOptions*)opt5)->MQTTVersion, MQTTVERSION_5);
}

TEST(SyncWillOptions_Setters)
{
	PMPC::SyncWillOptions will;

	will.SetTopicName("will/topic");
	will.SetMessage("gone");
	will.SetQOS(1);
	will.SetRetained(1);

	const MQTTClient_willOptions* raw = (MQTTClient_willOptions*)will;

	CHECK_EQ(std::string(raw->topicName), std::string("will/topic"));
	CHECK_EQ(std::string(raw->message), std::string("gone"));
	CHECK_EQ(raw->qos, 1);
	CHECK_EQ(raw->retained, 1);
}

TEST(SyncSSLOptions_Setters)
{
	PMPC::SyncSSLOptions ssl;

	ssl.SetTrustStore("cacert.pem");
	ssl.SetEnableServerCertAuth(0);

	const MQTTClient_SSLOptions* raw = (MQTTClient_SSLOptions*)ssl;

	CHECK_EQ(std::string(raw->trustStore), std::string("cacert.pem"));
	CHECK_EQ(raw->enableServerCertAuth, 0);
}
