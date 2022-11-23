#include <iostream>
#include <chrono>
#include <unordered_map>
#include <string>

#include <CTP/ThreadPool.hpp>

using namespace Chen;
using namespace std;
using namespace std::chrono;

// ******************************** [Initial] *************************************************

template<class F, class... Args>
void test_func_impl(F&& f, Args&&... args) {
    std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
}

void test_func() {
    int res = 0;
    test_func_impl([&](int i) { std::cout << res + i << std::endl; return 0; }, 10);
    test_func_impl([&](int i) { std::cout << res + i << std::endl; return 1; }, 20);
}

class Base {
public:
    Base(int _i = 0) : i(_i) {}
    Base(const Base&) = delete;
    Base& operator=(const Base&) = delete;
private:
    int i;
};

void execute(std::function<void()>&& f) {
    f();
}

void test_copy() {
    Base b0(1);
    
    // execute([b0 = std::move(b0)]() {
    //     std::cout << "Hello" << std::endl;
    // });
}

// ********************************************************************************************

void test_thread_pool() {
	ThreadPool pool(4);

    int res = 0;

	auto result = pool.Submit([](int answer) { return answer; }, 2);

    pool.Submit([](int* r) { *r = 2; }, &res);

	std::cout << "result: " << result.get() << std::endl;
    std::cout << "res: " << res << std::endl;

    pool.Submit([](int answer) { std::cout << answer << std::endl; }, 2);
	pool.Submit([](int answer) { std::cout << answer << std::endl; }, 4);
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

    std::vector<std::future<void>> rets;
    rets.reserve(20000);

    for (int i = 0; i < 20000; ++i) {
        rets.push_back(pool.Submit([]() { 
            A tmp;
            tmp.execute();
        }));
    }

    for (auto &r : rets)
        r.get();

    steady_clock::time_point end_time = steady_clock::now();

    duration<double> time_span = duration_cast<duration<double>>(end_time - start_time);

    std::cout << "with_thread_pool_cost : " << time_span.count() << std::endl;  
}
