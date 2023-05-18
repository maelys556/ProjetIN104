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
bool newpiece = true;

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

Bag bag={0};
    initializeBag(&bag);

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

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

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

bool inHiddenLayer(Board board) {
    for (int col = 0; col < COLUMN; col++)
    {
        if (board[0][col] != 0)
        {
            return true; // Une pièce est présente dans la ligne cachée
        }
    }
    return false; // Aucune pièce n'est présente dans la ligne cachée
}

void initializeBag(Bag *bag)
{
    // Initialisation des valeurs du sac de pièces
    for (int i = 0; i < NUM_TETROMINOS; i++)
    {
        bag->pieces[i] = i;
    }
    
    bag->index = NUM_TETROMINOS; // Index pour suivre la position dans le sac
    bag->size = NUM_TETROMINOS;  // Taille du sac
}

int updateBag(Bag *bag)
{
    if (bag->index >= bag->size)
    {
        // Mélange des pièces dans le sac
        for (int i = bag->size - 1; i > 0; i--)
        {
            int j = rand() % (i + 1);
            int temp = bag->pieces[i];
            bag->pieces[i] = bag->pieces[j];
            bag->pieces[j] = temp;
        }
        
        bag->index = 0; // Réinitialisation de l'index
    }
    
    int choice = bag->pieces[bag->index]; // Sélection de la prochaine pièce
    bag->index++; // Incrément de l'index pour la prochaine fois
    
    return choice; // Renvoie l'indice de la pièce sélectionnée
}

void hardDrop(Board *board, Tetromino *piece, bool *newpiece, bool checkCollision)
{
    while (!checkCollision && !collision(*board, *piece, 0, 1))
    {
        piece->y++; // Déplacement vers le bas
    }
    
    piece->y--; // Annuler le dernier déplacement vers le bas
    
    // Fixer la pièce sur le plateau
    for (int row = 0; row < PIECE_SIZE; row++)
    {
        for (int col = 0; col < PIECE_SIZE; col++)
        {
            if (piece->shape[row][col] != 0)
            {
                int boardRow = piece->y + row;
                int boardCol = piece->x + col;
                (*board)[boardRow][boardCol] = piece->color;
            }
        }
    }
    
    *newpiece = true; // Créer une nouvelle pièce
}

void drawGrid(SDL_Renderer* renderer)
{
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE); // Couleur de la grille (blanc)
    
    // Dessine les lignes verticales de la grille
    for (int x = 0; x <= (COLUMN+9) * CELLSIZE; x += CELLSIZE)
    {
        SDL_RenderDrawLine(renderer, x, 0, x, ROW * CELLSIZE);
    }
    
    // Dessine les lignes horizontales de la grille
    for (int y = 0; y <= ROW * CELLSIZE; y += CELLSIZE)
    {
        SDL_RenderDrawLine(renderer, 0, y, (COLUMN+9) * CELLSIZE, y);
    }
    
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE); // Réinitialise la couleur du rendu
}

void drawGhost(const Board& board, const Tetromino& piece, SDL_Renderer* renderer)
{
    int ghostY = piece.y;
    
    // Trouver la position verticale la plus basse où la pièce peut tomber
    while (!collision(board, piece, 0, 1))
    {
        ghostY++; // Déplacement vers le bas
    }
    
    // Dessiner l'ombre de la pièce
    for (int row = 0; row < PIECE_SIZE; row++)
    {
        for (int col = 0; col < PIECE_SIZE; col++)
        {
            if (piece.shape[row][col] != 0)
            {
                int x = (piece.x + col) * CELLSIZE;
                int y = (ghostY + row) * CELLSIZE;
                
                SDL_SetRenderDrawColor(renderer, 100, 100, 100, SDL_ALPHA_OPAQUE); // Couleur de l'ombre (gris)
                SDL_Rect rect = {x, y, CELLSIZE, CELLSIZE};
                SDL_RenderFillRect(renderer, &rect);
            }
        }
    }
    
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE); // Réinitialise la couleur du rendu
}

