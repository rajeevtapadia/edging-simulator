#include <paging.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static_assert(DEFAULT_MEMORY_SIZE % FRAME_SIZE == 0,
              "DEFAULT_MEMORY_SIZE size should be divisible by FRAME_SIZE");

unsigned char *phy_mem = NULL;
size_t last_frame_id = 0;

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

    multi_process_visualisation(proc, proc);

    destroy_proc(proc);
    proc = NULL;

    return 0;
}
