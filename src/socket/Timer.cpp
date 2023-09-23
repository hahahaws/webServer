#include "Timer.hpp"

#include "../log/Log.hpp"
#include "../userRing/Submission.hpp"
#include "../userRing/UserData.hpp"
#include "SystemCallError.hpp"

#include <sys/timerfd.h>

#include <cstring>

Timer::Timer(unsigned int fileDescriptorIndex)
    : fileDescriptorIndex{fileDescriptorIndex}, now{0}, expireCount{0}, timingTask{nullptr}, cancelTask{nullptr},
      closeTask{nullptr} {}

auto Timer::create() -> unsigned int {
    const unsigned int fileDescriptor{Timer::createFileDescriptor()};

    Timer::setTime(fileDescriptor);

    return fileDescriptor;
}

auto Timer::createFileDescriptor(std::source_location sourceLocation) -> unsigned int {
    const int fileDescriptor{timerfd_create(CLOCK_MONOTONIC, 0)};

    if (fileDescriptor == -1)
        throw SystemCallError{Log::formatLog(Log::Level::Fatal, std::chrono::system_clock::now(),
                                             std::this_thread::get_id(), sourceLocation, std::strerror(errno))};

    return fileDescriptor;
}

auto Timer::setTime(unsigned int fileDescriptor, std::source_location sourceLocation) -> void {
    constexpr itimerspec time{{1, 0}, {1, 0}};
    if (timerfd_settime(static_cast<int>(fileDescriptor), 0, &time, nullptr) == -1)
        throw SystemCallError{Log::formatLog(Log::Level::Fatal, std::chrono::system_clock::now(),
                                             std::this_thread::get_id(), sourceLocation, std::strerror(errno))};
}

auto Timer::getFileDescriptorIndex() const noexcept -> unsigned int { return this->fileDescriptorIndex; }

auto Timer::timing(io_uring_sqe *sqe) noexcept -> const Awaiter & {
    constexpr unsigned long offset{0};
    const Submission submission{sqe,
                                this->fileDescriptorIndex,
                                {std::as_writable_bytes(std::span{&this->expireCount, 1})},
                                offset};

    const UserData userData{TaskType::Timeout, this->fileDescriptorIndex};
    submission.setUserData(std::bit_cast<unsigned long>(userData));

    submission.setFlags(IOSQE_FIXED_FILE);

    return this->awaiter;
}

auto Timer::setTimingTask(Task &&task) noexcept -> void { this->timingTask = std::move(task); }

auto Timer::resumeTiming(std::pair<int, unsigned int> result) -> void {
    this->awaiter.setResult(result);

    this->timingTask.resume();
}

auto Timer::clearTimeout() -> std::vector<unsigned int> {
    std::vector<unsigned int> timeoutFileDescriptors;

    while (this->expireCount > 0) {
        auto &fileDescriptors{this->wheel[this->now]};

        for (const unsigned int fileDescriptor: fileDescriptors) {
            timeoutFileDescriptors.emplace_back(fileDescriptor);

            this->location.erase(fileDescriptor);
        }

        fileDescriptors.clear();

        this->now = (this->now + 1) % this->wheel.size();

        --this->expireCount;
    }

    return timeoutFileDescriptors;
}

auto Timer::add(unsigned int fileDescriptor, unsigned char timeout, std::source_location sourceLocation) -> void {
    if (timeout >= this->wheel.size())
        throw SystemCallError{Log::formatLog(Log::Level::Fatal, std::chrono::system_clock::now(),
                                             std::this_thread::get_id(), sourceLocation, "timeout is too large")};

    const unsigned char point{static_cast<unsigned char>((this->now + timeout) % this->wheel.size())};

    this->location.emplace(fileDescriptor, point);
    this->wheel[point].emplace(fileDescriptor);
}

auto Timer::update(unsigned int fileDescriptor, unsigned char timeout, std::source_location sourceLocation) -> void {
    this->wheel[this->location.at(fileDescriptor)].erase(fileDescriptor);

    if (timeout >= this->wheel.size())
        throw SystemCallError{Log::formatLog(Log::Level::Fatal, std::chrono::system_clock::now(),
                                             std::this_thread::get_id(), sourceLocation, "timeout is too large")};

    const unsigned char point{static_cast<unsigned char>((this->now + timeout) % this->wheel.size())};

    this->wheel[point].emplace(fileDescriptor);
    this->location.at(fileDescriptor) = point;
}

auto Timer::remove(unsigned int fileDescriptor) -> void {
    this->wheel[this->location.at(fileDescriptor)].erase(fileDescriptor);
    this->location.erase(fileDescriptor);
}

auto Timer::cancel(io_uring_sqe *sqe) const noexcept -> const Awaiter & {
    const Submission submission{sqe, this->fileDescriptorIndex, IORING_ASYNC_CANCEL_ALL};

    const UserData userData{TaskType::Cancel, this->fileDescriptorIndex};
    submission.setUserData(std::bit_cast<unsigned long>(userData));

    submission.setFlags(IOSQE_FIXED_FILE);

    return this->awaiter;
}

auto Timer::setCancelTask(Task &&task) noexcept -> void { this->cancelTask = std::move(task); }

auto Timer::resumeCancel(std::pair<int, unsigned int> result) -> void {
    this->awaiter.setResult(result);

    this->cancelTask.resume();
}

auto Timer::close(io_uring_sqe *sqe) const noexcept -> const Awaiter & {
    const Submission submission{sqe, this->fileDescriptorIndex};

    const UserData userData{TaskType::Close, this->fileDescriptorIndex};
    submission.setUserData(std::bit_cast<unsigned long>(userData));

    return this->awaiter;
}

auto Timer::setCloseTask(Task &&task) noexcept -> void { this->closeTask = std::move(task); }

auto Timer::resumeClose(std::pair<int, unsigned int> result) -> void {
    this->awaiter.setResult(result);

    this->closeTask.resume();
}
