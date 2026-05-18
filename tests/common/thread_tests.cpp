#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <chrono>
#include <thread>

#include <nexus/common/thread.h>

using namespace std::chrono_literals;

TEST_CASE("Event starts unsignaled") {
    nexus::common::Event event;
    CHECK_FALSE(event.wait(0));
}

TEST_CASE("Event emit releases waiter") {
    nexus::common::Event event;
    event.emit();
    CHECK(event.wait(0));
}

TEST_CASE("Event reset returns to unsignaled") {
    nexus::common::Event event;
    event.emit();
    event.reset();
    CHECK_FALSE(event.wait(0));
}

TEST_CASE("Event emit wakes blocked thread") {
    nexus::common::Event event;
    std::atomic<bool> woken{false};

    std::thread t([&] {
        event.wait();
        woken.store(true);
    });

    std::this_thread::sleep_for(10ms);
    CHECK_FALSE(woken.load());

    event.emit();
    t.join();
    CHECK(woken.load());
}

TEST_CASE("Thread start runs callback") {
    nexus::common::Thread t;
    std::atomic<bool> ran{false};

    CHECK(t.start([&] { ran.store(true); }));
    t.stop();
    CHECK(ran.load());
}

TEST_CASE("Thread periodic runs multiple times") {
    nexus::common::Thread t;
    std::atomic<int> count{0};

    CHECK(t.start([&] { count.fetch_add(1); }, 1ms));
    std::this_thread::sleep_for(50ms);
    t.stop();

    CHECK(count.load() >= 3);
}

TEST_CASE("Thread stop_requested observable from callback") {
    nexus::common::Thread t;
    std::atomic<bool> seen{false};

    CHECK(t.start([&] {
        while (!t.stop_requested()) {
            std::this_thread::sleep_for(1ms);
        }
        seen.store(true);
    }));

    std::this_thread::sleep_for(5ms);
    t.request_stop();
    t.stop();
    CHECK(seen.load());
}

TEST_CASE("Thread double start returns false") {
    nexus::common::Thread t;
    CHECK(t.start([] { std::this_thread::sleep_for(50ms); }));
    CHECK_FALSE(t.start([] {}));
    t.stop();
}

TEST_CASE("Thread is_running reflects state") {
    nexus::common::Thread t;
    CHECK_FALSE(t.is_running());

    CHECK(t.start([] { std::this_thread::sleep_for(50ms); }));
    CHECK(t.is_running());

    t.stop();
    CHECK_FALSE(t.is_running());
}

TEST_CASE("Thread run_async executes independently") {
    std::atomic<bool> ran{false};
    nexus::common::Thread::run_async([&] {
        std::this_thread::sleep_for(20ms);
        ran.store(true);
    });

    std::this_thread::sleep_for(50ms);
    CHECK(ran.load());
}

TEST_CASE("Thread move transfers ownership") {
    nexus::common::Thread t1;
    std::atomic<bool> ran{false};
    CHECK(t1.start([&] { ran.store(true); }));

    auto t2 = std::move(t1);
    CHECK_FALSE(t1.is_running());
    CHECK(t2.is_running());

    t2.stop();
    CHECK(ran.load());
}
