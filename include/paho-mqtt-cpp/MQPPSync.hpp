#pragma once

#include "MQPPCommon.hpp"
#include "MQTTClient.h"
#include <cstring>
#include <functional>
#include <memory>


namespace PMPC				///Paho - MQTT CPP Cient
{
	class SyncString
	{
		char* m_self;
		int m_len;
	public:
		///paho passes topicLen=0 when the topic has no embedded NUL (see MQTTClient.h on
		///MQTTClient_messageArrived); fall back to strlen in that case
		SyncString(char* self, int len)
			: m_self(self)
			, m_len((self && len == 0) ? (int)strlen(self) : len)
		{}

		SyncString(SyncString&& other)
			: m_self(std::move(other.m_self))
			, m_len(std::move(other.m_len))
		{
			other.m_self = nullptr;
			other.m_len = 0;
		}

		~SyncString()
		{
			MQTTClient_free(m_self);
		}

		operator char* ()
		{
			return m_self;
		}

		char* data() const
		{
			return m_self;
		}

		int size() const
		{
			return m_len;
		}

	};

	template<typename TCHAR>
	std::basic_ostream<TCHAR, std::char_traits<TCHAR> >& operator <<
		(std::basic_ostream<TCHAR, std::char_traits<TCHAR> >& os, const SyncString& str)
	{
		os.write(str.data(), str.size());
		return os;
	}

	class SyncSSLOptions
	{
		MQTTClient_SSLOptions m_self;

	public:
		SyncSSLOptions()
			: m_self(MQTTClient_SSLOptions_initializer)
		{}

		operator MQTTClient_SSLOptions* ()
		{
			return &m_self;
		}

		void SetTrustStore(const char* trustStore)
		{
			m_self.trustStore = trustStore;
		}

		void SetKeyStore(const char* keyStore)
		{
			m_self.keyStore = keyStore;
		}

		void SetPrivateKey(const char* privateKey)
		{
			m_self.privateKey = privateKey;
		}

		void SetPrivateKeyPassword(const char* privateKeyPassword)
		{
			m_self.privateKeyPassword = privateKeyPassword;
		}

		void SetEnabledCipherSuites(const char* enabledCipherSuites)
		{
			m_self.enabledCipherSuites = enabledCipherSuites;
		}

		void SetEnableServerCertAuth(int enableServerCertAuth)
		{
			m_self.enableServerCertAuth = enableServerCertAuth;
		}

		void SetSSLVersion(int sslVersion)
		{
			m_self.sslVersion = sslVersion;
		}

		void SetVerify(int verify)
		{
			m_self.verify = verify;
		}

		void SetCApath(const char* CApath)
		{
			m_self.CApath = CApath;
		}

		typedef std::function<int(const char* str, size_t len)> CbSSLError;
		typedef std::function<unsigned int(const char* hint, char* identity, unsigned int max_identity_len, unsigned char* psk, unsigned int max_psk_len)> CbSSLPSK;

		void SetSSLErrorCallback(CbSSLError* cb)
		{
			m_self.ssl_error_cb = [](const char* str, size_t len, void* u) {
				return (*(CbSSLError*)u)(str, len);
			};
			m_self.ssl_error_context = cb;
		}

		void SetPSKCallback(CbSSLPSK* cb)
		{
			m_self.ssl_psk_cb = [](const char* hint, char* identity, unsigned int max_identity_len, unsigned char* psk, unsigned int max_psk_len, void* u) {
				return (*(CbSSLPSK*)u)(hint, identity, max_identity_len, psk, max_psk_len);
			};
			m_self.ssl_psk_context = cb;
		}

		void SetDisableDefaultTrustStore(int disableDefaultTrustStore)
		{
			m_self.disableDefaultTrustStore = disableDefaultTrustStore;
		}

		void SetProtos(const unsigned char* protos, unsigned int protos_len)
		{
			m_self.protos = protos;
			m_self.protos_len = protos_len;
		}
	};

	class SyncWillOptions
	{
		MQTTClient_willOptions m_self;

	public:
		SyncWillOptions()
			: m_self(MQTTClient_willOptions_initializer)
		{}