void drawBag(const Bag& bag, SDL_Renderer* renderer)
{
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE); // Couleur du contour de la pièce (blanc)
    
    // Coordonnées du rectangle d'affichage de la prochaine pièce
    int startX = (COLUMN + 4) * CELLSIZE;
    int startY = 2 * CELLSIZE;
    
    // Dessiner un rectangle de fond pour la prochaine pièce
    SDL_Rect rect = {startX, startY, 5 * CELLSIZE, 5 * CELLSIZE};
    SDL_RenderDrawRect(renderer, &rect);
    
    // Dessiner la prochaine pièce
    if (bag.size() > 0)
    {
        const Tetromino& nextPiece = *Tetroarray[bag[0]];
        
        for (int row = 0; row < PIECE_SIZE; row++)
        {
            for (int col = 0; col < PIECE_SIZE; col++)
            {
                if (nextPiece.shape[row][col] != 0)
                {
                    int x = startX + col * CELLSIZE;
                    int y = startY + row * CELLSIZE;
                    
                    SDL_Rect cellRect = {x, y, CELLSIZE, CELLSIZE};
                    SDL_RenderFillRect(renderer, &cellRect);
                }
            }
        }
    }
    
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE); // Réinitialise la couleur du rendu
}

void endGame(Board* board)
{
    // Code pour afficher l'écran de fin de jeu
    // Cela peut inclure l'affichage du message "Game Over" ou toute autre interface graphique spécifique à votre jeu.
    // Par exemple, vous pouvez utiliser SDL pour afficher du texte à l'écran.
    // Voici un exemple basique pour effacer le plateau de jeu :
    
    for (int row = 0; row < ROW; row++)
    {
        for (int col = 0; col < COLUMN; col++)
        {
            board->grid[row][col] = 0;
        }
    }
}

void clearScoreBoard(SDL_Renderer* renderer)
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE); // Couleur de fond (noir)
    SDL_RenderClear(renderer); // Efface le rendu avec la couleur de fond définie
}

void drawScoreBoard(SDL_Renderer* renderer, int score, int lines)
{
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE); // Couleur du texte (blanc)
    
    // Affiche le score en haut à gauche de l'écran
    renderText(renderer, scoreText, 10, 10);
    
    // Affiche le nombre de lignes effacées à droite du score
    renderText(renderer, linesText, 10 + scoreText.length() * 8, 10);
    
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE); // Réinitialise la couleur du rendu
}

void setCoords(tetromino* tetromino, int x, int y)
{
    piece->x = x;
    piece->y = y;
}


// Gestion des événements
void handleEvents() {
    while(!quit){
        if(inHiddenLayer(board)) endgame=true;
        if (newpiece){ // Create a newpiece when "newpiece" is set to through
            int choice = updateBag(&bag);
            tetromino piece=*Tetroarray[choice];
            setCoords(&tetromino,5,2);
            newpiece=false;
        }
        int value=0;
        SDL_Event event; 
        while (SDL_PollEvent(&event)!= 0) {
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
    if (can_move_down(game)) {
        // Si oui, déplacer la pièce vers le bas
        movetetromino(&(game->current_tetromino), 0, 1); 
    } else {
        Board board = game->grid;
        // Si la pièce ne peut pas descendre, la verrouiller dans la grille
        lock_tetromino(game, game->grid.grid, game->current_tetromino); 
        // Vérifier s'il y a des lignes complètes
        int complete_lines[BOARD_HEIGHT];
        int num_complete_lines = 0;
        find_complete_lines(&(game->grid), complete_lines, &num_complete_lines);
        if (num_complete_lines > 0) { //if (clear_lines > 0) {
            // Mettre à jour le score en fonction du nombre de lignes complètes
            game->score += calculate_score(num_complete_lines); 
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
