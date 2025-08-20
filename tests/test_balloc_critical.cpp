#include <atomic>
#include <thread>
#include <vector>

#include "block_allocator.h"
#include "catch.hpp"

static std::mutex g_allocator_mutex;

static std::atomic<int> enter_count(0);
static std::atomic<int> exit_count(0);

void balloc_enter_critical(balloc_t* state) {
    (void)state;
    enter_count++;
    g_allocator_mutex.lock();
}

void balloc_exit_critical(balloc_t* state) {
    (void)state;
    exit_count++;
    g_allocator_mutex.unlock();
}

TEST_CASE("Critical section calls are balanced", "[critical]") {
    enter_count = 0;
    exit_count = 0;

    balloc_t state;
    balloc_init(&state);

    SECTION("Single allocation and deallocation") {
        void* ptr = balloc_allocate(&state);
        REQUIRE(ptr != nullptr);
        REQUIRE(enter_count == 1);
        REQUIRE(exit_count == 1);

        bool result = balloc_deallocate(&state, ptr);
        REQUIRE(result == true);
        REQUIRE(enter_count == 2);
        REQUIRE(exit_count == 2);
    }

    SECTION("Multiple operations") {
        void* ptr1 = balloc_allocate(&state);
        void* ptr2 = balloc_allocate(&state);

        REQUIRE(enter_count == 2);
        REQUIRE(exit_count == 2);

        balloc_deallocate(&state, ptr1);
        balloc_deallocate(&state, ptr2);

        REQUIRE(enter_count == 4);
        REQUIRE(exit_count == 4);
    }

    balloc_deinit(&state);
}

TEST_CASE("Critical sections protect internal state", "[critical]") {
    enter_count = 0;
    exit_count = 0;

    balloc_t state;
    balloc_init(&state);

    void* ptr1 = balloc_allocate(&state);
    void* ptr2 = balloc_allocate(&state);
    balloc_get_size(&state);
    balloc_deallocate(&state, ptr1);
    balloc_deallocate(&state, ptr2);

    REQUIRE(enter_count == exit_count);

    balloc_deinit(&state);
}

TEST_CASE("Thread safety under contention", "[critical][thread]") {
    balloc_t state;
    balloc_init(&state);

    constexpr int NUM_THREADS = 4;
    constexpr int OPERATIONS_PER_THREAD = 1000;
    std::vector<std::thread> threads;
    std::vector<void*> allocated_ptrs;
    std::mutex allocated_mutex;

    for (int i = 0; i < NUM_THREADS; i++) {
        threads.emplace_back([&state, &allocated_ptrs, &allocated_mutex, i]() {
            for (int j = 0; j < OPERATIONS_PER_THREAD; j++) {
                void* ptr = balloc_allocate(&state);
                if (ptr != nullptr) {
                    {
                        std::lock_guard<std::mutex> lock(allocated_mutex);
                        allocated_ptrs.push_back(ptr);
                    }
                    std::this_thread::yield();

                    if ((i + j) % 2 == 0) {
                        balloc_deallocate(&state, ptr);
                        {
                            std::lock_guard<std::mutex> lock(allocated_mutex);
                            auto it = std::find(allocated_ptrs.begin(), allocated_ptrs.end(), ptr);
                            if (it != allocated_ptrs.end()) {
                                allocated_ptrs.erase(it);
                            }
                        }
                    }
                }
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    {
        std::lock_guard<std::mutex> lock(allocated_mutex);
        for (void* ptr : allocated_ptrs) {
            balloc_deallocate(&state, ptr);
        }
    }

    balloc_deinit(&state);
}

TEST_CASE("No data race in bitmap access", "[critical][thread]") {
    balloc_t state;
    balloc_init(&state);

    constexpr int NUM_THREADS = 8;
    constexpr int ITERATIONS = 1000;
    std::vector<std::thread> threads;
    std::atomic<bool> error_detected(false);

    for (int i = 0; i < NUM_THREADS; i++) {
        threads.emplace_back([&state, &error_detected]() {
            for (int j = 0; j < ITERATIONS; j++) {
                void* ptr = balloc_allocate(&state);
                if (ptr != nullptr) {
                    if ((reinterpret_cast<uintptr_t>(ptr) % BLOCK_ALLOCATOR_MEM_ALIGN) != 0) {
                        error_detected = true;
                    }

                    std::this_thread::yield();

                    bool result = balloc_deallocate(&state, ptr);
                    if (!result) {
                        error_detected = true;
                    }
                }
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    REQUIRE(!error_detected);
    REQUIRE(balloc_get_size(&state) == 0);

    balloc_deinit(&state);
}
