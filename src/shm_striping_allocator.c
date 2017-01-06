//shm_striping_allocator.c

#include <errno.h>
#include "sched_thread.h"
#include "shm_striping_allocator.h"

void shm_striping_allocator_init(struct shm_striping_allocator *allocator,
		const struct shm_segment_striping_pair *ssp_index,
		const int64_t base_offset, const int total_size)
{
    allocator->index = *ssp_index;
    allocator->offset.base = base_offset;
    allocator->size.total = total_size;
    allocator->offset.end = base_offset + total_size;

    shm_striping_allocator_reset(allocator);
}

int64_t shm_striping_allocator_alloc(struct shm_striping_allocator *allocator,
        const int size)
{
    int64_t ptr_offset;
    if (allocator->offset.end - allocator->offset.free < size) {
        allocator->fail_times++;
        return -1;
    }

    allocator->last_alloc_time = g_current_time;
    ptr_offset = allocator->offset.free;
    allocator->size.used += size;
    allocator->offset.free += size;
    return ptr_offset;
}
