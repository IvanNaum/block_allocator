#include "block_allocator.h"

#include <stddef.h>

#define _BIT_PER_BYTE (8)

static bool _validate_ptr(balloc_t* state, void* ptr);
static uint16_t _get_block_number_by_ptr(balloc_t* state, void* ptr);

/**
 * @brief Enters a critical section for thread-safe operations.
 * @param state Allocator instance.
 * @note Default implementation does nothing. Override for OS environments.
 */
void balloc_enter_critical(balloc_t* state) {
    (void)state;
    return;
}

/**
 * @brief Exits a critical section.
 * @param state Allocator instance.
 */
void balloc_exit_critical(balloc_t* state) {
    (void)state;
    return;
}

/**
 * @brief Initializes allocator with all blocks free.
 * @param state Uninitialized allocator instance.
 * @return true Success, else false.
 */
bool balloc_init(balloc_t* state) {
    for (uint16_t i = 0; i < BLOCK_ALLOCATOR_BITMAP_SIZE; ++i) {
        state->free_bitmap[i] = 0;
    }
    return true;
}

/**
 * @brief Allocates a single fixed-size block.
 * @param state Initialized allocator instance.
 * @return void* Pointer to allocated block, else NULL.
 */
void* balloc_allocate(balloc_t* state) {
    balloc_enter_critical(state);
    void* result = NULL;

    for (uint16_t i = 0; i < BLOCK_ALLOCATOR_BITMAP_SIZE; ++i) {
        if (state->free_bitmap[i] != 0xff) {
            uint8_t block_number = 0;
            while ((state->free_bitmap[i] & (1 << block_number)) != 0) {
                block_number++;
            }
            state->free_bitmap[i] |= 1 << block_number;
            result = state->pool + BLOCK_ALLOCATOR_SIZE * (i * _BIT_PER_BYTE + block_number);
            break;
        }
    }

    balloc_exit_critical(state);
    return result;
}

/**
 * @brief Releases a previously allocated block.
 * @param state Initialized allocator instance.
 * @param ptr Block to deallocate.
 * @return true Success, else false.
 */
bool balloc_deallocate(balloc_t* state, void* ptr) {
    if (!_validate_ptr(state, ptr)) {
        return false;
    }

    balloc_enter_critical(state);
    uint16_t block_num = _get_block_number_by_ptr(state, ptr);
    state->free_bitmap[block_num / _BIT_PER_BYTE] &= ~((uint8_t)1 << (block_num % _BIT_PER_BYTE));
    balloc_exit_critical(state);

    return true;
}

/**
 * @brief Returns number of currently allocated blocks.
 * @param state Initialized allocator instance.
 * @return uint16_t Count of allocated blocks.
 */
uint16_t balloc_get_size(balloc_t* state) {
    balloc_enter_critical(state);
    uint16_t size = 0;
    for (uint16_t i = 0; i < BLOCK_ALLOCATOR_BITMAP_SIZE; ++i) {
        for (uint8_t j = 0; j < _BIT_PER_BYTE; ++j) {
            size += (state->free_bitmap[i] >> j) & 1;
        }
    }
    balloc_exit_critical(state);
    return size;
}

/**
 * @brief Returns total available blocks in allocator.
 * @param state Initialized allocator instance.
 * @return uint16_t Total capacity (BLOCK_ALLOCATOR_COUNT).
 */
uint16_t balloc_get_capacity(balloc_t* state) {
    (void)state;
    return BLOCK_ALLOCATOR_COUNT;
}

/**
 * @brief Deinitializes allocator.
 * @param state Initialized allocator instance.
 * @return true Success, else false.
 */
bool balloc_deinit(balloc_t* state) { return balloc_init(state); }

static bool _validate_ptr(balloc_t* state, void* ptr) {
    const uintptr_t start = (uintptr_t)state->pool;
    const uintptr_t end = start + BLOCK_ALLOCATOR_COUNT * BLOCK_ALLOCATOR_SIZE;
    const uintptr_t p = (uintptr_t)ptr;

    // Check that pointer:
    // 1. Within pool
    // 2. Correctly aligned
    return (p >= start) && (p < end) && (((p - start) % BLOCK_ALLOCATOR_SIZE) == 0);
}

static uint16_t _get_block_number_by_ptr(balloc_t* state, void* ptr) {
    const uintptr_t start = (uintptr_t)state->pool;
    const uintptr_t p = (uintptr_t)ptr;

    return (p - start) / BLOCK_ALLOCATOR_SIZE;
}
