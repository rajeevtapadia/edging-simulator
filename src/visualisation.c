#include <paging.h>
#include <raylib.h>
#include <stddef.h>
#include <stdio.h>

#define SCREEN_WIDTH 16
#define SCREEN_HEIGHT 9

#define BOX_WIDTH 130
#define BOX_HEIGHT 80

const int factor = 100;
int width = SCREEN_WIDTH * factor;
int height = SCREEN_HEIGHT * factor;

Color BG_COLOR = (Color){20, 20, 21, 255};
Color TEXT_COLOR = (Color){174, 174, 209, 255};
Color TITLE_COLOR = (Color){187, 157, 189, 255};
Color BOX_BOUNDRY_COLOR = (Color){135, 135, 135, 255};

static struct Proc *proc1 = NULL;
static struct Proc *proc2 = NULL;

void draw_page_table(struct Proc *proc, size_t offset_x) {
    size_t sim_page_size = 10;
    int font_size = 20;
    int offset_y = 80;
    for (size_t i = 0; i < sim_page_size; i++) {
        Rectangle rec = {.x = offset_x,
                         .y = i * BOX_HEIGHT + offset_y,
                         .height = BOX_HEIGHT,
                         .width = BOX_WIDTH};
        DrawRectangleLinesEx(rec, 2, BOX_BOUNDRY_COLOR);

        char buf[20];
        sprintf(buf, "%zu: %p", i, (void *)proc->page_table->entries[i]);
        DrawText(buf, offset_x + 10,
                 i * BOX_HEIGHT + BOX_HEIGHT / 2 - font_size / 2 + offset_y, font_size,
                 TEXT_COLOR);
    }
}

void draw_physical_memory() {
    size_t sim_frame_count = 10;
    int font_size = 20;
    int offset_x = width / 2 - BOX_WIDTH / 2;
    int offset_y = 80;
    for (size_t i = 0; i < sim_frame_count; i++) {
        Rectangle rec = {.x = offset_x,
                         .y = i * BOX_HEIGHT + offset_y,
                         .height = BOX_HEIGHT,
                         .width = BOX_WIDTH};
        DrawRectangleLinesEx(rec, 2, BOX_BOUNDRY_COLOR);

        char buf[20];
        sprintf(buf, "0x%lx", i << 12);
        DrawText(buf, offset_x + 10,
                 i * BOX_HEIGHT + BOX_HEIGHT / 2 - font_size / 2 + offset_y, font_size,
                 TEXT_COLOR);
    }
}

void render_loop() {
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BG_COLOR);
        DrawText("Multi-Process Physical Memory Access", 380, width / 64, 40,
                 TITLE_COLOR);

        size_t left_padding = 150;
        size_t right_padding = width - left_padding - BOX_WIDTH;
        draw_page_table(proc1, left_padding);
        draw_page_table(proc2, right_padding);
        draw_physical_memory();
        EndDrawing();
    }
}

void init_visualsation() {
    InitWindow(width, height, "Paging Simulator");

    render_loop();

    CloseWindow();
}

void multi_process_visualisation(struct Proc *_proc1, struct Proc *_proc2) {
    (void)_proc2;
    proc1 = _proc1;
    proc2 = _proc2;

    init_visualsation();
}
