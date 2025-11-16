#include <paging.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct PageTable *create_page_table(size_t size) {
    struct PageTable *pt = (struct PageTable *)malloc(sizeof(struct PageTable));
    pt->entries = (uintptr_t *)malloc(size * sizeof(uintptr_t));
    pt->size = size;
    pt->curr = 0;
    memset(pt->entries, 0, size * sizeof(uintptr_t));
    return pt;
}

void destroy_page_table(struct PageTable *pt) {
    free(pt->entries);
    free(pt);
}

bool is_frame_unused(struct PageTable *pt, size_t frame_idx) {
    for (size_t j = 0; j < pt->size; j++) {
        if (FRAME_SIZE * frame_idx == pt->entries[j]) {
            return false;
        }
    }
    return true;
}

void print_page_table(struct PageTable *pt) {
    LOG_INFO("size: %zu", pt->size);
    LOG_INFO("curr: %zu", pt->curr);
    for (size_t i = 0; i < pt->size; i++) {
        if (i % 16 == 0)
            printf("\n");
        printf("%8p ", (void *)pt->entries[i]);
        // printf("0x%8" PRIxPTR " ", pt->entries[i]);
    }
    printf("\n");
}

// find a unused frame and map it to given virutal address
void map_frame_at_addr(struct PageTable *page_table, virt_addr_t virt_addr) {
    if (virt_addr == 0) {
        LOG_ERROR("Attempted to map guard page (0x0) to a valid physical frame");
        return;
    }

    size_t page_idx = virt_addr / PAGE_SIZE;
    size_t total_frames = DEFAULT_MEMORY_SIZE / FRAME_SIZE;

    // TODO: Handle out of memory infinite loop
    while (1) {
        last_frame_id++;
        if (last_frame_id == total_frames) {
            last_frame_id = 0;
        }

        if (is_frame_unused(page_table, last_frame_id)) {
            page_table->entries[page_idx] = FRAME_SIZE * last_frame_id;
            return;
        }
    }
}

void unmap_frame(struct PageTable *pt, virt_addr_t virt_addr) {
    size_t page_idx = virt_addr / PAGE_SIZE;
    pt->entries[page_idx] = 0;
}
