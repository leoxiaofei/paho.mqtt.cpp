#include "test_framework.hpp"

#include <paho-mqtt-cpp/MQPPAsync.hpp>
#include <paho-mqtt-cpp/MQPPSync.hpp>

#include <cstring>
#include <sstream>
#include <string>
#include <utility>

namespace
{
	///AsyncString/SyncString free their buffer with MQTTAsync_free/MQTTClient_free,
	///so they must be built from buffers allocated by the matching allocator
	char* AllocAsync(const std::string& text)
	{
		char* buffer = (char*)MQTTAsync_malloc(text.size() + 1);
		memcpy(buffer, text.c_str(), text.size() + 1);
		return buffer;
	}

	char* AllocSync(const std::string& text)
	{
		char* buffer = (char*)MQTTClient_malloc(text.size() + 1);
		memcpy(buffer, text.c_str(), text.size() + 1);
		return buffer;
	}
}

TEST(AsyncString_KeepsExplicitLength)
{
	char* buffer = AllocAsync("hello");
	PMPC::AsyncString str(buffer, 5);

	CHECK(str.data() == buffer);
	CHECK_EQ(str.size(), 5);
}

TEST(AsyncString_FallsBackToStrlenWhenLengthZero)
{
	///paho passes topicLen=0 when the topic has no embedded NUL
	PMPC::AsyncString str(AllocAsync("hello"), 0);

	CHECK_EQ(str.size(), 5);
}

TEST(AsyncString_MoveTransfersOwnership)
{
	PMPC::AsyncString str(AllocAsync("hello"), 5);
	PMPC::AsyncString moved(std::move(str));

	CHECK(str.data() == nullptr);
	CHECK_EQ(str.size(), 0);
	CHECK(moved.data() != nullptr);
	CHECK_EQ(moved.size(), 5);
}

TEST(AsyncString_StreamsToOstream)
{
	PMPC::AsyncString str(AllocAsync("topic/1"), 7);

	std::ostringstream os;
	os << str;

	CHECK_EQ(os.str(), std::string("topic/1"));
}

TEST(SyncString_KeepsExplicitLength)
{
	char* buffer = AllocSync("hello");
	PMPC::SyncString str(buffer, 5);

	CHECK(str.data() == buffer);
	CHECK_EQ(str.size(), 5);
}

TEST(SyncString_FallsBackToStrlenWhenLengthZero)
{
	PMPC::SyncString str(AllocSync("hello"), 0);

	CHECK_EQ(str.size(), 5);
}

TEST(SyncString_MoveTransfersOwnership)
{
	PMPC::SyncString str(AllocSync("hello"), 5);
	PMPC::SyncString moved(std::move(str));

	CHECK(str.data() == nullptr);
	CHECK_EQ(str.size(), 0);
	CHECK(moved.data() != nullptr);
	CHECK_EQ(moved.size(), 5);
}

TEST(SyncString_StreamsToOstream)
{
	PMPC::SyncString str(AllocSync("topic/1"), 7);

	std::ostringstream os;
	os << str;

	CHECK_EQ(os.str(), std::string("topic/1"));
}
