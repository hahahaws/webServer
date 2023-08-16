#include "UserRing.h"

#include "../exception/Exception.h"
#include "../log/message.h"

#include <cstring>

using namespace std;

auto UserRing::getFileDescriptorLimit(source_location sourceLocation) -> rlim_t {
    rlimit limit{};

    const int result{getrlimit(RLIMIT_NOFILE, &limit)};
    if (result != 0)
        throw Exception{message::combine(chrono::system_clock::now(), this_thread::get_id(), sourceLocation,
                                         Level::FATAL, std::strerror(errno))};

    return limit.rlim_cur;
}

UserRing::UserRing(unsigned int entries, io_uring_params &params, source_location sourceLocation) : userRing{} {
    const int result{io_uring_queue_init_params(entries, &this->userRing, &params)};
    if (result != 0)
        throw Exception{message::combine(chrono::system_clock::now(), this_thread::get_id(), sourceLocation,
                                         Level::FATAL, std::strerror(std::abs(result)))};
}

UserRing::UserRing(UserRing &&other) noexcept : userRing{other.userRing} { other.userRing.ring_fd = -1; }

auto UserRing::getSelfFileDescriptor() const noexcept -> int { return this->userRing.ring_fd; }

auto UserRing::registerSelfFileDescriptor(source_location sourceLocation) -> void {
    const int result{io_uring_register_ring_fd(&this->userRing)};
    if (result != 1)
        throw Exception{message::combine(chrono::system_clock::now(), this_thread::get_id(), sourceLocation,
                                         Level::FATAL, std::strerror(std::abs(result)))};
}

auto UserRing::registerCpu(std::uint_least8_t cpuCode, source_location sourceLocation) -> void {
    cpu_set_t cpuSet{};

    CPU_SET(cpuCode, &cpuSet);

    const int result{io_uring_register_iowq_aff(&this->userRing, sizeof(cpuSet), &cpuSet)};
    if (result != 0)
        throw Exception{message::combine(chrono::system_clock::now(), this_thread::get_id(), sourceLocation,
                                         Level::FATAL, std::strerror(std::abs(result)))};
}

auto UserRing::registerFileDescriptors(unsigned int fileDescriptorCount, source_location sourceLocation) -> void {
    const int result{io_uring_register_files_sparse(&this->userRing, fileDescriptorCount)};
    if (result != 0)
        throw Exception{message::combine(chrono::system_clock::now(), this_thread::get_id(), sourceLocation,
                                         Level::FATAL, std::strerror(std::abs(result)))};
}

auto UserRing::allocateFileDescriptorRange(unsigned int offset, unsigned int length, source_location sourceLocation)
        -> void {
    const int result{io_uring_register_file_alloc_range(&this->userRing, offset, length)};
    if (result != 0)
        throw Exception{message::combine(chrono::system_clock::now(), this_thread::get_id(), sourceLocation,
                                         Level::FATAL, std::strerror(std::abs(result)))};
}

auto UserRing::updateFileDescriptors(unsigned int offset, span<const int> fileDescriptors,
                                     source_location sourceLocation) -> void {
    const int result{
            io_uring_register_files_update(&this->userRing, offset, fileDescriptors.data(), fileDescriptors.size())};
    if (result < 0)
        throw Exception{message::combine(chrono::system_clock::now(), this_thread::get_id(), sourceLocation,
                                         Level::FATAL, std::strerror(std::abs(result)))};
}

auto UserRing::setupBufferRing(unsigned int entries, int id, unsigned int flags, source_location sourceLocation)
        -> io_uring_buf_ring * {
    int result;

    io_uring_buf_ring *const bufferRing{io_uring_setup_buf_ring(&this->userRing, entries, id, flags, &result)};
    if (bufferRing == nullptr)
        throw Exception{message::combine(chrono::system_clock::now(), this_thread::get_id(), sourceLocation,
                                         Level::FATAL, std::strerror(std::abs(result)))};

    return bufferRing;
}

auto UserRing::freeBufferRing(io_uring_buf_ring *bufferRing, unsigned int entries, int id,
                              source_location sourceLocation) -> void {
    const int result{io_uring_free_buf_ring(&this->userRing, bufferRing, entries, id)};
    if (result < 0)
        throw Exception{message::combine(chrono::system_clock::now(), this_thread::get_id(), sourceLocation,
                                         Level::FATAL, std::strerror(std::abs(result)))};
}

auto UserRing::submitWait(unsigned int count, source_location sourceLocation) -> void {
    const int result{io_uring_submit_and_wait(&this->userRing, count)};
    if (result < 0)
        throw Exception{message::combine(chrono::system_clock::now(), this_thread::get_id(), sourceLocation,
                                         Level::FATAL, std::strerror(std::abs(result)))};
}

auto UserRing::forEachCompletion(const function<auto(io_uring_cqe *cqe)->void> &task) noexcept -> int {
    int completionCount{0};

    unsigned int head;
    io_uring_cqe *cqe;
    io_uring_for_each_cqe(&this->userRing, head, cqe) {
        task(cqe);
        ++completionCount;
    }

    return completionCount;
}

auto UserRing::getSqe(source_location sourceLocation) -> io_uring_sqe * {
    io_uring_sqe *const sqe{io_uring_get_sqe(&this->userRing)};
    if (sqe == nullptr)
        throw Exception{message::combine(chrono::system_clock::now(), this_thread::get_id(), sourceLocation,
                                         Level::FATAL, "no sqe available")};

    return sqe;
}

auto UserRing::advanceCompletionBufferRingBuffer(io_uring_buf_ring *bufferRing, int completionCount,
                                                 int bufferRingBufferCount) noexcept -> void {
    __io_uring_buf_ring_cq_advance(&this->userRing, bufferRing, completionCount, bufferRingBufferCount);
}

UserRing::~UserRing() {
    if (this->userRing.ring_fd != -1) io_uring_queue_exit(&this->userRing);
}
