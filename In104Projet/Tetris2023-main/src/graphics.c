#include "graphics.h"


void init_graphics() {
    render_changed = false //the game hasn't started, open a window
    window = SDL_CreateWindow(
        // title of the window 
        WINDOW_TITLE, 
        
        // initial postion of the window 
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
        
        //initial size of the window 
        WINDOW_WIDTH, WINDOW_HEIGHT, flags);

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

void updateRender() { // ?

    // lazily update the screen only if render operations are queued
    if(render_changed) {

    }
}

void draw_block(uint8_t x, uint8_t y, uint32_t color) {

    assert(x>=0 && y>=0 && x<PLAYFIELD_HEIGHT && y<PLAYFIELD_WEIGHT)

    //change of scale (affine transformation) from (x,y) to the coords of each corner of a block : 
    // coords block upper left
    uint16_t x_bul = x*(BLOCK_SIZE + 1) + 1; 
    uint16_t y_bul = y*(BLOCK_SIZE + 1) + 1; 

    // coords block upper right
    uint16_t x_bur = x_bul + BLOCK_SIZE;
    uint16_t y_bur = y_bul;

    // coords block bottom left
    uint16_t x_bbl = x_bul;
    uint16_t y_bbl = y_bul - BLOCK_SIZE; 

    // coords block bottom right
    uint16_t x_bbr = x_bur; 
    uint16_t y_bbr = y_bul; 
    
    boxColor(render, x_bul, y_bul, x_bbr, y_bbr, color);

    // making the grill : we are going to draw for each block : a line at the bottom of the block and another line at the right of the block to draw all the grill. 
    // There will only be missing the upper line for y = 0 and the left line for x = 0, which we are going to draw first.

    if(y = 0) {
        aalineRGBA(render, x_bul, y_bul, x_bur, y_bur, 187, 173, 160, 255);
    }

    if(x = 0) {
        aalineRGBA(render, x_bul, y_bul, x_bbl, y_bbl, 187, 173, 160, 255);
    }

    // now drawing the two lines for the block as said previously 

    // line at the right of the block (from upper right to bottom right)
    aalineRGBA(render, x_bur, y_bur, x_bbr, y_bbr, 187, 173, 160, 255);

    // line at the bottom of the block (from bottom left to bottom right)
    aalineRGBA(render, x_bbl, y_bbl, x_bbr, y_bbr, 187, 173, 160, 255);

    setRenderChanged();
}

void cleanup_graphics() {
    SDL_DestroyRenderer(render);
    SDL_DestroyWindow(window);
} //ok
