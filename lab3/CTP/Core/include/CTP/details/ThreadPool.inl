#pragma once

namespace Chen {
    template<class F, class... Args>
    auto ThreadPool::Submit(F&& f, Args&&... args) {
        using return_type = std::invoke_result_t<F, Args...>;

        std::promise<return_type> barrier;

        std::future<return_type> fret = barrier.get_future();

        // Here
        BasicSubmit([f = std::forward<F>(f), barrier = std::move(barrier), ... args = std::forward<Args>(args)]() mutable {
            if constexpr (std::is_void_v<return_type>) {
                std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
                barrier.set_value();  // only for Sync
            }
            else
                barrier.set_value(std::invoke(std::forward<F>(f), std::forward<Args>(args)...));
        });

        return fret;
    }
}
