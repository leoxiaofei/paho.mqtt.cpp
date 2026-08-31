#include <iostream>
#include <string>
#include <paho-mqtt-cpp/MQPPAsync.hpp>

int main()
{
	MQTTAsync_token test_token;

	PMPC::MQPPAsync::SetTraceLevel(MQTTASYNC_TRACE_PROTOCOL);
	PMPC::MQPPAsync::SetTraceCallback(
		[](enum MQTTASYNC_TRACE_LEVELS level, char* message)
	{
		std::cout << level << " " << message << std::endl;
	});

	// PMPC::MQPPAsync test("tcp://127.0.0.1:1883", "MQPP_1");
	PMPC::MQPPAsync test("tcp://broker-cn.emqx.io:1883", "MQPP_MS");

#ifdef OPENSSL
	PMPC::AsyncSSLOptions ssl;
	///server cert verification skipped for debugging; configure ssl options for production
	ssl.SetEnableServerCertAuth(0);
	// ssl.SetVerify(1);
	// ssl.SetTrustStore("C:\\Users\\Administrator\\Desktop\\emqx-certs\\cacert.pem");
#endif

	test.SetCallbacks([](char* cause)
	{
		std::cout << "CbConnectionLost: " << (const char*)(cause ? cause : "") << std::endl;
	}, [](PMPC::AsyncString&& topicName, PMPC::AsyncMessage&& message)
	{
		std::cout << "CbMessageArrived: " << topicName << " p: " << (char*)message.Payload() << std::endl;

		return 1;
	}, [&](MQTTAsync_token token) {

		test_token = token;
		std::cout << "CbDeliveryComplete: " << token << std::endl;
	});

	test.SetConnected([](char* cause)
	{
		std::cout << "SetConnected: " << cause << std::endl;
	});

	test.SetDisconnected([](MQTTProperties* properties, MQTTReasonCodes reasonCode)
	{
		std::cout << "SetDisconnected: " << reasonCode << std::endl;
	});

	PMPC::AsyncConnectOptions opt([&](MQTTAsync_successData* response)
	{
		std::cout << "onSuccess: " << response->token << std::endl;
		test.Subscribe("testtopic/#", 2, PMPC::AsyncResponseOptions());
	}, [](MQTTAsync_failureData* response)
	{
		std::cout << "onFailure: " << response->message << std::endl;
	});

	opt.SetAutomaticReconnect(true);
	opt.SetRetryInterval(5);
#ifdef OPENSSL
	opt.SetSSL(ssl);
#endif

	auto rc = test.Connect(opt);
	std::cout << "Connect Result: " << rc << std::endl;

	while (true)
	{
		std::string strCmd;
		std::cin >> strCmd;
		if (strCmd == "d")
		{
			PMPC::AsyncDisconnectOptions disopt;
			test.Disconnect(disopt);
		}
		else if (strCmd == "c")
		{
			PMPC::AsyncConnectOptions opt([](MQTTAsync_successData* response)
			{
				std::cout << "onSuccess: " << response->token << std::endl;
			}, [](MQTTAsync_failureData* response)
			{
				std::cout << "onFailure: " << response->message << std::endl;
			});

			opt.SetAutomaticReconnect(true);
			opt.SetRetryInterval(5);
#ifdef OPENSSL
			opt.SetSSL(ssl);
#endif

			auto rc = test.Connect(opt);
			std::cout << "Connect Result: " << rc << std::endl;

		}
		if (strCmd == "s")
		{
			std::string strMsg;
			std::cin >> strMsg;

			test.Subscribe(strMsg.c_str(), 2, PMPC::AsyncResponseOptions());
		}
		else if(strCmd == "p")
		{
			std::string strMsg;
			std::string strTopic;
			std::cin >> strTopic >> strMsg;
			PMPC::AsyncMessage message;
			message.SetQOS(2);
			message.SetPayload(&strMsg[0], (int)strMsg.size());
			test.PublishMessage(strTopic.c_str(), message);
		}
	}

	return 0;
}
