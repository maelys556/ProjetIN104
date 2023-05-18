#include "SDL.h"
#include "SDL_ttf.h"
#include <stdbool.h>
#include <string.h>
#include "SDL_pixels.h"

#define FRAME_DELAY 1000 / 60
#define MIN_FRAME_DELAY 10
#define FRAME_DELAY_DECREMENT 10

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define BLOCK_SIZE 20
#define BOARD_WIDTH 10
#define BOARD_HEIGHT 20
#define EMPTY_BLOCK 0
#define TETROMINO_SIZE 4
#define NUM_TETROMINOS 7
#define PLAYFIELD_HEIGHT 22
#define PLAYFIELD_WIDTH 10
#define WINDOW_HEIGHT PLAYFIELD_HEIGHT * (BLOCK_SIZE + 1) + 1
#define WINDOW_WIDTH PLAYFIELD_WIDTH * (BLOCK_SIZE + 1) + 1

bool game_over = false;
bool render_changed = false;

typedef struct {
    int x, y;
} Point;

typedef struct {
    char grid[BOARD_HEIGHT][BOARD_WIDTH];
} Board;

typedef struct {
    int x;
    int y;
    int shape[4][4];
    int rotation;
    int height;
    int width;
} tetromino;

typedef struct {
    Board grid;
    tetromino current_tetromino;
    int frame_delay;
    bool game_over;
    int score;
} GameState;

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
TTF_Font* font = NULL;
GameState game;

bool init() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("Erreur lors de l'initialisation de SDL: %s\n", SDL_GetError());
        return false;
    }

    window = SDL_CreateWindow("Tetris", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (window == NULL) {
        printf("Erreur lors de la création de la fenêtre: %s\n", SDL_GetError());
        return false;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {
        printf("Erreur lors de la création du rendu: %s\n", SDL_GetError());
        return false;
    }

    render_changed = false;

    if (TTF_Init() == -1) {
        printf("Erreur lors de l'initialisation de SDL_ttf: %s\n", TTF_GetError());
        return false;
    }

    // texture for renderer context
    SDL_Texture * display = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, WINDOW_WIDTH, WINDOW_HEIGHT);

    SDL_SetRenderTarget(renderer, display);

    font = TTF_OpenFont("font.ttf", 24);
    if (font == NULL) {
        printf("Erreur lors du chargement de la police de caractères: %s\n", TTF_GetError());
        return false;
    }

     memset(game.grid.grid, EMPTY_BLOCK, sizeof(game.grid.grid));
    return true;
}

//chercher à quoi ça sert :  et l'utiliser !!!
void preRender() { 
    SDL_Texture * display = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, WINDOW_WIDTH, WINDOW_HEIGHT);
    SDL_SetRenderTarget(renderer, display);
}

//chercher à quoi ça sert : et l'utiliser !!!!
void setRenderChanged() {
    render_changed = true;
}

void closeGame() {
    TTF_CloseFont(font);
    TTF_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void drawBlock(int x, int y, SDL_Color color) {
    SDL_Rect blockRect = { x * BLOCK_SIZE, y * BLOCK_SIZE, BLOCK_SIZE, BLOCK_SIZE };
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255);
    SDL_RenderFillRect(renderer, &blockRect);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderDrawRect(renderer, &blockRect);
}

// Dessine le Tetromino courant

void drawtetromino(tetromino* tetromino) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (((tetromino->shape[i][j] >> (i * 4 + j)) & 1) != 0) {
                drawBlock(tetromino->x + j, tetromino->y + i, (SDL_Color){ 255, 255, 0 });
            }
        }
    }
}

void drawBoard(Board* board) {
    for (int i = 0; i < BOARD_HEIGHT; i++) {
        for (int j = 0; j < BOARD_WIDTH; j++) {
            if (board->grid[i][j] != EMPTY_BLOCK) {
                drawBlock(j, i, (SDL_Color){ 255, 255, 255 });
            }
        }
    }
}

// Fonction pour faire tourner une pièce de Tetris
void rotatetetromino(tetromino* tetromino) {
    int temp = tetromino->width;
    tetromino->width = tetromino->height;
    tetromino->height = temp;
}

void movetetromino(tetromino* tetromino, int dx, int dy) {
    tetromino->x += dx;
    tetromino->y += dy;
}