		operator MQTTClient_willOptions* ()
		{
			return &m_self;
		}

		void SetTopicName(const char* topicName)
		{
			m_self.topicName = topicName;
		}

		void SetMessage(const char* message)
		{
			m_self.message = message;
		}

		void SetRetained(int retained)
		{
			m_self.retained = retained;
		}

		void SetQOS(int qos)
		{
			m_self.qos = qos;
		}

		void SetPayload(const void* data, int len)
		{
			m_self.payload.data = data;
			m_self.payload.len = len;
		}
	};

	class SyncConnectOptions
	{
		MQTTClient_connectOptions m_self;

	public:
		SyncConnectOptions(bool mqtt5 = false)
		{
			if (mqtt5)
				m_self = MQTTClient_connectOptions_initializer5;
			else
				m_self = MQTTClient_connectOptions_initializer;
		}

		operator MQTTClient_connectOptions* () const
		{
			return (MQTTClient_connectOptions*)&m_self;
		}

		void SetUserPass(const char* username, const char* password)
		{
			m_self.username = username;
			m_self.password = password;
		}

		void SetKeepAliveInterval(int keepAliveInterval)
		{
			m_self.keepAliveInterval = keepAliveInterval;
		}

		void SetCleansession(int cleansession)
		{
			m_self.cleansession = cleansession;
		}

		void SetReliable(int reliable)
		{
			m_self.reliable = reliable;
		}

		void SetConnectTimeout(int connectTimeout)
		{
			m_self.connectTimeout = connectTimeout;
		}

		void SetRetryInterval(int retryInterval)
		{
			m_self.retryInterval = retryInterval;
		}

		void SetSSL(SyncSSLOptions& ssl)
		{
			m_self.ssl = ssl;
		}

		void SetWill(SyncWillOptions& will)
		{
			m_self.will = will;
		}

		void SetMQTTVersion(int mqttVersion)
		{
			m_self.MQTTVersion = mqttVersion;
		}

		void SetServerURIs(int count, char* const* serverURIs)
		{
			m_self.serverURIcount = count;
			m_self.serverURIs = serverURIs;
		}

		void SetMaxInflightMessages(int maxInflightMessages)
		{
			m_self.maxInflightMessages = maxInflightMessages;
		}

		void SetCleanstart(int cleanstart)
		{
			m_self.cleanstart = cleanstart;
		}
	};

	class SyncMessage
	{
		std::shared_ptr<MQTTClient_message> m_self;
	public:
		SyncMessage()
			: m_self((MQTTClient_message*)MQTTClient_malloc(sizeof(MQTTClient_message)), MQTTClient_free)
		{
			*m_self = MQTTClient_message_initializer;
		}

		SyncMessage(MQTTClient_message* self)
			: m_self(self, [](MQTTClient_message* message) { MQTTClient_freeMessage(&message); })
		{
		}

		~SyncMessage()
		{
		}

		operator MQTTClient_message*() const
		{
			return m_self.get();
		}

		void SetPayload(void* payload, int payloadlen)
		{
			m_self->payload = payload;
			m_self->payloadlen = payloadlen;
		}

		void SetQOS(int qos)
		{
			m_self->qos = qos;
		}

		void SetRetained(int retained)
		{
			m_self->retained = retained;
		}

		void* Payload() const
		{
			return m_self->payload;
		}

		int PayloadLen() const
		{
			return m_self->payloadlen;
		}
	};

	class MQPPSync
	{
	public:
		typedef std::function<int(SyncString&& topicName, SyncMessage&& message)> CbMessageArrived;
		typedef std::function<void(MQTTClient_deliveryToken token)> CbDeliveryComplete;
		typedef std::function<void(char* cause)> CbConnectionLost;
		typedef std::function<void(MQTTProperties* properties, MQTTReasonCodes reasonCode)> CbDisconnected;
		typedef std::function<void(int dt, int packet_type, MQTTProperties* properties, MQTTReasonCodes reasonCode)> CbPublished;

