#include "paging.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * ExecLog is a dyanmically sized stack that stores
 * 1. executed instructions
 * 2. the memory changed by them
 */

#define DEFAULT_EXEC_LOG_SIZE 5

struct ExecLog *create_exec_log() {
    struct ExecLog *new_log = (struct ExecLog *)malloc(sizeof(struct ExecLog));
    new_log->top = -1;
    new_log->stack = (struct ExecLogEntry *)malloc(sizeof(struct ExecLogEntry) *
                                                   DEFAULT_EXEC_LOG_SIZE);
    new_log->size = DEFAULT_EXEC_LOG_SIZE;
    return new_log;
};

static void resize_exec_log(struct ExecLog *log, size_t new_size) {
    assert(log != NULL && "ExecLog pointer is null");

    if (new_size == log->size) {
        return;
    }

    log->stack = realloc(log->stack, new_size * sizeof(struct ExecLogEntry));
    assert(log->stack != NULL);
    log->size = new_size;
}

void push_to_exec_log(struct ExecLog *log, struct ExecLogEntry entry) {
    assert(log != NULL && "ExecLog pointer is null");

    if (log->top == (int)log->size - 1) {
        resize_exec_log(log, log->size * 1.5);
    }
    log->top++;
    log->stack[log->top] = entry;
}

void pop_to_exec_log(struct ExecLog *log) {
    assert(log != NULL && "ExecLog pointer is null");

    log->size--;
}

struct ExecLogEntry peek_to_exec_log(struct ExecLog *log) {
    assert(log != NULL && "ExecLog pointer is null");

    return log->stack[log->top];
}

void destroy_exec_log_entry(struct ExecLogEntry *entry) {
    free(entry);
}

void destroy_exec_log(struct ExecLog *log) {
    free(log->stack);
    free(log);
}

void print_exec_stack(struct ExecLog *log) {
    LOG_INFO("--------------------ExecLog Stack--------------------");
    for (int i = log->top; i >= 0; i--) {
        struct ExecLogEntry entry = log->stack[i];

        switch (entry.action) {
        case WRITE:
            printf(">> Action: WRITE, ");
            printf("old data: %c, new data: %c, at %p, did_map: %d ", entry.old_data,
                   entry.new_data, (void *)entry.virt_addr, entry.did_map);
            break;
        case READ:
            printf(">> Action: READ, ");
            printf("at %p, ", (void *)entry.virt_addr);
            break;
        case UNMAP:
            printf(">> Action: UNMAP, ");
            printf("addr: %p, ", (void *)entry.virt_addr);
            break;

        default:
            assert("Invalid action");
        }
        // TODO: handle this case
        assert(entry.proc != NULL && "No process found in the log entry");
        printf("by proc: %s\n", entry.proc->name);
    }
    LOG_INFO("-----------------------------------------------------");
}
