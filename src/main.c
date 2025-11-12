#include <assert.h>
#include <inttypes.h>
#include <paging.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define DEFAULT_MEMORY_SIZE (1024 * 1024) // 1MB
#define DEFAULT_PAGE_TABLE_SIZE DEFAULT_MEMORY_SIZE / FRAME_SIZE

static_assert(DEFAULT_MEMORY_SIZE % FRAME_SIZE == 0,
              "DEFAULT_MEMORY_SIZE size should be divisible by FRAME_SIZE");

unsigned char *phy_mem = NULL;

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

// add an entry to page table at next available place
size_t add_entry_to_page_table(struct PageTable *pt, uintptr_t frame_addr) {
    while (pt->entries[pt->curr] != 0) {
        // TODO: detect page table full
        pt->curr++;
        if (pt->curr == pt->size)
            pt->curr = 0;
    }
    pt->entries[pt->curr] = frame_addr;
    return pt->curr;
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

// find a unused frame in memory and map it in page table
size_t map_frame(struct PageTable *page_table) {
    for (size_t frame_idx = 0; frame_idx < DEFAULT_PAGE_TABLE_SIZE; frame_idx++) {
        if (is_frame_unused(page_table, frame_idx)) {
            LOG_INFO("empty frame found: frame_id %zu", frame_idx);
            uintptr_t relative_frame_addr = FRAME_SIZE * frame_idx;
            size_t pt_idx = add_entry_to_page_table(page_table, relative_frame_addr);
            return pt_idx;
        }
    }
    assert("[FATAL] No unused frame found. Memory is full!");
    return 0;
}

void unmap_frame(struct PageTable *pt, size_t page_idx) {
    pt->entries[page_idx] = 0;
}

struct Proc *create_proc() {
    struct Proc *new_proc = (struct Proc *)malloc(sizeof(struct Proc));
    new_proc->page_table = create_page_table(DEFAULT_PAGE_TABLE_SIZE);
    return new_proc;
}

void destroy_proc(struct Proc *proc) {
    destroy_page_table(proc->page_table);
    free(proc);
}

int main() {
    LOG_INFO("arch: %d bit", 8 * (int)sizeof(uintptr_t));

    phy_mem = malloc(DEFAULT_PAGE_TABLE_SIZE);

    LOG_INFO("default table size: %d pages", DEFAULT_PAGE_TABLE_SIZE);
    struct Proc *proc = create_proc();
    LOG_INFO("pg table: %p", proc->page_table->entries);

    while (1) {
        size_t page_idx = map_frame(proc->page_table);
        LOG_INFO("new page index: %zu", page_idx);
        print_page_table(proc->page_table);
        usleep(1000 * 1000);
        printf("---------------------------------------------------------------------\n");

        size_t page_idx2 = map_frame(proc->page_table);
        LOG_INFO("new page index: %zu", page_idx2);
        print_page_table(proc->page_table);
        usleep(1000 * 1000);
        // unmap_frame(&page_table, page_idx2);
        unmap_frame(proc->page_table, page_idx);
        printf("---------------------------------------------------------------------\n");
    }

    return 0;
}
