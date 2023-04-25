#include "graphics.h"
#include "defs.h"


void init_graphics() {
    render_changed = false //the game hasn't started, open a window
    window = SDL_CreateWindow(WINDOW_TITLE, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, flags);
    //window is already defined in init.h (its type: SDL_Window*)
    if (window==NULL) { //if the previous function did not work, window = NULL, so, an error message pops up.
        fprintf(stderr,
                "\nTTF_Init Error:  %s\n",
                SDL_GetError());
        exit(1);
    }
}

void setRenderChanged() {
    render_changed = true;
} //the function changes the bool render_changed to true, meaning that the render has to be changed

void preRender() {
    SDL_SetRenderTarget(render, display);
}  //ok

void updateRender() {

    // lazily update the screen only if render operations are queued
    if(render_changed) {

    }
}

void draw_block(uint8_t x, uint8_t y, uint32_t color) {


}

void cleanup_graphics() {
    SDL_DestroyRenderer(render);
    SDL_DestroyWindow(window);
} //ok
