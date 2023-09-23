#pragma once

#include "../coroutine/Awaiter.hpp"
#include "../coroutine/Task.hpp"

#include <liburing.h>

#include <netinet/in.h>
#include <source_location>

class Server {
public:
    explicit Server(unsigned int fileDescriptorIndex) noexcept;

    Server(const Server &) = delete;

    Server(Server &&) = default;

    auto operator=(const Server &) -> Server & = delete;

    auto operator=(Server &&) -> Server & = default;

    ~Server() = default;

    [[nodiscard]] static auto create(unsigned short port) -> unsigned int;

private:
    [[nodiscard]] static auto socket(std::source_location sourceLocation = std::source_location::current())
            -> unsigned int;

    static auto setSocketOption(unsigned int fileDescriptor,
                                std::source_location sourceLocation = std::source_location::current()) -> void;

    static auto translateIpAddress(in_addr &address,
                                   std::source_location sourceLocation = std::source_location::current()) -> void;

    static auto bind(unsigned int fileDescriptor, const sockaddr_in &address,
                     std::source_location sourceLocation = std::source_location::current()) -> void;

    static auto listen(unsigned int fileDescriptor,
                       std::source_location sourceLocation = std::source_location::current()) -> void;

public:
    [[nodiscard]] auto getFileDescriptorIndex() const noexcept -> unsigned int;

    auto startAccept(io_uring_sqe *sqe) const noexcept -> void;

    [[nodiscard]] auto accept() const noexcept -> const Awaiter &;

    auto setAcceptTask(Task &&task) noexcept -> void;

    auto resumeAccept(std::pair<int, unsigned int> result) -> void;

    [[nodiscard]] auto cancel(io_uring_sqe *sqe) const noexcept -> const Awaiter &;

    auto setCancelTask(Task &&task) noexcept -> void;

    auto resumeCancel(std::pair<int, unsigned int> result) -> void;

    [[nodiscard]] auto close(io_uring_sqe *sqe) const noexcept -> const Awaiter &;

    auto setCloseTask(Task &&task) noexcept -> void;

    auto resumeClose(std::pair<int, unsigned int> result) -> void;

private:
    unsigned int fileDescriptorIndex;
    Task acceptTask, cancelTask, closeTask;
    Awaiter awaiter;
};
