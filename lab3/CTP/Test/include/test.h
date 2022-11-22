#include <iostream>
#include <chrono>
#include <unordered_map>
#include <string>

#include <CTP/ThreadPool.hpp>

using namespace Chen;
using namespace std;
using namespace std::chrono;

template<class F, class... Args>
auto test_func_impl(F&& f, Args&&... args) {
    using return_type = std::invoke_result_t<F, Args...>;

    std::promise<return_type> barrier;

    std::future<return_type> rst = barrier.get_future();

    if constexpr (std::is_void_v<return_type>) {
        std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
        barrier.set_value();
    }
    else
        barrier.set_value(std::invoke(std::forward<F>(f), std::forward<Args>(args)...));

    return rst;
}

void test_func() {
    int res = 0;
    auto res0 = test_func_impl([&](int i) { std::cout << res + i << std::endl; return 0; }, 10);
    auto res1 = test_func_impl([&](int i) { std::cout << res + i << std::endl; return 1; }, 20);
    std::cout << "result: " << res0.get() << std::endl;
    std::cout << "result: " << res1.get() << std::endl;
}

void error() {
    std::promise<void> barrier;
    std::future<void> rst = barrier.get_future();

    fu2::unique_function<void()> task0 = [barrier = std::move(barrier)](){ int i = 0; };
    // std::function<void()> task1 = [barrier = std::move(barrier)](){ int i = 0; };
}


void test_thread_pool() {
	ThreadPool pool(4);

    int res = 0;

	auto result = pool.ReturnSubmit([](int answer) { return answer; }, 2);

    pool.ReturnSubmit([](int* r) { *r = 2; }, &res);

	std::cout << "result: " << result.get() << std::endl;
    std::cout << "res: " << res << std::endl;

    pool.ReturnSubmit([](int answer) { std::cout << answer << std::endl; }, 2);
	pool.ReturnSubmit([](int answer) { std::cout << answer << std::endl; }, 4);
}

class A {
public:
    A() { constructMap(); }

    void execute() {
        int res = 0;
        for (int i = 0; i < 300; ++i) {
            for (const auto& ele: str2int) {
                std::string tmp = ele.first;
                res += ele.second;
            }
        }
    }

private:
    void constructMap() {
        str2int["0"] = 0;
        str2int["1"] = 1;
        str2int["2"] = 2;
        str2int["3"] = 3;
        str2int["4"] = 4;
        str2int["5"] = 5;
        str2int["6"] = 6;
        str2int["7"] = 7;
        str2int["8"] = 8;
        str2int["9"] = 9;
    }

    std::unordered_map<std::string, int> str2int;  
};

void test_efficiency_no_threadpool() {
    steady_clock::time_point start_time = steady_clock::now();

    // ********************************************************************

    for (int i = 0; i < 20000; ++i) {
        A tmp;
        tmp.execute();
    }

    // ********************************************************************

    steady_clock::time_point end_time = steady_clock::now();

    duration<double> time_span = duration_cast<duration<double>>(end_time - start_time);

    std::cout << "no_thread_pool_cost   : " << time_span.count() << std::endl;
}

void test_efficiency_with_threadpool() {
    ThreadPool pool(std::thread::hardware_concurrency());

    steady_clock::time_point start_time = steady_clock::now();

    for (int i = 0; i < 20000; ++i) {
        pool.ReturnSubmit([]() { 
            A tmp;
            tmp.execute();
        });
    }

    steady_clock::time_point end_time = steady_clock::now();

    duration<double> time_span = duration_cast<duration<double>>(end_time - start_time);

    std::cout << "with_thread_pool_cost : " << time_span.count() << std::endl;  
}
