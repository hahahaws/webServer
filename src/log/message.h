#pragma once

#include <source_location>
#include <thread>

enum class Level : std::uint_least8_t { INFO, WARN, ERROR, FATAL };

namespace message {
    auto combine(std::chrono::system_clock::time_point timestamp, std::jthread::id threadId,
                 std::source_location sourceLocation, Level level, std::string &&information) -> std::string;
}