int can_move_down(GameState *game) {
    const tetromino *tetromino = &(game->current_tetromino);
    const Board *board = &(game->grid);

    for (int i = 0; i < TETROMINO_SIZE; i++) {
        for (int j = 0; j < TETROMINO_SIZE; j++) {
            if (tetromino->shape[i][j] != 0) {
                int x = tetromino->x + j;
                int y = tetromino->y + i + 1;

                if (y >= BOARD_HEIGHT || board->grid[y][x] != 0) {
                    return 0; // Cannot move down
                }
            }
        }
    }

    return 1; // Can move down
}

int can_place_tetromino(GameState *game, tetromino *tetromino, Board *board) {
    // const Board *board = &(game->board);
    int x = tetromino->x;
    int y = tetromino->y;

    for (int i = 0; i < TETROMINO_SIZE; i++) {
        for (int j = 0; j < TETROMINO_SIZE; j++) {
            if (tetromino->shape[i][j] != 0) {
                int tx = x + j;
                int ty = y + i;

                if (tx < 0 || tx >= BOARD_WIDTH || ty >= BOARD_HEIGHT || board->grid[ty][tx] != 0) {
                    return 0; // Cannot place tetromino here
                }
            }
        }
    }

    return 1; // Can place tetromino here
}

tetromino generate_random_tetromino() {
    tetromino tetromino;

    // List of available tetrominos
    const int tetrominos[NUM_TETROMINOS][TETROMINO_SIZE][TETROMINO_SIZE] = {
        // I tetromino
        {
            {0, 0, 0, 0},
            {1, 1, 1, 1},
            {0, 0, 0, 0},
            {0, 0, 0, 0}
        },
        // J tetromino
        {
            {1, 0, 0},
            {1, 1, 1},
            {0, 0, 0}
        },
        // L tetromino
        {
            {0, 0, 1},
            {1, 1, 1},
            {0, 0, 0}
        },
        // O tetromino
        {
            {1, 1},
            {1, 1}
        },
        // S tetromino
        {
            {0, 1, 1},
            {1, 1, 0},
            {0, 0, 0}
        },
        // T tetromino
        {
            {0, 1, 0},
            {1, 1, 1},
            {0, 0, 0}
        },
        // Z tetromino
        {
            {1, 1, 0},
            {0, 1, 1},
            {0, 0, 0}
        }
    };

    // Generate a random index to select a tetromino from the list
    int index = rand() % NUM_TETROMINOS;

    // Copy the selected tetromino into the Tetromino structure
    for (int i = 0; i < TETROMINO_SIZE; i++) {
        for (int j = 0; j < TETROMINO_SIZE; j++) {
            tetromino.shape[i][j] = tetrominos[index][i][j];
        }
    }

    return tetromino;
}

int calculate_score(int lines_cleared) {
    int score = 0;

    switch (lines_cleared) {
        case 1:
            score = 100;
            break;
        case 2:
            score = 300;
            break;
        case 3:
            score = 500;
            break;
        case 4:
            score = 800;
            break;
        default:
            score = 0;
            break;
    }

    return score;
}

void find_complete_lines(const Board* board, int* complete_lines, int* num_complete_lines) {
    int i, j;
    *num_complete_lines = 0;

    for (i = 0; i < BOARD_HEIGHT; i++) {
        int line_complete = 1;

        for (j = 0; j < BOARD_WIDTH; j++) {
            if (board->grid[i][j] == 0) {
                line_complete = 0;
                break;
            }
        }

        if (line_complete) {
            complete_lines[*num_complete_lines] = i;
            (*num_complete_lines)++;
        }
    }
}

void clear_lines(char board[BOARD_HEIGHT][BOARD_WIDTH], int* lines_to_clear, int num_lines) {
    // Parcourir les lignes à effacer
    for (int i = 0; i < num_lines; i++) {
        int line_to_clear = lines_to_clear[i];
        // Déplacer toutes les lignes au-dessus de la ligne à effacer vers le bas
        for (int j = line_to_clear; j > 0; j--) {
            for (int k = 0; k < BOARD_WIDTH; k++) {
                board[j][k] = board[j - 1][k];
            }
        }
        // Effacer la première ligne en la remplissant de zéros
        for (int k = 0; k < BOARD_WIDTH; k++) {
            board[0][k] = 0;
        }
    }
}



void lock_tetromino(GameState *game, char grid[BOARD_HEIGHT][BOARD_WIDTH], tetromino tetromino) {
    int i, j;
    Board board = game->grid;
    
    // Verrouillage des blocs du tétrmino sur le plateau
    for (i = 0; i < TETROMINO_SIZE; i++) {
        for (j = 0; j < TETROMINO_SIZE; j++) {
            if (tetromino.shape[i][j] != EMPTY_BLOCK) {
                grid[tetromino.y + i][tetromino.x + j] = tetromino.shape[i][j];
            }
        }
    }
}


