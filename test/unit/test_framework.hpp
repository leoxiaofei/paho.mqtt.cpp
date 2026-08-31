#pragma once

#include <cstdio>
#include <sstream>
#include <string>
#include <vector>

///minimal dependency-free test framework: TEST() registers a function,
///testfw::RunAll() in main() runs them all and returns the failure count

namespace testfw
{
	struct TestCase
	{
		const char* name;
		void (*fn)();
	};

	inline std::vector<TestCase>& Registry()
	{
		static std::vector<TestCase> registry;
		return registry;
	}

	struct Result
	{
		bool failed;
		bool skipped;

		Result()
			: failed(false)
			, skipped(false)
		{}
	};

	inline Result& Current()
	{
		static Result result;
		return result;
	}

	inline void Skip(const char* reason)
	{
		Current().skipped = true;
		std::printf("         skipped: %s\n", reason);
	}

	inline int RunAll()
	{
		int passed = 0;
		int failed = 0;
		int skipped = 0;

		for (const TestCase& test : Registry())
		{
			Current() = Result();
			test.fn();

			if (Current().skipped)
			{
				++skipped;
				std::printf("[ SKIP   ] %s\n", test.name);
			}
			else if (Current().failed)
			{
				++failed;
				std::printf("[  FAIL  ] %s\n", test.name);
			}
			else
			{
				++passed;
				std::printf("[ PASS   ] %s\n", test.name);
			}
		}

		std::printf("\n%d passed, %d failed, %d skipped\n", passed, failed, skipped);
		return failed;
	}
}

#define TEST(name) \
	static void TESTFN_##name(); \
	static const bool TESTREG_##name = (testfw::Registry().push_back({#name, &TESTFN_##name}), true); \
	static void TESTFN_##name()

#define CHECK(cond) \
	do { \
		if (!(cond)) { \
			testfw::Current().failed = true; \
			std::printf("         CHECK failed: %s (%s:%d)\n", #cond, __FILE__, __LINE__); \
		} \
	} while (false)

///compares via operator== and prints both sides as streams
#define CHECK_EQ(actual, expected) \
	do { \
		std::ostringstream check_eq_a_, check_eq_e_; \
		check_eq_a_ << (actual); \
		check_eq_e_ << (expected); \
		if (check_eq_a_.str() != check_eq_e_.str()) { \
			testfw::Current().failed = true; \
			std::printf("         CHECK_EQ failed: %s == %s (got \"%s\", want \"%s\") (%s:%d)\n", \
				#actual, #expected, check_eq_a_.str().c_str(), check_eq_e_.str().c_str(), __FILE__, __LINE__); \
		} \
	} while (false)

///check a precondition; on failure abort the current test (the test body is a plain function)
#define REQUIRE(cond) \
	do { \
		if (!(cond)) { \
			testfw::Current().failed = true; \
			std::printf("         REQUIRE failed: %s (%s:%d)\n", #cond, __FILE__, __LINE__); \
			return; \
		} \
	} while (false)
