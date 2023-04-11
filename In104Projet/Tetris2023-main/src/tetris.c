#include "tetris.h"


void draw_playing_field() {

    // loop var
    int i;

    // Set rendering clear color
    // This sets the 'background color'
    SDL_SetRenderDrawColor(render, 204, 192, 179, 255);

    // Clear the render
    // 'set' background color defined in SDL_SetRenderDrawColor(...)
    SDL_RenderClear(render);


    i = PLAYFIELD_HEIGHT * PLAYFIELD_WIDTH;
    while (i --> 0)
        set_playfield(i % PLAYFIELD_WIDTH, i / PLAYFIELD_WIDTH, playfield[i]);


    // Update the screen
    setRenderChanged();
}

Uint32 auto_drop_timer(Uint32 interval, void *param) {

}

void initTetris() {


}

void lockTetromino() {

}

void render_score() {

}

void updateTetris() {

}

void spawn_tetromino() {

}

bool can_render_tetromino(Tetromino_Movement tetra_request, uint8_t block_render_queue[]) {

    return true;
}

bool render_current_tetromino(Tetromino_Movement tetra_request) {

    return false;
}

bool render_tetromino(Tetromino_Movement tetra_request, uint8_t current_coords[]) {

    return true;
}

Color_Block get_playfield(uint8_t x, uint8_t y) {

}

void set_playfield(uint8_t x, uint8_t y, Color_Block color) {

}
