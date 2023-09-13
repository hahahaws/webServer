#pragma once

#include <coroutine>

class Task {
public:
    struct promise_type {
        [[nodiscard]] auto get_return_object() -> Task;

        [[nodiscard]] constexpr auto initial_suspend() const noexcept -> std::suspend_always { return {}; }

        [[nodiscard]] constexpr auto final_suspend() const noexcept -> std::suspend_always { return {}; }

        auto unhandled_exception() const -> void;
    };

    explicit Task(std::coroutine_handle<promise_type> handle) noexcept;

    Task(const Task &) = delete;

    Task(Task &&) noexcept;

    auto operator=(Task &&) noexcept -> Task &;

    auto resume() const -> void;

    ~Task();

private:
    auto destroy() const -> void;

    std::coroutine_handle<promise_type> handle;
};
