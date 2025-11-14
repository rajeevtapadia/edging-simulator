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
size_t last_frame_id = 0;

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

struct Proc *create_proc(char *name) {
    struct Proc *new_proc = (struct Proc *)malloc(sizeof(struct Proc));
    new_proc->name = (char *)malloc(strlen(name));
    memcpy(new_proc->name, name, strlen(name));
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

    // check for page fault
    if (proc->page_table->entries[page_idx] == 0) {
        LOG_ERROR("Page fault while accessing %p", (void *)virt_addr);
        LOG_ERROR("%s: Segmentation fault", proc->name);
        return -1;
    }

    uintptr_t phy_addr =
        convert_virtual_addr_to_physical_addr(proc->page_table, virt_addr);
    return phy_mem[phy_addr];
    assert("[FATAL] UNIMPLEMENTED");
    return 0;
}

void set_memory(struct Proc *proc, virt_addr_t virt_addr, unsigned char data) {
    size_t page_idx = virt_addr / PAGE_SIZE;

    // check for page fault
    if (proc->page_table->entries[page_idx] == 0) {
        map_frame_at_addr(proc->page_table, virt_addr);
    }

    uintptr_t phy_addr =
        convert_virtual_addr_to_physical_addr(proc->page_table, virt_addr);
    phy_mem[phy_addr] = data;
}

int main() {
    LOG_INFO("arch: %d bit", 8 * (int)sizeof(uintptr_t));

    phy_mem = malloc(DEFAULT_PAGE_TABLE_SIZE);

    LOG_INFO("default table size: %d pages", DEFAULT_PAGE_TABLE_SIZE);
    struct Proc *proc = create_proc("proc 1");
    LOG_INFO("pg table: %p", proc->page_table->entries);


    uintptr_t test_addr = 0x1FFF;
    print_page_table(proc->page_table);
    printf("---------------------------------------------------------------------\n");

    char *str = "ligma balls";
    for (size_t i = 0; i < strlen(str); i++)
        set_memory(proc, test_addr + i, str[i]);

    print_page_table(proc->page_table);
    for (size_t i = 0; i < strlen(str); i++)
        printf("%c ", access_memory(proc, test_addr + i));
    printf("\n");

    // try to read invalid memory
    access_memory(proc, 0x696969);

    destroy_proc(proc);
    proc = NULL;

    return 0;
}
