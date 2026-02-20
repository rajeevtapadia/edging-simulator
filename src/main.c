#include <paging.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static_assert(DEFAULT_MEMORY_SIZE % FRAME_SIZE == 0,
              "DEFAULT_MEMORY_SIZE size should be divisible by FRAME_SIZE");

unsigned char *phy_mem = NULL;
size_t last_frame_id = 0;
struct ExecLog *exec_log = NULL;
struct FrameDBEntry frame_db[DEFAULT_FRAME_COUNT];

int main() {
    LOG_INFO("arch: %d bit", 8 * (int)sizeof(uintptr_t));

    phy_mem = malloc(DEFAULT_MEMORY_SIZE);
    exec_log = create_exec_log();

    LOG_INFO("default table size: %d pages", DEFAULT_PAGE_TABLE_SIZE);
    struct Proc *proc1 = create_proc("proc 1");

    printf("Select visualisation:\n");
    printf("1. Two processes accessing physical memory\n");
    printf("2. Virtual memory with memory inspector\n");
    printf("Enter choice (1 or 2): ");

    switch (getchar()) {
    case '1':
        struct Proc *proc2 = create_proc("proc 2");
        multi_process_visualisation(proc1, proc2);
        destroy_proc(proc2);
        break;
    case '2':
        memory_inspector_visualisation(proc1);
        break;
    default:
        printf("Invalid choice\n");
    }

    destroy_proc(proc1);
    destroy_exec_log(exec_log);
    free(phy_mem);

    return 0;
}
