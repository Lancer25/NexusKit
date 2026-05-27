#pragma once

#include <chrono>
#include <functional>
#include <memory>

#include <nexus/common/export.h>

/// Cross-platform threading utilities.
namespace nexus::common {

namespace detail {
class EventStorage;
class ThreadStorage;
}

/// Cross-platform manual-reset waitable event.
///
/// Modeled after Win32 manual-reset events.  Threads calling `wait()` block
/// until another thread calls `emit()`.  The event stays signaled until
/// `reset()` is called.
class NEXUS_COMMON_API Event {
public:
    /// Creates an unsignaled event.
    Event();
    /// Event handles are move-only and cannot be copied.
    Event(const Event&) = delete;
    /// Event handles are move-only and cannot be copy-assigned.
    Event& operator=(const Event&) = delete;
    /// Releases the event handle.
    ~Event();

    /// Signal the event; all waiting threads are released.
    void emit();
    /// Reset to unsignaled state.
    void reset();
    /// Wait for the event to become signaled.
    ///
    /// @param timeout_ms Timeout in milliseconds, or -1 for infinite wait.
    /// @return true when signaled, false on timeout.
    bool wait(int timeout_ms = -1);

private:
    std::unique_ptr<detail::EventStorage> storage_;
};

/// Move-only background thread with start / stop lifecycle.
///
/// Copy is deleted; move transfers ownership.  A moved-from thread is stopped.
///
/// Usage:
/// @code
/// nexus::common::Thread t;
/// t.start([] { do_work(); }, std::chrono::milliseconds(100));
/// // ... later ...
/// t.stop();
/// @endcode
class NEXUS_COMMON_API Thread {
public:
    /// Constructs a stopped thread handle.
    Thread();
    /// Thread handles are move-only and cannot be copied.
    Thread(const Thread&) = delete;
    /// Thread handles are move-only and cannot be copy-assigned.
    Thread& operator=(const Thread&) = delete;
    /// Moves a thread handle.
    Thread(Thread&& other) noexcept;
    /// Moves a thread handle.
    Thread& operator=(Thread&& other) noexcept;
    /// Stops and releases the thread handle if needed.
    ~Thread();

    /// Start the callback on a new thread.
    ///
    /// When `interval` is non-zero, the callback runs in a loop with
    /// `interval` sleep between invocations until `stop()` or
    /// `request_stop()` is called.  When `interval` is zero (default),
    /// the callback runs once and the thread exits.
    ///
    /// @return true on success, false if already running.
    bool start(std::function<void()> callback,
               std::chrono::milliseconds interval = {});

    /// Synchronous stop; signals stop and joins the thread.
    ///
    /// Must not be called from within the thread callback; that would cause
    /// `std::thread::join` to deadlock waiting on itself.
    void stop();
    /// Asynchronous stop request; sets the stop flag without joining.
    void request_stop();

    /// True while the thread callback is executing.
    bool is_running() const;

    /// True after stop has been requested.
    ///
    /// Callbacks should check this periodically to exit early.
    bool stop_requested() const;

    /// Fire-and-forget: run a callback on a new detached thread.
    static void run_async(std::function<void()> callback);

private:
    std::unique_ptr<detail::ThreadStorage> storage_;
};

} // namespace nexus::common
