//shm_allocator.c

#include <errno.h>
#include "shm_allocator.h"

void shm_allocator_init(struct shm_allocator_info *allocator,
		const int64_t base_offset, const int total_size)
{
    allocator->offset.base = base_offset;
    allocator->size.total = total_size;
    allocator->offset.end = base_offset + total_size;

    shm_allocator_reset(allocator);
}

void shm_allocator_reset(struct shm_allocator_info *allocator)
{
    allocator->size.used = 0;
    allocator->offset.free = allocator->offset.base;
}

int64_t shm_allocator_alloc(struct shm_allocator_info *allocator,
        const int size)
{
    int64_t ptr_offset;
    if (allocator->offset.end - allocator->offset.free < size) {
        return -1;
    }

    ptr_offset = allocator->offset.free;
    allocator->offset.free += size;
    allocator->size.used += size;
    return ptr_offset;
}

