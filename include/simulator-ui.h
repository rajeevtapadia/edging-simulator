#ifndef SIMULATOR_UI_H

#include "paging.h"
#include <raylib.h>
#include <stddef.h>

#define SCALING_FACTOR 100
#define SCREEN_WIDTH 16 * SCALING_FACTOR
#define SCREEN_HEIGHT 9 * SCALING_FACTOR

#define BOX_WIDTH 130
#define BOX_HEIGHT 60

#define TOP_PADDING 80
#define LEFT_PADDING 150
#define RIGHT_PADDING 150

#define NORMAL_LINE_THICKNESS 2

#define DIVIDER_POS 700

static const Color BG_COLOR = (Color){20, 20, 21, 255};
static const Color TEXT_COLOR = (Color){174, 174, 209, 255};
static const Color TITLE_COLOR = (Color){187, 157, 189, 255};
static const Color BOX_BOUNDRY_COLOR = (Color){135, 135, 135, 255};

extern size_t sim_page_size;
extern size_t sim_frame_count;

struct FocusCtx {
    size_t page_table_idx;
    struct Proc *proc;
    bool is_selected;
};

struct Operation {
    enum Action action;
    struct Proc *proc;
    unsigned char data;
    virt_addr_t virt_addr;
};

struct TestCase {
    struct Operation *ops;
    size_t operation_count;
    size_t curr_operation_idx;
};

extern struct FocusCtx focus;
extern struct TestCase test_case;

// ui-utils.c
void draw_arrow_from_proc_left(size_t page_idx, size_t frame_idx);
void draw_arrow_from_proc_right(size_t page_idx, size_t frame_idx);
void draw_physical_memory();
void draw_page_table(struct Proc *proc, size_t offset_x);
void draw_arrow_head(Vector2 arrow_start, Vector2 arrow_end);
void draw_divider();
void draw_text_section();

int page_table_idx_at_cursor();
void print_operation(struct Operation *op);
void perform_operation(struct Operation *op);
void operation_to_str(struct Operation *op, size_t idx, char *buf, size_t size);
char *action_to_str(enum Action action);

#endif // SIMULATOR_UI_H
