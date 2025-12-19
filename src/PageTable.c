#include <paging.h>
#include <stdint.h>
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
            uintptr_t phy_addr = FRAME_SIZE * last_frame_id;
            // zero out a frame before mapping it
            memset(&phy_mem[phy_addr], 0, PAGE_SIZE);
            page_table->entries[page_idx] = phy_addr;
            return;
        }
    }
}

void unmap_page_by_virtual_addr(struct PageTable *pt, virt_addr_t virt_addr) {
    size_t page_idx = virt_addr / PAGE_SIZE;
    unmap_page_by_page_idx(pt, page_idx);
}

// TODO: Bad API design, these functions are not supposed to be called directly
// NEED a prcess level abstraction for these
// also then it would be possible to log the process in the entry
void unmap_page_by_page_idx(struct PageTable *pt, size_t page_idx) {
    pt->entries[page_idx] = 0;

    struct ExecLogEntry entry = {.action = UNMAP, .virt_addr = page_idx * PAGE_SIZE};
    push_to_exec_log(exec_log, entry);
}
