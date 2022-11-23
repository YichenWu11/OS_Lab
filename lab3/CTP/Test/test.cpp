#include <test.h>
#include <test_func.h>

int main() {

    // test_func();

    // test_thread_pool();

    test_efficiency_no_threadpool();
    test_efficiency_with_threadpool();

    return 0;
}
