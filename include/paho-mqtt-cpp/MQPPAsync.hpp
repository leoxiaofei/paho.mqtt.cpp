#pragma once

#include "MQPPCommon.hpp"
#include "MQTTAsync.h"
#include <atomic>
#include <cstring>
#include <functional>
#include <memory>
#include <vector>


namespace PMPC				///Paho - MQTT CPP Cient
{
	class AsyncString
	{
		char* m_self;
		int m_len;
	public:
		///paho passes topicLen=0 when the topic has no embedded NUL (see MQTTAsync.h on
		///MQTTAsync_messageArrived); fall back to strlen in that case
		AsyncString(char* self, int len)
			: m_self(self)
			, m_len((self && len == 0) ? (int)strlen(self) : len)
		{}

		AsyncString(AsyncString&& other)
			: m_self(std::move(other.m_self))
			, m_len(std::move(other.m_len))
		{
			other.m_self = nullptr;
			other.m_len = 0;
		}

		~AsyncString()
		{
			MQTTAsync_free(m_self);
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
		(std::basic_ostream<TCHAR, std::char_traits<TCHAR> >& os, const AsyncString& str)
	{
		os.write(str.data(), str.size());
		return os;
	}

	typedef std::function<void(MQTTAsync_successData* response)> CbOnSuccess;
	typedef std::function<void(MQTTAsync_successData5* response)> CbOnSuccess5;
	typedef std::function<void(MQTTAsync_failureData* response)> CbOnFailure;
	typedef std::function<void(MQTTAsync_failureData5* response)> CbOnFailure5;
	typedef std::function<void()> CbOnRelease;

	class AsRespAptBase
	{
	protected:
		bool autoRelease = true;

	public:
		virtual ~AsRespAptBase() = default;

		void SetAutoRelease(bool enable)
		{
			autoRelease = enable;
		}

		///called from paho callbacks: self-delete only for one-shot commands
		///(subscribe/publish/disconnect responses). Connect adapters disable
		///autoRelease - paho retains them in the client and re-invokes them
		///from automaticReconnect retries (see AsyncConnectOptions).
		void Release()
		{
			if (autoRelease)
			{
				delete this;
			}
		}
	};

	class AsRespApt : public AsRespAptBase
	{
	public:
		CbOnSuccess cbOnSuccess;
		CbOnFailure cbOnFailure;

	public:
		AsRespApt(CbOnSuccess&& cbOnSuccess, CbOnFailure&& cbOnFailure)
			: cbOnSuccess(std::move(cbOnSuccess))
			, cbOnFailure(std::move(cbOnFailure))
		{}

		template<class T>
		static AsRespAptBase* Attach(T& self, CbOnSuccess&& cbOnSuccess, CbOnFailure&& cbOnFailure)
		{
			AsRespApt* ret = nullptr;

			if(cbOnSuccess || cbOnFailure)
			{
				ret = new AsRespApt{ std::move(cbOnSuccess), std::move(cbOnFailure) };
				self.context = ret;

				///install callbacks only when an adapter exists: paho does not check context
				///for NULL when invoking callbacks - unconditional install with NULL context crashes
				self.onSuccess = [](void* context, MQTTAsync_successData* response)
				{
					auto aro = (AsRespApt*)context;
					if(aro->cbOnSuccess)
					{
						aro->cbOnSuccess(response);
					}
					aro->Release();
				};

				self.onFailure = [](void* context, MQTTAsync_failureData* response)
				{
					auto aro = (AsRespApt*)context;
					if(aro->cbOnFailure)
					{
						aro->cbOnFailure(response);
					}
					aro->Release();
				};
			}

			return ret;
		}
	};

	class AsRespApt5 : public AsRespAptBase
	{
	public:
		CbOnSuccess5 cbOnSuccess;
		CbOnFailure5 cbOnFailure;

	public:
		AsRespApt5(CbOnSuccess5&& cbOnSuccess, CbOnFailure5&& cbOnFailure)
			: cbOnSuccess(std::move(cbOnSuccess))
			, cbOnFailure(std::move(cbOnFailure))
		{}

		template<class T>
		static AsRespAptBase* Attach(T& self, CbOnSuccess5&& cbOnSuccess, CbOnFailure5&& cbOnFailure)
		{
			AsRespApt5* ret = nullptr;

			if(cbOnSuccess || cbOnFailure)
			{
				ret = new AsRespApt5{ std::move(cbOnSuccess), std::move(cbOnFailure) };
				self.context = ret;

				self.onSuccess5 = [](void* context, MQTTAsync_successData5* response)
				{
					auto aro = (AsRespApt5*)context;
					if(aro->cbOnSuccess)
					{
						aro->cbOnSuccess(response);
					}
					aro->Release();
				};

				self.onFailure5 = [](void* context, MQTTAsync_failureData5* response)
				{
					auto aro = (AsRespApt5*)context;
					if(aro->cbOnFailure)
					{
						aro->cbOnFailure(response);
					}
					aro->Release();
				};
			}

			return ret;
		}
	};

	class AsyncSSLOptions
	{
		MQTTAsync_SSLOptions m_self;

	public:
		AsyncSSLOptions()
			: m_self(MQTTAsync_SSLOptions_initializer)
		{}

		operator MQTTAsync_SSLOptions* ()
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

	class AsyncWillOptions
	{
		MQTTAsync_willOptions m_self;

	public:
		AsyncWillOptions()
			: m_self(MQTTAsync_willOptions_initializer)
		{}

		AsyncWillOptions(const AsyncWillOptions&) = delete;
		AsyncWillOptions& operator=(const AsyncWillOptions&) = delete;

		operator MQTTAsync_willOptions* ()
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

	///note: SetUserPass/SetSSL/SetWill store raw pointers held by paho for the whole
	///connection (also reused by automaticReconnect) - the objects must outlive the client.
	///connect callbacks are also retained by paho for the whole connection and re-invoked
	///on every automaticReconnect retry: they must only reference long-lived state.
	///after a successful submit the adapter is owned by MQPPAsync (freed in ~MQPPAsync)
	class AsyncConnectOptions
	{
		MQTTAsync_connectOptions m_self;
		mutable std::unique_ptr<AsRespAptBase> m_asRespApt;

	public:
		AsyncConnectOptions(CbOnSuccess&& cbOnSuccess, CbOnFailure&& cbOnFailure)
			: m_self(MQTTAsync_connectOptions_initializer)
			, m_asRespApt(AsRespApt::Attach(m_self, std::move(cbOnSuccess), std::move(cbOnFailure)))
		{
			keepAdapterForReconnects();
		}

		AsyncConnectOptions(CbOnSuccess5&& cbOnSuccess, CbOnFailure5&& cbOnFailure)
			: m_self(MQTTAsync_connectOptions_initializer5)
			, m_asRespApt(AsRespApt5::Attach(m_self, std::move(cbOnSuccess), std::move(cbOnFailure)))
		{
			keepAdapterForReconnects();
		}

		AsyncConnectOptions(const AsyncConnectOptions&) = delete;
		AsyncConnectOptions& operator=(const AsyncConnectOptions&) = delete;

		operator MQTTAsync_connectOptions* () const
		{
			return (MQTTAsync_connectOptions*)&m_self;
		}

		void SetUserPass(const char* username, const char* password)
		{
			m_self.username = username;
			m_self.password = password;
		}

		void SetAutomaticReconnect(bool automaticReconnect)
		{
			m_self.automaticReconnect = automaticReconnect ? 1 : 0;
		}

		///seconds. NOTE: in-session retry interval for unacknowledged QoS>0 publishes
		///(per MQTTAsync.h), unrelated to automatic reconnect
		void SetRetryInterval(int retryInterval)
		{
			m_self.retryInterval = retryInterval;
		}

		///automatic reconnect backoff (seconds): first retry after minRetryInterval, doubling on
		///each failure, capped at maxRetryInterval (+-20% jitter)
		void SetReconnectRetryInterval(int minRetryInterval, int maxRetryInterval)
		{
			m_self.minRetryInterval = minRetryInterval;
			m_self.maxRetryInterval = maxRetryInterval;
		}

		///seconds
		void SetConnectTimeout(int connectTimeout)
		{
			m_self.connectTimeout = connectTimeout;
		}

		///seconds
		void SetKeepAliveInterval(int keepAliveInterval)
		{
			m_self.keepAliveInterval = keepAliveInterval;
		}

		void SetSSL(AsyncSSLOptions& ssl)
		{
			m_self.ssl = ssl;
		}

		void SetWill(AsyncWillOptions& will)
		{
			m_self.will = will;
		}

		///internal, used by MQPPAsync: transfers ownership of the adapter out (the
		///source is left null, so the destructor will not free it). not called on
		///synchronous submit failure: ownership stays local, reclaimed by the
		///destructor, and the same object may be submitted again
		std::unique_ptr<AsRespAptBase> Detach() const
		{
			return std::move(m_asRespApt);
		}

	private:
		///paho stores the connect callbacks in the client and re-invokes them on every
		///automaticReconnect retry (and on a retry that eventually succeeds), so the
		///adapter must not self-delete after the first callback - MQPPAsync owns it
		void keepAdapterForReconnects()
		{
			if (m_asRespApt)
			{
				m_asRespApt->SetAutoRelease(false);
			}
		}
	};

	///options with callbacks are one-shot: never resubmit after a successful submit
	class AsyncDisconnectOptions
	{
		MQTTAsync_disconnectOptions m_self;
		mutable std::unique_ptr<AsRespAptBase> m_asRespApt;
	public:
		AsyncDisconnectOptions()
			: m_self(MQTTAsync_disconnectOptions_initializer)
		{
		}

		AsyncDisconnectOptions(CbOnSuccess&& cbOnSuccess, CbOnFailure&& cbOnFailure)
			: m_self(MQTTAsync_disconnectOptions_initializer)
			, m_asRespApt(AsRespApt::Attach(m_self, std::move(cbOnSuccess), std::move(cbOnFailure)))
		{
		}

		AsyncDisconnectOptions(CbOnSuccess5&& cbOnSuccess, CbOnFailure5&& cbOnFailure)
			: m_self(MQTTAsync_disconnectOptions_initializer5)
			, m_asRespApt(AsRespApt5::Attach(m_self, std::move(cbOnSuccess), std::move(cbOnFailure)))
		{
		}

		AsyncDisconnectOptions(const AsyncDisconnectOptions&) = delete;
		AsyncDisconnectOptions& operator=(const AsyncDisconnectOptions&) = delete;

		operator MQTTAsync_disconnectOptions* () const
		{
			return (MQTTAsync_disconnectOptions*)&m_self;
		}

		///internal, used by MQPPAsync: after a successful submit the adapter is owned by paho
		///(self-deleted in the callback); release without deleting. not called on synchronous
		///submit failure: ownership stays local, reclaimed by the destructor, and the same
		///object may be submitted again
		void Detach() const
		{
			m_asRespApt.release();
		}
	};

	///options with callbacks are one-shot: never resubmit after a successful submit
	class AsyncResponseOptions
	{
	private:
		MQTTAsync_responseOptions m_self;
		mutable std::unique_ptr<AsRespAptBase> m_asRespApt;

	public:
		AsyncResponseOptions()
			: m_self(MQTTAsync_responseOptions_initializer)
		{

		}

		AsyncResponseOptions(CbOnSuccess&& cbOnSuccess, CbOnFailure&& cbOnFailure)
			: m_self(MQTTAsync_responseOptions_initializer)
			, m_asRespApt(AsRespApt::Attach(m_self, std::move(cbOnSuccess), std::move(cbOnFailure)))
		{
		}

		AsyncResponseOptions(CbOnSuccess5&& cbOnSuccess, CbOnFailure5&& cbOnFailure)
			: m_self(MQTTAsync_responseOptions_initializer)
			, m_asRespApt(AsRespApt5::Attach(m_self, std::move(cbOnSuccess), std::move(cbOnFailure)))
		{
		}

		AsyncResponseOptions(const AsyncResponseOptions&) = delete;
		AsyncResponseOptions& operator=(const AsyncResponseOptions&) = delete;

		operator MQTTAsync_responseOptions*() const
		{
			return (MQTTAsync_responseOptions*)&m_self;
		}

		MQTTAsync_token GetToken() const
		{
			return m_self.token;
		}

		///internal, used by MQPPAsync: after a successful submit the adapter is owned by paho
		///(self-deleted in the callback); release without deleting. not called on synchronous
		///submit failure: ownership stays local, reclaimed by the destructor, and the same
		///object may be submitted again
		void Detach() const
		{
			m_asRespApt.release();
		}
	};

	class AsyncMessage
	{
		std::shared_ptr<MQTTAsync_message> m_self;
	public:
		AsyncMessage()
			: m_self((MQTTAsync_message*)MQTTAsync_malloc(sizeof(MQTTAsync_message)), MQTTAsync_free)
		{
			*m_self = MQTTAsync_message_initializer;
		}

		AsyncMessage(MQTTAsync_message* self)
			: m_self(self, [](MQTTAsync_message* message) { MQTTAsync_freeMessage(&message); })
		{
		}

		~AsyncMessage()
		{
		}

		operator MQTTAsync_message*() const
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

	class MQPPAsync
	{	
	public:
		typedef std::function<int(AsyncString&& topicName, AsyncMessage&& message)> CbMessageArrived;
		typedef std::function<void(MQTTAsync_token token)> CbDeliveryComplete;
		typedef std::function<void(char* cause)> CbConnectionLost;
		typedef std::function<void(char* cause)> CbConnected;
		typedef std::function<void(MQTTProperties* properties, MQTTReasonCodes reasonCode) > CbDisconnected;
		typedef std::function<int(MQTTAsync_connectData* data)> CbUpdateConnectOptions;
		typedef std::function<int(int bufcount, char* buffers[], int buflens[])> CbBeforeWrite;
		typedef std::function<int(char** buffer, int* buflen)> CbAfterRead;

	private:
		MQTTAsync m_self;
		CbConnected m_cbConnected;
		CbConnectionLost m_cbConnectionLost;
		CbDisconnected m_cbDisconnected;
		CbMessageArrived m_cbMessageArrived;
		CbDeliveryComplete m_cbDeliveryComplete;
		CbUpdateConnectOptions m_cbUpdateConnectOptions;
		CbBeforeWrite m_cbBeforeWrite;
		CbAfterRead m_cbAfterRead;

		///connect callback adapters taken over from AsyncConnectOptions. paho retains
		///the connect callbacks in the client and re-invokes them on every
		///automaticReconnect retry, so they must outlive every submitted options
		///object. Old adapters are NOT dropped on a new Connect: paho may still be
		///holding their pointers (previous in-flight/retrying connect), so one is
		///kept per submitted options object.
		std::vector<std::unique_ptr<AsRespAptBase>> m_connApts;

	public:
		MQPPAsync(const char* serverURI, const char* clientId, PersistenceType persistence_type = PT_PERSISTENCE_NONE, void* persistence_context = nullptr, MQTTAsync_createOptions* options = nullptr)
			: m_self(nullptr)
		{
			MQTTAsync_createOptions defOpt = MQTTAsync_createOptions_initializer5;
			MQTTAsync_createWithOptions(&m_self, serverURI, clientId, persistence_type, persistence_context, options ? options : &defOpt);
		}

		~MQPPAsync()
		{
			///members (callbacks, m_connApts) are destroyed after this body, which is
			///required: destroy removes the client under mqttasync_mutex, so no paho
			///thread can be inside - or start - one of our callbacks afterwards
			MQTTAsync_destroy(&m_self);
		}

		///copy and move deleted: the context registered by the SetCallbacks family is the
		///object's own (or a member's) address; after move/copy paho would still hold the
		///stale address, dangling on destruction
		MQPPAsync(const MQPPAsync&) = delete;
		MQPPAsync& operator=(const MQPPAsync&) = delete;
		MQPPAsync(MQPPAsync&&) = delete;
		MQPPAsync& operator=(MQPPAsync&&) = delete;

		operator bool() const
		{
			return !!m_self;
		}

		int SetDisconnected(CbDisconnected&& cbDisconnected)
		{
			m_cbDisconnected = std::move(cbDisconnected);

			return MQTTAsync_setDisconnected(m_self, &m_cbDisconnected,
				[](void* context, MQTTProperties* properties, enum MQTTReasonCodes reasonCode)
			{
				if (context)
				{
					auto& cb = (*(CbDisconnected*)context);
					if (cb) cb(properties, reasonCode);
				}
			});
		}

		int SetUpdateConnectOptions(CbUpdateConnectOptions&& cbUpdateConnectOptions)
		{
			m_cbUpdateConnectOptions = std::move(cbUpdateConnectOptions);

			return MQTTAsync_setUpdateConnectOptions(m_self, &m_cbUpdateConnectOptions,
				[](void* context, MQTTAsync_connectData* data)
			{
				if (context)
				{
					auto& cb = (*(CbUpdateConnectOptions*)context);
					if (cb) return cb(data);
				}
				return 1;
			});
		}

		int SetBeforePersistenceWrite(CbBeforeWrite&& cbBeforeWrite)
		{
			m_cbBeforeWrite = std::move(cbBeforeWrite);

			return MQTTAsync_setBeforePersistenceWrite(m_self, &m_cbBeforeWrite,
				[](void* context, int bufcount, char* buffers[], int buflens[])
			{
				if (context)
				{
					auto& cb = (*(CbBeforeWrite*)context);
					if (cb) return cb(bufcount, buffers, buflens);
				}
				return 0;
			});
		}

		int SetAfterPersistenceRead(CbAfterRead&& cbAfterRead)
		{
			m_cbAfterRead = std::move(cbAfterRead);

			return MQTTAsync_setAfterPersistenceRead(m_self, &m_cbAfterRead,
				[](void* context, char** buffer, int* buflen)
			{
				if (context)
				{
					auto& cb = (*(CbAfterRead*)context);
					if (cb) return cb(buffer, buflen);
				}
				return 0;
			});
		}

		int SetCallbacks(CbConnectionLost&& cbConnectionLost, CbMessageArrived&& cbMessageArrived, CbDeliveryComplete&& cbDeliveryComplete)
		{
			m_cbConnectionLost = std::move(cbConnectionLost);
			m_cbMessageArrived = std::move(cbMessageArrived);
			m_cbDeliveryComplete = std::move(cbDeliveryComplete);

			return MQTTAsync_setCallbacks(m_self, this,
				[](void* context, char* cause)
			{
				auto self = (MQPPAsync*)context;
				if (self && self->m_cbConnectionLost)
				{
					self->m_cbConnectionLost(cause);
				}

			}, [](void* context, char* topicName, int topicLen, MQTTAsync_message* message)
			{
				auto self = (MQPPAsync*)context;
				if (self && self->m_cbMessageArrived)
				{
					return self->m_cbMessageArrived(AsyncString(topicName, topicLen), message);
				}
				///TRUE: message accepted, paho frees it normally
				return 1;

			}, [](void* context, MQTTAsync_token token)
			{
				auto self = (MQPPAsync*)context;
				if (self && self->m_cbDeliveryComplete)
				{
					self->m_cbDeliveryComplete(token);
				}
			});
		}

		int SetConnectionLostCallback(CbConnectionLost&& cbConnectionLost)
		{
			m_cbConnectionLost = std::move(cbConnectionLost);

			return MQTTAsync_setConnectionLostCallback(m_self, &m_cbConnectionLost,
				[](void* context, char* cause)
			{
				if (context)
				{
					auto& cb = (*(CbConnectionLost*)context);
					if (cb) cb(cause);
				}
			});
		}

		int SetMessageArrivedCallback(CbMessageArrived&& cbMessageArrived)
		{
			m_cbMessageArrived = std::move(cbMessageArrived);

			return MQTTAsync_setMessageArrivedCallback(m_self, &m_cbMessageArrived,
				[](void* context, char* topicName, int topicLen, MQTTAsync_message* message)
			{
				if (context)
				{
					auto& cb = (*(CbMessageArrived*)context);
					if (cb) return cb(AsyncString(topicName, topicLen), message);
				}
				return 1;
			});
		}

		int SetDeliveryCompleteCallback(CbDeliveryComplete&& cbDeliveryComplete)
		{
			m_cbDeliveryComplete = std::move(cbDeliveryComplete);

			return MQTTAsync_setDeliveryCompleteCallback(m_self, &m_cbDeliveryComplete,
				[](void* context, MQTTAsync_token token)
			{
				if (context)
				{
					auto& cb = (*(CbDeliveryComplete*)context);
					if (cb) cb(token);
				}
			});
		}

		int SetConnected(CbConnected&& cbConnected)
		{
			m_cbConnected = std::move(cbConnected);

			return MQTTAsync_setConnected(m_self, &m_cbConnected,
				[](void* context, char* cause)
			{
				if (context)
				{
					auto& cb = (*(CbConnected*)context);
					if (cb) cb(cause);
				}
			});
		}

		int Reconnect()
		{
			return MQTTAsync_reconnect(m_self);
		}

		int Connect(const AsyncConnectOptions& options)
		{
			int ret = MQTTAsync_connect(m_self, options);
			if (ret == MQTTASYNC_SUCCESS)
			{
				///success: paho copied the callbacks into the queued command; keep the
				///adapter for this client's lifetime - automaticReconnect re-invokes the
				///connect callbacks on every retry, they are not one-shot
				m_connApts.emplace_back(options.Detach());
			}
			else if (ret == MQTTASYNC_COMMAND_IGNORED)
			{
				///duplicate queued CONNECT ignored, but paho already copied the callbacks into
				///m->connect (before the dedup) - the in-flight connection's completion takes
				///over, so the adapter must also outlive this options object
				m_connApts.emplace_back(options.Detach());
			}
			///other synchronous failures: paho did not take the adapter, ownership stays in
			///options and is reclaimed by its destructor (same object may be submitted again)
			return ret;
		}

		int Disconnect(const AsyncDisconnectOptions& options)
		{
			int ret = MQTTAsync_disconnect(m_self, options);
			if (ret == MQTTASYNC_SUCCESS)
			{
				options.Detach();
			}
			///synchronous failure: ownership stays in options, reclaimed by its destructor
			return ret;
		}

		int IsConnected()
		{
			return MQTTAsync_isConnected(m_self);
		}

		int Subscribe(const char* topic, int qos, const AsyncResponseOptions& response = AsyncResponseOptions())
		{
			int ret = MQTTAsync_subscribe(m_self, topic, qos, response);
			if (ret == MQTTASYNC_SUCCESS)
			{
				response.Detach();
			}
			///synchronous failure: ownership stays in response, reclaimed by its destructor
			return ret;
		}

		int SubscribeMany(int count, char* const* topic, const int* qos, const AsyncResponseOptions& response = AsyncResponseOptions())
		{
			int ret = MQTTAsync_subscribeMany(m_self, count, topic, qos, response);
			if (ret == MQTTASYNC_SUCCESS)
			{
				response.Detach();
			}
			///synchronous failure: ownership stays in response, reclaimed by its destructor
			return ret;
		}

		int Unsubscribe(const char* topic, const AsyncResponseOptions& response = AsyncResponseOptions())
		{
			int ret = MQTTAsync_unsubscribe(m_self, topic, response);
			if (ret == MQTTASYNC_SUCCESS)
			{
				response.Detach();
			}
			///synchronous failure: ownership stays in response, reclaimed by its destructor
			return ret;
		}

		int UnsubscribeMany(int count, char* const* topic, const AsyncResponseOptions& response = AsyncResponseOptions())
		{
			int ret = MQTTAsync_unsubscribeMany(m_self, count, topic, response);
			if (ret == MQTTASYNC_SUCCESS)
			{
				response.Detach();
			}
			///synchronous failure: ownership stays in response, reclaimed by its destructor
			return ret;
		}

		int Publish(const char* destinationName, int payloadlen, const void* payload, int qos, int retained, const AsyncResponseOptions& response = AsyncResponseOptions())
		{
			int ret = MQTTAsync_send(m_self, destinationName, payloadlen, payload, qos, retained, response);
			if (ret == MQTTASYNC_SUCCESS)
			{
				response.Detach();
			}
			///synchronous failure: ownership stays in response, reclaimed by its destructor
			return ret;
		}

		int PublishMessage(const char* destinationName, const AsyncMessage& msg, const AsyncResponseOptions& response = AsyncResponseOptions())
		{
			int ret = MQTTAsync_sendMessage(m_self, destinationName, msg, response);
			if (ret == MQTTASYNC_SUCCESS)
			{
				response.Detach();
			}
			///synchronous failure: ownership stays in response, reclaimed by its destructor
			return ret;
		}

		int GetPendingTokens(MQTTAsync_token **tokens)
		{
			return MQTTAsync_getPendingTokens(m_self, tokens);
		}

		int IsComplete(MQTTAsync_token token)
		{
			return MQTTAsync_isComplete(m_self, token);
		}

		int WaitForCompletion(MQTTAsync_token token, unsigned long timeout)
		{
			return MQTTAsync_waitForCompletion(m_self, token, timeout);
		}

		static void SetTraceLevel(enum MQTTASYNC_TRACE_LEVELS level)
		{
			MQTTAsync_setTraceLevel(level);
		}

		typedef std::function<void(enum MQTTASYNC_TRACE_LEVELS level, char* message)> CbTrace;

		static CbTrace& GetCbTrace()
		{
			static CbTrace s_cbTrace;
			return s_cbTrace;
		}

		static void SetTraceCallback(CbTrace&& cbTrace)
		{
			MQPPAsync::GetCbTrace() = std::move(cbTrace);

			MQTTAsync_setTraceCallback([](enum MQTTASYNC_TRACE_LEVELS level, char* message)
			{
				auto& cb = MQPPAsync::GetCbTrace();
				if (cb) cb(level, message);
			});
		}

	};
}