#pragma once

namespace PMPC				///Paho - MQTT CPP Cient
{
	enum PersistenceType {
		/**
		  * This <i>persistence_type</i> value specifies the default file system-based
		  * persistence mechanism (see MQTTClient_create()).
		  */
		PT_PERSISTENCE_DEFAULT = 0,
		/**
		  * This <i>persistence_type</i> value specifies a memory-based
		  * persistence mechanism (see MQTTClient_create()).
		  */
		PT_PERSISTENCE_NONE = 1,
		/**
		  * This <i>persistence_type</i> value specifies an application-specific
		  * persistence mechanism (see MQTTClient_create()).
		  */
		PT_PERSISTENCE_USER = 2
	};
}