	private:
		MQTTClient m_self;
		CbConnectionLost m_cbConnectionLost;
		CbMessageArrived m_cbMessageArrived;
		CbDeliveryComplete m_cbDeliveryComplete;
		CbDisconnected m_cbDisconnected;
		CbPublished m_cbPublished;

	public:
		MQPPSync(const char* serverURI, const char* clientId, PersistenceType persistence_type = PT_PERSISTENCE_NONE, void* persistence_context = nullptr, MQTTClient_createOptions* options = nullptr)
			: m_self(nullptr)
		{
			if (options)
			{
				MQTTClient_createWithOptions(&m_self, serverURI, clientId, persistence_type, persistence_context, options);
			}
			else
			{
				MQTTClient_create(&m_self, serverURI, clientId, persistence_type, persistence_context);
			}
		}

		~MQPPSync()
		{
			MQTTClient_destroy(&m_self);
		}

		///copy and move deleted: the context registered by the SetCallbacks family is the
		///object's own (or a member's) address; after move/copy paho would still hold the
		///stale address, dangling on destruction
		MQPPSync(const MQPPSync&) = delete;
		MQPPSync& operator=(const MQPPSync&) = delete;
		MQPPSync(MQPPSync&&) = delete;
		MQPPSync& operator=(MQPPSync&&) = delete;

		operator bool() const
		{
			return !!m_self;
		}

		int SetDisconnected(CbDisconnected&& cbDisconnected)
		{
			m_cbDisconnected = std::move(cbDisconnected);

			return MQTTClient_setDisconnected(m_self, &m_cbDisconnected,
				[](void* context, MQTTProperties* properties, enum MQTTReasonCodes reasonCode)
			{
				if (context)
				{
					auto& cb = (*(CbDisconnected*)context);
					if (cb) cb(properties, reasonCode);
				}
			});
		}

		int SetPublished(CbPublished&& cbPublished)
		{
			m_cbPublished = std::move(cbPublished);

			return MQTTClient_setPublished(m_self, &m_cbPublished,
				[](void* context, int dt, int packet_type, MQTTProperties* properties, enum MQTTReasonCodes reasonCode)
			{
				if (context)
				{
					auto& cb = (*(CbPublished*)context);
					if (cb) cb(dt, packet_type, properties, reasonCode);
				}
			});
		}

		int SetCallbacks(CbConnectionLost&& cbConnectionLost, CbMessageArrived&& cbMessageArrived, CbDeliveryComplete&& cbDeliveryComplete)
		{
			m_cbConnectionLost = std::move(cbConnectionLost);
			m_cbMessageArrived = std::move(cbMessageArrived);
			m_cbDeliveryComplete = std::move(cbDeliveryComplete);

			return MQTTClient_setCallbacks(m_self, this,
				[](void* context, char* cause)
			{
				auto self = (MQPPSync*)context;
				if (self && self->m_cbConnectionLost)
				{
					self->m_cbConnectionLost(cause);
				}

			}, [](void* context, char* topicName, int topicLen, MQTTClient_message* message)
			{
				auto self = (MQPPSync*)context;
				if (self && self->m_cbMessageArrived)
				{
					return self->m_cbMessageArrived(SyncString(topicName, topicLen), message);
				}
				///TRUE: message accepted, paho frees it normally
				return 1;

			}, [](void* context, MQTTClient_deliveryToken dt)
			{
				auto self = (MQPPSync*)context;
				if (self && self->m_cbDeliveryComplete)
				{
					self->m_cbDeliveryComplete(dt);
				}
			});
		}

		int Connect(const SyncConnectOptions& options)
		{
			return MQTTClient_connect(m_self, options);
		}

		MQTTResponse Connect5(const SyncConnectOptions& options, MQTTProperties* connectProperties = nullptr, MQTTProperties* willProperties = nullptr)
		{
			return MQTTClient_connect5(m_self, options, connectProperties, willProperties);
		}

		int Disconnect(int timeout)
		{
			return MQTTClient_disconnect(m_self, timeout);
		}

		int Disconnect5(int timeout, MQTTReasonCodes reason, MQTTProperties* props = nullptr)
		{
			return MQTTClient_disconnect5(m_self, timeout, reason, props);
		}

