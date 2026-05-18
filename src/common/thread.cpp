#include <nexus/common/thread.h>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <utility>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#else
#include <chrono>
#endif

namespace nexus::common {
namespace detail {

#if defined(_WIN32)

class EventStorage {
public:
    EventStorage() {
        handle_ = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    }

    ~EventStorage() {
        if (handle_ != nullptr) {
            CloseHandle(handle_);
        }
    }

    void emit() {
        if (handle_ != nullptr) {
            SetEvent(handle_);
        }
    }

    void reset() {
        if (handle_ != nullptr) {
            ResetEvent(handle_);
        }
    }

    bool wait(int timeout_ms) {
        if (handle_ == nullptr) {
            return false;
        }
        const auto result = WaitForSingleObject(
            handle_,
            timeout_ms < 0 ? INFINITE : static_cast<DWORD>(timeout_ms));
        return result == WAIT_OBJECT_0;
    }

private:
    HANDLE handle_ = nullptr;
};

#else // Linux / POSIX

class EventStorage {
public:
    EventStorage() = default;

    void emit() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            signaled_ = true;
        }
        cond_.notify_all();
    }

    void reset() {
        std::lock_guard<std::mutex> lock(mutex_);
        signaled_ = false;
    }

    bool wait(int timeout_ms) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (timeout_ms < 0) {
            cond_.wait(lock, [this] { return signaled_; });
            return true;
        }
        return cond_.wait_for(
            lock,
            std::chrono::milliseconds(timeout_ms),
            [this] { return signaled_; });
    }

private:
    std::mutex mutex_;
    std::condition_variable cond_;
    bool signaled_ = false;
};

#endif

class ThreadStorage {
public:
    ~ThreadStorage() {
        stop();
    }

    bool start(std::function<void()> callback,
               std::chrono::milliseconds interval) {
        std::lock_guard<std::mutex> lock(state_mutex_);
        if (running_) {
            return false;
        }

        stop_requested_.store(false);
        running_.store(true);

        thread_ = std::thread([this, callback = std::move(callback), interval] {
            do {
                callback();
                if (stop_requested_.load()) {
                    break;
                }
                if (interval.count() > 0) {
                    std::this_thread::sleep_for(interval);
                }
            } while (interval.count() > 0 && !stop_requested_.load());

            running_.store(false);
        });

        return true;
    }

    void stop() {
        {
            std::lock_guard<std::mutex> lock(state_mutex_);
            stop_requested_.store(true);
        }
        if (thread_.joinable()) {
            thread_.join();
        }
    }

    void request_stop() {
        std::lock_guard<std::mutex> lock(state_mutex_);
        stop_requested_.store(true);
    }

    bool is_running() const {
        return running_.load();
    }

    bool stop_requested() const {
        return stop_requested_.load();
    }

    static void run_async(std::function<void()> callback) {
        std::thread t(std::move(callback));
        t.detach();
    }

private:
    std::thread thread_;
    std::mutex state_mutex_;
    std::atomic<bool> running_{false};
    std::atomic<bool> stop_requested_{false};
};

} // namespace detail

Event::Event() : storage_(std::make_unique<detail::EventStorage>()) {}
Event::~Event() = default;

void Event::emit() {
    if (storage_) {
        storage_->emit();
    }
}

void Event::reset() {
    if (storage_) {
        storage_->reset();
    }
}

bool Event::wait(int timeout_ms) {
    return storage_ && storage_->wait(timeout_ms);
}

Thread::Thread() : storage_(std::make_unique<detail::ThreadStorage>()) {}
Thread::~Thread() = default;

Thread::Thread(Thread&& other) noexcept = default;
Thread& Thread::operator=(Thread&& other) noexcept = default;

bool Thread::start(std::function<void()> callback,
                   std::chrono::milliseconds interval) {
    if (!storage_) {
        storage_ = std::make_unique<detail::ThreadStorage>();
    }
    return storage_->start(std::move(callback), interval);
}

void Thread::stop() {
    if (storage_) {
        storage_->stop();
    }
}

void Thread::request_stop() {
    if (storage_) {
        storage_->request_stop();
    }
}

bool Thread::is_running() const {
    return storage_ && storage_->is_running();
}

bool Thread::stop_requested() const {
    return !storage_ || storage_->stop_requested();
}
void Thread::run_async(std::function<void()> callback) {
    detail::ThreadStorage::run_async(std::move(callback));
}

} // namespace nexus::common
