#define CATCH_CONFIG_MAIN  // Do not copy to new tests

#include "block_allocator.h"
#include "catch.hpp"

TEST_CASE("Initialization and Deinitialization", "[allocator]") {
    balloc_t state;
    REQUIRE(balloc_init(&state) == true);
    REQUIRE(balloc_get_size(&state) == 0);
    REQUIRE(balloc_get_capacity(&state) == BLOCK_ALLOCATOR_COUNT);
    REQUIRE(balloc_deinit(&state) == true);
}

TEST_CASE("Single Allocation", "[allocator]") {
    balloc_t state;
    balloc_init(&state);

    void* ptr = balloc_allocate(&state);
    REQUIRE(ptr != NULL);
    REQUIRE(reinterpret_cast<uintptr_t>(ptr) % BLOCK_ALLOCATOR_MEM_ALIGN == 0);

    REQUIRE(balloc_get_size(&state) == 1);
    REQUIRE(balloc_get_capacity(&state) == BLOCK_ALLOCATOR_COUNT);

    balloc_deinit(&state);
}

TEST_CASE("Allocate All Blocks", "[allocator]") {
    balloc_t state;
    balloc_init(&state);

    void* ptrs[BLOCK_ALLOCATOR_COUNT] = {NULL};
    for (int i = 0; i < BLOCK_ALLOCATOR_COUNT; ++i) {
        ptrs[i] = balloc_allocate(&state);
        REQUIRE(ptrs[i] != NULL);
        REQUIRE(balloc_get_size(&state) == i + 1);
    }

    REQUIRE(balloc_get_capacity(&state) == BLOCK_ALLOCATOR_COUNT);

    void* overflow = balloc_allocate(&state);
    REQUIRE(overflow == NULL);

    REQUIRE(balloc_get_size(&state) == BLOCK_ALLOCATOR_COUNT);

    balloc_deinit(&state);
}

TEST_CASE("Deallocation", "[allocator]") {
    balloc_t state;
    balloc_init(&state);

    void* ptr = balloc_allocate(&state);
    REQUIRE(ptr != NULL);

    REQUIRE(balloc_deallocate(&state, ptr) == true);
    REQUIRE(balloc_deallocate(&state, ptr) == true);  // Double free

    balloc_deinit(&state);
}

TEST_CASE("Invalid Deallocation", "[allocator]") {
    balloc_t state;
    balloc_init(&state);

    SECTION("Null pointer") { REQUIRE(balloc_deallocate(&state, NULL) == false); }

    SECTION("Pointer outside pool") {
        uint8_t external;
        REQUIRE(balloc_deallocate(&state, &external) == false);
    }

    SECTION("Misaligned pointer") {
        void* ptr = balloc_allocate(&state);
        REQUIRE(ptr != NULL);
        uint8_t* misaligned = static_cast<uint8_t*>(ptr) + 1;
        REQUIRE(balloc_deallocate(&state, misaligned) == false);
    }

    balloc_deinit(&state);
}

TEST_CASE("Reuse After Deallocation", "[allocator]") {
    balloc_t state;
    balloc_init(&state);

    void* ptr1 = balloc_allocate(&state);
    REQUIRE(ptr1 != NULL);
    REQUIRE(balloc_deallocate(&state, ptr1) == true);

    void* ptr2 = balloc_allocate(&state);
    REQUIRE(ptr2 != NULL);
    REQUIRE(ptr1 == ptr2);

    balloc_deinit(&state);
}

TEST_CASE("Fragmentation Handling", "[allocator]") {
    balloc_t state;
    balloc_init(&state);

    void* ptr1 = balloc_allocate(&state);
    void* ptr2 = balloc_allocate(&state);
    REQUIRE(ptr1 != NULL);
    REQUIRE(ptr2 != NULL);

    REQUIRE(balloc_get_size(&state) == 2);

    REQUIRE(balloc_deallocate(&state, ptr1) == true);
    REQUIRE(balloc_deallocate(&state, ptr2) == true);

    REQUIRE(balloc_get_size(&state) == 0);

    for (int i = 0; i < BLOCK_ALLOCATOR_COUNT; ++i) {
        REQUIRE(balloc_allocate(&state) != NULL);
    }

    balloc_deinit(&state);
}
