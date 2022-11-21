#include <CTP/ThreadPool.hpp>
#include <iostream>

using namespace Chen;

void test_thread_pool() {
	ThreadPool pool(4);

    int res = 0;

	auto result = pool.ReturnSubmit([](int answer) { return answer; }, 2);

    pool.ReturnSubmit([](int* r) { *r = 2; }, &res);

	std::cout << "result: " << result.get() << std::endl;
    std::cout << "res: " << res << std::endl;
}
