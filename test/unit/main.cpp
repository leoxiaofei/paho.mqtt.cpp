#include "test_framework.hpp"

int main()
{
	return testfw::RunAll() == 0 ? 0 : 1;
}