		int IsConnected()
		{
			return MQTTClient_isConnected(m_self);
		}

		int Subscribe(const char* topic, int qos)
		{
			return MQTTClient_subscribe(m_self, topic, qos);
		}

		MQTTResponse Subscribe5(const char* topic, int qos, MQTTSubscribe_options* opts = nullptr, MQTTProperties* props = nullptr)
		{
			return MQTTClient_subscribe5(m_self, topic, qos, opts, props);
		}

		int SubscribeMany(int count, char* const* topic, int* qos)
		{
			return MQTTClient_subscribeMany(m_self, count, topic, qos);
		}

		MQTTResponse SubscribeMany5(int count, char* const* topic, int* qos, MQTTSubscribe_options* opts = nullptr, MQTTProperties* props = nullptr)
		{
			return MQTTClient_subscribeMany5(m_self, count, topic, qos, opts, props);
		}

		int Unsubscribe(const char* topic)
		{
			return MQTTClient_unsubscribe(m_self, topic);
		}

		MQTTResponse Unsubscribe5(const char* topic, MQTTProperties* props = nullptr)
		{
			return MQTTClient_unsubscribe5(m_self, topic, props);
		}

		int UnsubscribeMany(int count, char* const* topic)
		{
			return MQTTClient_unsubscribeMany(m_self, count, topic);
		}

		MQTTResponse UnsubscribeMany5(int count, char* const* topic, MQTTProperties* props = nullptr)
		{
			return MQTTClient_unsubscribeMany5(m_self, count, topic, props);
		}

		int Publish(const char* topicName, int payloadlen, const void* payload, int qos, int retained, MQTTClient_deliveryToken* dt = nullptr)
		{
			return MQTTClient_publish(m_self, topicName, payloadlen, payload, qos, retained, dt);
		}

		MQTTResponse Publish5(const char* topicName, int payloadlen, const void* payload, int qos, int retained, MQTTProperties* properties = nullptr, MQTTClient_deliveryToken* dt = nullptr)
		{
			return MQTTClient_publish5(m_self, topicName, payloadlen, payload, qos, retained, properties, dt);
		}

		int PublishMessage(const char* topicName, const SyncMessage& msg, MQTTClient_deliveryToken* dt = nullptr)
		{
			return MQTTClient_publishMessage(m_self, topicName, msg, dt);
		}

		MQTTResponse PublishMessage5(const char* topicName, const SyncMessage& msg, MQTTClient_deliveryToken* dt = nullptr)
		{
			return MQTTClient_publishMessage5(m_self, topicName, msg, dt);
		}

		int WaitForCompletion(MQTTClient_deliveryToken dt, unsigned long timeout)
		{
			return MQTTClient_waitForCompletion(m_self, dt, timeout);
		}

		int GetPendingDeliveryTokens(MQTTClient_deliveryToken** tokens)
		{
			return MQTTClient_getPendingDeliveryTokens(m_self, tokens);
		}

		int Receive(char** topicName, int* topicLen, MQTTClient_message** message, unsigned long timeout)
		{
			return MQTTClient_receive(m_self, topicName, topicLen, message, timeout);
		}

		static void ThreadYield()
		{
			MQTTClient_yield();
		}

		int SetCommandTimeout(unsigned long milliSeconds)
		{
			return MQTTClient_setCommandTimeout(m_self, milliSeconds);
		}

		static void SetTraceLevel(enum MQTTCLIENT_TRACE_LEVELS level)
		{
			MQTTClient_setTraceLevel(level);
		}

		typedef std::function<void(enum MQTTCLIENT_TRACE_LEVELS level, char* message)> CbTrace;

		static CbTrace& GetCbTrace()
		{
			static CbTrace s_cbTrace;
			return s_cbTrace;
		}

		static void SetTraceCallback(CbTrace&& cbTrace)
		{
			MQPPSync::GetCbTrace() = std::move(cbTrace);

			MQTTClient_setTraceCallback([](enum MQTTCLIENT_TRACE_LEVELS level, char* message)
			{
				auto& cb = MQPPSync::GetCbTrace();
				if (cb) cb(level, message);
			});
		}

	};
}
