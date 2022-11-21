#pragma once

namespace Chen {
    template<class F, class... Args>
    auto ThreadPool::ReturnSubmit(F&& f, Args&&... args) {
        using return_type = std::invoke_result_t<F, Args...>;

        std::promise<return_type> barrier;

        std::future<return_type> rst = barrier.get_future();

        BasicSubmit([f = std::forward<F>(f), barrier = std::move(barrier), ... args = std::forward<Args>(args)]() mutable {
            if constexpr (std::is_void_v<return_type>) {
                std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
                barrier.set_value();
            }
            else
                barrier.set_value(std::invoke(std::forward<F>(f), std::forward<Args>(args)...));
        });

        return rst;
    }
}
