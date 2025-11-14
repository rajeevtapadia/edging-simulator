#include <assert.h>
#include <inttypes.h>
#include <paging.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
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
virt_addr_t add_entry_to_page_table(struct PageTable *pt, uintptr_t frame_addr) {
    while (pt->entries[pt->curr] != 0) {
        // TODO: detect page table full
        pt->curr++;
        if (pt->curr == pt->size)
            pt->curr = 0;
    }
    pt->entries[pt->curr] = frame_addr;
    virt_addr_t virt_addr = pt->curr * PAGE_SIZE;
    return virt_addr;
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
virt_addr_t map_frame(struct PageTable *page_table) {
    for (size_t frame_idx = 0; frame_idx < DEFAULT_PAGE_TABLE_SIZE; frame_idx++) {
        if (is_frame_unused(page_table, frame_idx)) {
            LOG_INFO("empty frame found: frame_id %zu", frame_idx);
            uintptr_t relative_frame_addr = FRAME_SIZE * frame_idx;
            return add_entry_to_page_table(page_table, relative_frame_addr);
        }
    }
    assert("[FATAL] No unused frame found. Memory is full!");
    return 0;
}

void unmap_frame(struct PageTable *pt, virt_addr_t virt_addr) {
    size_t page_idx = virt_addr / PAGE_SIZE;
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

uintptr_t convert_virtual_addr_to_physical_addr(struct PageTable *pt,
                                                virt_addr_t virt_addr) {
    size_t page_idx = virt_addr / PAGE_SIZE;
    assert(pt->entries[page_idx] != 0 && "[FATAL] Invalid page");

    uintptr_t frame_addr = pt->entries[page_idx];
    uintptr_t offset = virt_addr & (PAGE_SIZE - 1);
    return frame_addr + offset;
}

unsigned char access_memory(struct Proc *proc, virt_addr_t virt_addr) {
    size_t page_idx = virt_addr / PAGE_SIZE;
    if (proc->page_table->entries[page_idx] != 0) {
        // page hit
        uintptr_t phy_addr =
            convert_virtual_addr_to_physical_addr(proc->page_table, virt_addr);
        return phy_mem[phy_addr];
    } else {
        // page fault
    }

    assert("[FATAL] UNIMPLEMENTED");
    return 0;
}

void set_memory(struct Proc *proc, virt_addr_t virt_addr, unsigned char data) {
    size_t page_idx = virt_addr / PAGE_SIZE;
    if (proc->page_table->entries[page_idx] != 0) {
        uintptr_t phy_addr =
            convert_virtual_addr_to_physical_addr(proc->page_table, virt_addr);
        phy_mem[phy_addr] = data;
        // *(unsigned char *)phy_addr = data;
    } else {
        // page fault
        assert("[FATAL] UNIMPLEMENTED");
    }
}

int main() {
    LOG_INFO("arch: %d bit", 8 * (int)sizeof(uintptr_t));

    phy_mem = malloc(DEFAULT_PAGE_TABLE_SIZE);

    LOG_INFO("default table size: %d pages", DEFAULT_PAGE_TABLE_SIZE);
    struct Proc *proc = create_proc();
    LOG_INFO("pg table: %p", proc->page_table->entries);

    // while (1) {
    for (int i = 0; i < 2; i++) {
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
        // unmap_frame(proc->page_table, page_idx);
        printf("---------------------------------------------------------------------\n");
    }

    uintptr_t test_addr = 0x1FFF;
    char *str = "ligma balls";
    for (size_t i = 0; i < strlen(str); i++)
        set_memory(proc, test_addr + i, str[i]);

    for (size_t i = 0; i < strlen(str); i++)
        printf("%c ", access_memory(proc, test_addr + i));
    printf("\n");
    destroy_proc(proc);
    proc = NULL;

    return 0;
}