// Gestion des événements
void handleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            game_over = true;
        }
        else if (event.type == SDL_KEYDOWN) {
            switch (event.key.keysym.sym) {
                case SDLK_LEFT:
                    movetetromino(&(game.current_tetromino), -1, 0);
                    break;
                case SDLK_RIGHT:
                    movetetromino(&(game.current_tetromino), 1, 0);
                    break;
                case SDLK_DOWN:
                    movetetromino(&(game.current_tetromino), 0, 1);
                    break;
                case SDLK_SPACE:
                    rotatetetromino(&(game.current_tetromino));
                    break;
            }
        }
    }
}

// update :  chercher à quoi ça sert : 

void updateRender() {
    if(render_changed) {
        SDL_SetRenderTarget(renderer, NULL);
        SDL_Texture * display = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, WINDOW_WIDTH, WINDOW_HEIGHT);

        SDL_RenderClear(renderer); // Ajouter cette ligne pour effacer le rendu précédent

        SDL_RenderCopy(renderer, display, NULL, NULL);
        SDL_RenderPresent(renderer);
        render_changed = true;
    }
}



void update_game(GameState* game) {
    preRender();
    // Vérifier si la pièce actuelle peut descendre
    if (can_move_down(game)) { //     if (can_move_down(game->current_tetromino, game->grid)) {
        // Si oui, déplacer la pièce vers le bas
        movetetromino(&(game->current_tetromino), 0, 1);  // movetetromino(game->current_tetromino, DirectionDown);
    } else {
        Board board = game->grid;
        // Si la pièce ne peut pas descendre, la verrouiller dans la grille
        lock_tetromino(game, game->grid.grid, game->current_tetromino); //lock_tetromino(game->current_tetromino, game->grid);
        // Vérifier s'il y a des lignes complètes
//      int* complete_lines; 
//      int* num_complete_lines;
        int complete_lines[BOARD_HEIGHT];
        int num_complete_lines = 0;
//      int lines_to_clear = find_complete_lines(game->grid,complete_lines,num_complete_lines);
        find_complete_lines(&(game->grid), complete_lines, &num_complete_lines);
        if (num_complete_lines > 0) { //if (clear_lines > 0) {
            // Mettre à jour le score en fonction du nombre de lignes complètes
            game->score += calculate_score(num_complete_lines); //game->score += calculate_score(lines_to_clear);
            // Effacer les lignes complètes
            clear_lines(board.grid, complete_lines, num_complete_lines);
        }
        // Générer une nouvelle pièce aléatoire
        game->current_tetromino = generate_random_tetromino(); //   generate_random_tetromino(&(game->current_tetromino));

        // Vérifier si la nouvelle pièce peut être placée
        if (!can_place_tetromino(game,&(game->current_tetromino),&board)) {
            // Si la nouvelle pièce ne peut pas être placée, c'est game over
            game->game_over = true;
            return;
        }
    }

    // Mettre à jour le délai entre les frames (ajuster la vitesse du jeu)
    game->frame_delay -= FRAME_DELAY_DECREMENT;
    if (game->frame_delay < MIN_FRAME_DELAY) {
        game->frame_delay = MIN_FRAME_DELAY;
    }
    updateRender();
}

// Dessine l'état actuel du jeu sur le rendu
void render(GameState *game) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    Board board = game->grid;
    drawBoard(&game->grid);;
    tetromino tetromino = game->current_tetromino;
    drawtetromino(&tetromino);

    SDL_RenderPresent(renderer);
}


// Boucle principale du jeu
void gameLoop() {
    while (!game_over) {
        Uint32 lastTime = SDL_GetTicks();
        Uint32 currentTime;
        Uint32 deltaTime;

        handleEvents();
        update_game(&game);
        updateRender();
        render(&game);

        currentTime = SDL_GetTicks();
        deltaTime = currentTime - lastTime;
        if (deltaTime < FRAME_DELAY) {
            SDL_Delay(FRAME_DELAY - deltaTime);
        }
    }
}



// Fonction principale
int main(int argc, char* argv[]) {
    if (!init()) {
        return 1;
    }
    gameLoop();

    closeGame();

    return 0;
}


// int main() {
//     init();
//     gameLoop();
//     SDL_DestroyRenderer(renderer);
//     SDL_DestroyWindow(window);
//     SDL_Quit();
//     closeGame();

//     return 0;
// }
