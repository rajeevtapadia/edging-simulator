#include <math.h>
#include <paging.h>
#include <raylib.h>
#include <stddef.h>
#include <stdio.h>

#define SCREEN_WIDTH 16
#define SCREEN_HEIGHT 9

#define BOX_WIDTH 130
#define BOX_HEIGHT 80

#define TOP_PADDING 80
#define LEFT_PADDING 150
#define RIGHT_PADDING 150

#define NORMAL_LINE_THICKNESS 2

const int factor = 100;
int width = SCREEN_WIDTH * factor;
int height = SCREEN_HEIGHT * factor;

size_t sim_page_size = 10;
size_t sim_frame_count = 10;

Color BG_COLOR = (Color){20, 20, 21, 255};
Color TEXT_COLOR = (Color){174, 174, 209, 255};
Color TITLE_COLOR = (Color){187, 157, 189, 255};
Color BOX_BOUNDRY_COLOR = (Color){135, 135, 135, 255};

static struct Proc *proc1 = NULL;
static struct Proc *proc2 = NULL;

void draw_page_table(struct Proc *proc, size_t offset_x) {
    int font_size = 20;
    int offset_y = TOP_PADDING;
    for (size_t i = 0; i < sim_page_size; i++) {
        Rectangle rec = {.x = offset_x,
                         .y = i * BOX_HEIGHT + offset_y,
                         .height = BOX_HEIGHT,
                         .width = BOX_WIDTH};
        DrawRectangleLinesEx(rec, NORMAL_LINE_THICKNESS, BOX_BOUNDRY_COLOR);

        char buf[20];
        sprintf(buf, "%zu: %p", i, (void *)proc->page_table->entries[i]);
        DrawText(buf, offset_x + 10,
                 i * BOX_HEIGHT + BOX_HEIGHT / 2 - font_size / 2 + offset_y, font_size,
                 TEXT_COLOR);
    }
}

void draw_physical_memory() {
    int font_size = 20;
    int offset_x = width / 2 - BOX_WIDTH / 2;
    int offset_y = TOP_PADDING;
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

void draw_arrow_head(Vector2 arrow_start, Vector2 arrow_end) {
    // calculate slope of arrow body
    float dx = arrow_end.x - arrow_start.x;
    float dy = arrow_end.y - arrow_start.y;

    // calculate angle made by arrow body to positive x axis
    float angle = atan2f(dy, dx);

    float theta = .4f;
    float head_length = -50;

    /*
     * find coordinates of two points extending backwards from arrow tip
     * cos/sin: convert angle to x/y coordinates
     * head_length: extend the points backwards by head_length
     * add arrow_end.x/y: shift the points relative to arrow tip
     */
    float x1 = arrow_end.x + (head_length * cosf(angle + theta));
    float y1 = arrow_end.y + (head_length * sinf(angle + theta));

    float x2 = arrow_end.x + (head_length * cosf(angle - theta));
    float y2 = arrow_end.y + (head_length * sinf(angle - theta));

    DrawTriangle(arrow_end, (Vector2){x1, y1}, (Vector2){x2, y2}, BOX_BOUNDRY_COLOR);
}

void draw_arrow_from_proc1(size_t page_idx, size_t frame_idx) {
    Vector2 arrow_start, arrow_end;
    arrow_start.x = LEFT_PADDING + BOX_WIDTH;
    arrow_start.y = TOP_PADDING + (page_idx * BOX_HEIGHT) + BOX_HEIGHT / 2.f;

    arrow_end.x = width / 2.f - BOX_WIDTH / 2.f;
    arrow_end.y = TOP_PADDING + (frame_idx * BOX_HEIGHT) + BOX_HEIGHT / 2.f;

    DrawLineEx(arrow_start, arrow_end, NORMAL_LINE_THICKNESS, BOX_BOUNDRY_COLOR);
    draw_arrow_head(arrow_start, arrow_end);
}

void draw_arrow_from_proc2(size_t page_idx, size_t frame_idx) {
    Vector2 arrow_start, arrow_end;
    arrow_start.x = width - RIGHT_PADDING - BOX_WIDTH;
    arrow_start.y = TOP_PADDING + (page_idx * BOX_HEIGHT) + BOX_HEIGHT / 2.f;

    arrow_end.x = width / 2.f + BOX_WIDTH / 2.f;
    arrow_end.y = TOP_PADDING + (frame_idx * BOX_HEIGHT) + BOX_HEIGHT / 2.f;

    DrawLineEx(arrow_start, arrow_end, NORMAL_LINE_THICKNESS, BOX_BOUNDRY_COLOR);
    draw_arrow_head(arrow_start, arrow_end);
}

void render_loop() {
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BG_COLOR);
        DrawText("Multi-Process Physical Memory Access", 380, width / 64, 40,
                 TITLE_COLOR);

        size_t left_padding = LEFT_PADDING;
        size_t right_padding = width - left_padding - BOX_WIDTH;
        draw_page_table(proc1, left_padding);
        draw_page_table(proc2, right_padding);
        draw_physical_memory();

        for (size_t i = 0; i < sim_page_size; i++) {
            if (proc1->page_table->entries[i] != 0) {
                size_t frame_idx = proc1->page_table->entries[i] >> 12;
                draw_arrow_from_proc1(i, frame_idx);
            }
        }

        for (size_t i = 0; i < sim_page_size; i++) {
            if (proc1->page_table->entries[i] != 0) {
                size_t frame_idx = proc2->page_table->entries[i] >> 12;
                draw_arrow_from_proc2(i, frame_idx);
            }
        }

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
