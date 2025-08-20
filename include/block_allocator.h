#ifndef BLOCK_ALLOCATOR_H
#define BLOCK_ALLOCATOR_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BLOCK_ALLOCATOR_SIZE
#define BLOCK_ALLOCATOR_SIZE (64)  // Block size, bytes
#endif

#ifndef BLOCK_ALLOCATOR_COUNT
#define BLOCK_ALLOCATOR_COUNT (128)  // Number of blocks
#endif

#ifndef BLOCK_ALLOCATOR_MEM_ALIGN
#define BLOCK_ALLOCATOR_MEM_ALIGN (sizeof(uintptr_t))  // Align
#endif

#define BLOCK_ALLOCATOR_POOL_SIZE (BLOCK_ALLOCATOR_SIZE * BLOCK_ALLOCATOR_COUNT)
#define BLOCK_ALLOCATOR_BITMAP_SIZE \
    (BLOCK_ALLOCATOR_COUNT % 8 == 0 ? BLOCK_ALLOCATOR_COUNT / 8 : BLOCK_ALLOCATOR_COUNT / 8 + 1)

typedef struct alloc {
    uint8_t pool[BLOCK_ALLOCATOR_POOL_SIZE] __attribute__((aligned(BLOCK_ALLOCATOR_MEM_ALIGN)));
    uint8_t free_bitmap[BLOCK_ALLOCATOR_BITMAP_SIZE];
} balloc_t;

void balloc_enter_critical(balloc_t* state) __attribute__((weak));
void balloc_exit_critical(balloc_t* state) __attribute__((weak));

bool balloc_init(balloc_t* state);
void* balloc_allocate(balloc_t* state);
bool balloc_deallocate(balloc_t* state, void* ptr);
uint16_t balloc_get_size(balloc_t* state);
uint16_t balloc_get_capacity(balloc_t* state);
bool balloc_deinit(balloc_t* state);

#ifdef __cplusplus
}
#endif

#endif  // BLOCK_ALLOCATOR_H
