#include "SDL.h"
#include "SDL_ttf.h"
#include <stdbool.h>
#include <string.h>
#include <time.h>

#define FRAME_DELAY 1000 / 60
#define MIN_FRAME_DELAY 10
#define FRAME_DELAY_DECREMENT 10

// #define SCREEN_WIDTH 100000
// #define SCREEN_HEIGHT 70000
#define BLOCK_SIZE 20
#define BOARD_WIDTH 700
#define BOARD_HEIGHT 1000
#define EMPTY_BLOCK 0
#define TETROMINO_SIZE 4
#define NUM_TETROMINOS 7
#define PLAYFIELD_HEIGHT 22
#define PLAYFIELD_WIDTH 10
#define WINDOW_HEIGHT PLAYFIELD_HEIGHT * (BLOCK_SIZE + 1) + 1
#define WINDOW_WIDTH PLAYFIELD_WIDTH * (BLOCK_SIZE + 1) + 1
#define SCORE_HEIGHT 500

bool render_changed = false;
bool newpiece = true;
bool endgame = false;
bool quit = false;
int PIECES_COUNT = 2;
int HIDDEN_ROW = 0;
int BAG_SIZE = 2;
SDL_Color color = {255, 255, 255, 255}; // Couleur du texte (blanc)

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
    int size;
    int color;
    int matrix[4][4];
} Tetromino;

typedef struct {
    int pieces[NUM_TETROMINOS];
    int index;
    int size;
} Bag;

typedef struct {
    Board grid;
    Tetromino current_tetromino;
    int frame_delay;
    bool game_over;
    int score;
} GameState;

Tetromino Tetroarray[] = {
    { .matrix = {{0, 0, 0, 0},
                 {1, 1, 1, 1},
                 {0, 0, 0, 0},
                 {0, 0, 0, 0}},
      .color = 1 },
    { .matrix = {{2, 0, 0},
                 {2, 2, 2},
                 {0, 0, 0}},
      .color = 2 },
    { .matrix = {{0, 0, 3},
                 {3, 3, 3},
                 {0, 0, 0}},
      .color = 3 },
    { .matrix = {{4, 4},
                 {4, 4}},
      .color = 4 },
    { .matrix = {{0, 5, 5},
                 {5, 5, 0},
                 {0, 0, 0}},
      .color = 5 },
    { .matrix = {{0, 6, 0},
                 {6, 6, 6},
                 {0, 0, 0}},
      .color = 6 },
    { .matrix = {{7, 7, 0},
                 {0, 7, 7},
                 {0, 0, 0}},
      .color = 7 }
};

void init_board(Board* board) {
    memset(board->grid, EMPTY_BLOCK, sizeof(board->grid));
}

void init_tetromino(Tetromino* tetromino) {
    int random_index = rand() % NUM_TETROMINOS;
    *tetromino = Tetroarray[random_index];
}

void init_bag(Bag* bag) {
    bag->index = 0;
    bag->size = NUM_TETROMINOS;
    for (int i = 0; i < NUM_TETROMINOS; i++) {
        bag->pieces[i] = i;
    }
    for (int i = 0; i < NUM_TETROMINOS; i++) {
        int j = rand() % NUM_TETROMINOS;
        int temp = bag->pieces[i];
        bag->pieces[i] = bag->pieces[j];
        bag->pieces[j] = temp;
    }
}

void rotate_tetromino(Tetromino* tetromino) {
    int size = tetromino->size;
    int temp[size][size];
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            temp[i][j] = tetromino->matrix[i][j];
        }
    }
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            tetromino->matrix[i][j] = temp[size - j - 1][i];
        }
    }
    tetromino->rotation = (tetromino->rotation + 1) % 4;
}

bool check_collision(const Board* board, const Tetromino* tetromino, int x_offset, int y_offset) {
    for (int i = 0; i < tetromino->size; i++) {
        for (int j = 0; j < tetromino->size; j++) {
            if (tetromino->matrix[i][j] != EMPTY_BLOCK) {
                int x = tetromino->x + j + x_offset;
                int y = tetromino->y + i + y_offset;
                if (x < 0 || x >= BOARD_WIDTH || y >= BOARD_HEIGHT || board->grid[y][x] != EMPTY_BLOCK) {
                    return true;
                }
            }
        }
    }
    return false;
}

void merge_tetromino(Board* board, const Tetromino* tetromino) {
    for (int i = 0; i < tetromino->size; i++) {
        for (int j = 0; j < tetromino->size; j++) {
            if (tetromino->matrix[i][j] != EMPTY_BLOCK) {
                int x = tetromino->x + j;
                int y = tetromino->y + i;
                board->grid[y][x] = tetromino->color;
            }
        }
    }
}

bool check_full_lines(Board* board) {
        int lines_cleared = 0;
    for (int i = BOARD_HEIGHT - 1; i >= 0; i--) {
        bool is_full_line = true;
        for (int j = 0; j < BOARD_WIDTH; j++) {
            if (board->grid[i][j] == EMPTY_BLOCK) {
                is_full_line = false;
                break;
            }
        }
        if (is_full_line) {
            lines_cleared++;
            for (int k = i; k > 0; k--) {
                memcpy(board->grid[k], board->grid[k - 1], sizeof(board->grid[k]));
            }
            memset(board->grid[0], EMPTY_BLOCK, sizeof(board->grid[0]));
        }
    }
    return lines_cleared > 0;
}

bool game_over(const Board* board) {
    for (int j = 0; j < BOARD_WIDTH; j++) {
        if (board->grid[0][j] != EMPTY_BLOCK) {
            return true;
        }
    }
    return false;
}

void initializeBag(Bag *bag) {
    for (int i = 0; i < BAG_SIZE; i++) {
        bag->pieces[i] = i;
    }
    bag->size = BAG_SIZE;
    printf("ok\n");
}

int updateBag(Bag *bag) {
    if (bag->size == 0) {
        initializeBag(bag);  // Réinitialiser le sac si tous les éléments ont été utilisés
    }

    int randomIndex = rand() % bag->size;  // Générer un index aléatoire dans la taille actuelle du sac
    int choice = bag->pieces[randomIndex];  // Sélectionner la pièce correspondant à l'index
    bag->pieces[randomIndex] = bag->pieces[bag->size - 1];  // Remplacer l'élément sélectionné par le dernier élément
    bag->size--;  // Réduire la taille du sac
    return choice;  // Retourner le choix de la prochaine pièce
}

void setCoords(Tetromino *piece, int x, int y) {
    piece->x = x;  // Définir la coordonnée x de la pièce
    piece->y = y;  // Définir la coordonnée y de la pièce
}

bool inHiddenLayer(Board board) {
    for (int i = 0; i < HIDDEN_ROW; i++) {
        for (int j = 0; j < PLAYFIELD_HEIGHT; j++) {
            if (board.grid[i][j] != EMPTY_BLOCK) {
                return true;  // La pièce est présente dans la couche cachée
            }
        }
    }
    return false;  // Aucune pièce dans la couche cachée
}

bool isValidMove(Board board, Tetromino piece, int newX, int newY) {
    // Vérifier les collisions avec les bords du plateau
    if (newX < 0 || newX + piece.size > PLAYFIELD_HEIGHT || newY + piece.size > PLAYFIELD_WIDTH) {
        return false;
    }

    // Vérifier les collisions avec les autres pièces dans le plateau
    for (int row = 0; row < piece.size; row++) {
        for (int col = 0; col < piece.size; col++) {
            if (piece.shape[row][col]) {
                int boardX = newX + col;
                int boardY = newY + row;

                // Vérifier si la case est déjà occupée
                if (board.grid[boardY][boardX]) {
                    return false;
                }
            }
        }
    }

    return true;
}

bool checkCollision(Board *board, Tetromino *piece, int newX, int newY) {
    // Vérifier les collisions avec les bords du plateau
    if (newX < 0 || newX + piece->size > PLAYFIELD_HEIGHT || newY + piece->size > PLAYFIELD_WIDTH) {
        return true;
    }

    // Vérifier les collisions avec les autres pièces dans le plateau
    for (int row = 0; row < piece->size; row++) {
        for (int col = 0; col < piece->size; col++) {
            if (piece->shape[row][col]) {
                int boardX = newX + col;
                int boardY = newY + row;

                // Vérifier si la case est déjà occupée
                if (board->grid[boardY][boardX]) {
                    return true;
                }
            }
        }
    }

    return false;
}

void lockPiece(Board *board, Tetromino *piece) {
    // Parcourir chaque case de la pièce
    for (int row = 0; row < piece->size; row++) {
        for (int col = 0; col < piece->size; col++) {
            if (piece->shape[row][col]) {
                int boardX = piece->x + col;
                int boardY = piece->y + row;

                // Verrouiller la case sur le plateau en attribuant la couleur de la pièce
                board->grid[boardY][boardX] = piece->color;
            }
        }
    }
}

void updateRotate(Board *board, Tetromino *piece) {
    Tetromino rotatedPiece = *piece;  // Copie de la pièce d'origine pour la rotation
    
    // Effectuer la rotation de la pièce
    for (int i = 0; i < TETROMINO_SIZE; i++) {
        for (int j = 0; j < TETROMINO_SIZE; j++) {
            rotatedPiece.shape[i][j] = piece->shape[TETROMINO_SIZE - 1 - j][i];
        }
    }
    
    // Vérifier si la rotation est valide en évitant les collisions avec les autres pièces et les bords du plateau
    if (!checkCollision(board, &rotatedPiece, rotatedPiece.x, rotatedPiece.y)) {
        *piece = rotatedPiece;  // Mettre à jour la pièce avec la rotation valide
    }
}

void updateDown(Board *board, Tetromino *piece, bool *newpiece, bool hardDrop) {
    int newX = piece->x;
    int newY = piece->y + 1;

    // Vérifier si le déplacement est valide
    if (!checkCollision(board, piece, newX, newY)) {
        piece->y = newY;  // Mettre à jour la coordonnée y de la pièce

        if (hardDrop) {
            while (!checkCollision(board, piece, newX, piece->y + 1)) {
                piece->y++;
            }
            *newpiece = true;  // Indiquer qu'une nouvelle pièce doit être générée
        }
    }
    else if (hardDrop) {
        *newpiece = true;  // Indiquer qu'une nouvelle pièce doit être générée
    }
}

void updateLeft(Board *board, Tetromino *piece) {
    int newX = piece->x - 1;
    int newY = piece->y;

    // Vérifier si le déplacement est valide
    if (!checkCollision(board, piece, newX, newY)) {
        piece->x = newX;  // Mettre à jour la coordonnée x de la pièce
    }
}

void updateRight(Board *board, Tetromino *piece) {
    int newX = piece->x + 1;

    // Vérifier si le déplacement est valide
    if (isValidMove(*board, *piece, newX, piece->y)) {
        piece->x = newX;  // Mettre à jour la coordonnée x de la pièce
    }
}

void hardDrop(Board *board, Tetromino *piece, bool *newpiece, bool lock) {
    while (isValidMove(*board, *piece, piece->x, piece->y + 1)) {
        piece->y++;  // Déplacer la pièce vers le bas
    }

    // Verrouiller la pièce si nécessaire
    if (lock) {
        lockPiece(board, piece);  // Verrouiller la pièce sur le plateau
        *newpiece = true;  // Indiquer qu'une nouvelle pièce doit être créée
    }
}

int lineClear(Board *board, int *multipleLines) {
    int score = 0;
    int linesCleared = 0;

    // Parcourir chaque ligne de la grille
    for (int row = 0; row < PLAYFIELD_WIDTH; row++) {
        bool lineComplete = true;

        // Vérifier si la ligne est complète
        for (int col = 0; col < PLAYFIELD_HEIGHT; col++) {
            if (board->grid[row][col] == 0) {
                lineComplete = false;
                break;
            }
        }

        // Si la ligne est complète, la supprimer
        if (lineComplete) {
            linesCleared++;
            score += 100;  // Augmenter le score

            // Déplacer les lignes supérieures vers le bas
            for (int r = row; r > 0; r--) {
                for (int c = 0; c < PLAYFIELD_HEIGHT; c++) {
                    board->grid[r][c] = board->grid[r - 1][c];
                }
            }

            // Effacer la première ligne
            for (int c = 0; c < PLAYFIELD_HEIGHT; c++) {
                board->grid[0][c] = 0;
            }
        }
    }

    *multipleLines = linesCleared;

    return score;
}

void clearScoreBoard(SDL_Renderer *renderer) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);  // Couleur noire
    SDL_Rect scoreRect = { 0, PLAYFIELD_WIDTH * BLOCK_SIZE, PLAYFIELD_HEIGHT * BLOCK_SIZE, SCORE_HEIGHT };
    SDL_RenderFillRect(renderer, &scoreRect);
}

void drawGrid(SDL_Renderer *renderer) {
    SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);  // Couleur grise pour les lignes de la grille

    // Dessiner les lignes horizontales de la grille
    for (int row = 0; row < PLAYFIELD_WIDTH; row++) {
        SDL_RenderDrawLine(renderer, 0, row * BLOCK_SIZE, PLAYFIELD_HEIGHT * BLOCK_SIZE, row * BLOCK_SIZE);
    }

    // Dessiner les lignes verticales de la grille
    for (int col = 0; col < PLAYFIELD_HEIGHT; col++) {
        SDL_RenderDrawLine(renderer, col * BLOCK_SIZE, 0, col * BLOCK_SIZE, PLAYFIELD_WIDTH * BLOCK_SIZE);
    }
}

void drawCell(int x, int y, int color, SDL_Renderer* renderer) {
    // Définir la couleur de rendu en fonction de la valeur de couleur spécifiée
    switch (color) {
        case 0: // Couleur 0 (par exemple, noir)
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            break;
        case 1: // Couleur 1 (par exemple, rouge)
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            break;
        // Ajoutez d'autres cas pour d'autres couleurs si nécessaire
        // case 2: ...
        // case 3: ...
        default: // Couleur par défaut (par exemple, blanc)
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            break;
    }

    // Dessiner un rectangle plein pour représenter la cellule
    SDL_Rect cellRect;
    cellRect.x = x;
    cellRect.y = y;
    cellRect.w = 10; // Largeur de la cellule (10 pixels dans cet exemple)
    cellRect.h = 10; // Hauteur de la cellule (10 pixels dans cet exemple)
    SDL_RenderFillRect(renderer, &cellRect);
}

void drawBoard(Board board, SDL_Renderer *renderer) {
    for (int row = HIDDEN_ROW; row < PLAYFIELD_WIDTH; row++) {
        for (int col = 0; col < PLAYFIELD_HEIGHT; col++) {
            int cellValue = board.grid[row][col];
            if (cellValue != 0) {
                drawCell(col, row, cellValue, renderer);
            }
        }
    }
}


void drawGhostCell(int x, int y, SDL_Renderer *renderer, GameState* game)
{
    // Calculer les coordonnées de la cellule à dessiner
    int screenX = x * BLOCK_SIZE;
    int screenY = y * BLOCK_SIZE;

    // Dessiner un rectangle translucide pour représenter la cellule
    SDL_Rect cellRect = { screenX, screenY, BLOCK_SIZE, BLOCK_SIZE };
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 100);  // Couleur blanche translucide
    SDL_RenderFillRect(renderer, &cellRect);
}

void drawTetromino(Tetromino piece, SDL_Renderer *renderer, bool ghost, GameState* game) {
    int startX = piece.x * BLOCK_SIZE;  // Coordonnée x de départ de la pièce
    int startY = piece.y * BLOCK_SIZE;  // Coordonnée y de départ de la pièce

    // Utiliser une boucle pour parcourir toutes les cellules de la pièce
    for (int row = 0; row < TETROMINO_SIZE; row++) {
        for (int col = 0; col < TETROMINO_SIZE; col++) {
            int cellValue = piece.shape[row][col];
            if (cellValue != 0) {
                int x = startX + col * BLOCK_SIZE;  // Coordonnée x de la cellule
                int y = startY + row * BLOCK_SIZE;  // Coordonnée y de la cellule

                if (ghost) {
                    // Dessiner une cellule transparente pour le fantôme
                    drawGhostCell(x, y, renderer, game);
                } else {
                    // Dessiner une cellule pleine pour la pièce
                    drawCell(x, y, piece.color, renderer);
                }
            }
        }
    }
}

void drawBag(Bag bag, SDL_Renderer *renderer, GameState* game) {
    int offsetX = (PLAYFIELD_HEIGHT + 4) * BLOCK_SIZE;  // Décalage horizontal pour le sac de pièces
    int offsetY = 2 * BLOCK_SIZE;  // Décalage vertical pour le sac de pièces

    for (int i = 0; i < BAG_SIZE; i++) {
        Tetromino piece = Tetroarray[bag.size];
        drawTetromino(piece, renderer, true, game);  // Appeler une fonction pour dessiner chaque pièce du sac
        piece.x = offsetX;
        piece.y = offsetY + i * (BLOCK_SIZE * 5);  // Espace vertical entre les pièces du sac
        drawTetromino(piece, renderer, false, game);  // Dessiner la pièce à sa position dans le sac
    }
}



// int movePiece(int dx, int dy)
// {
//     // Vérifier si le déplacement est valide
//     if (!checkCollision(currentPiece, currentX + dx, currentY + dy, currentRotation))
//     {
//         // Effacer la pièce de sa position actuelle
//         erasepiece(currentPiece, currentX, currentY, currentRotation);

//         // Mettre à jour les coordonnées de la pièce
//         currentX += dx;
//         currentY += dy;

//         // Dessiner la pièce à sa nouvelle position
//         drawpiece(currentPiece, currentX, currentY, currentRotation);

//         return 1; // Déplacement réussi
//     }

//     return 0; // Déplacement invalide
// }

int calculateFrameDelay(int score, int multipleLines) {
    int frameDelay = 0;

    // Calcul du délai en fonction du score
    if (score < 1000) {
        frameDelay = 1000 - score;
    } else if (score < 2000) {
        frameDelay = 500;
    } else if (score < 4000) {
        frameDelay = 250;
    } else {
        frameDelay = 100;
    }

    // Ajouter un délai supplémentaire pour les lignes supprimées d'un coup
    if (multipleLines == 4) {
        frameDelay += 200;
    } else if (multipleLines >= 2) {
        frameDelay += 100;
    }

    return frameDelay;
}


void fixPiece(Tetromino* ghostPiece, GameState* game) {
    // Copier les coordonnées de la pièce fantôme
    int ghostX = ghostPiece->x;
    int ghostY = ghostPiece->y;

    // Générer une nouvelle pièce
        Tetromino currentPiece;

    // Mettre à jour la pièce actuelle avec les coordonnées de la pièce fantôme
    currentPiece.x = ghostX;
    currentPiece.y = ghostY;

    // Vérifier si la pièce fantôme a atteint le bas du plateau
    if (ghostY + currentPiece.size >= BOARD_HEIGHT) {
        // Verrouiller la pièce sur le plateau
        lockPiece(&(game->grid), &currentPiece);

        // Vérifier si des lignes peuvent être effacées
        int multipleLines;
        int score = lineClear(&(game->grid), &multipleLines);

        // Mettre à jour le score
        game->score += score;


        // Vérifier si la nouvelle pièce est en collision avec une autre pièce
        if (checkCollision(&(game->grid), &currentPiece, currentPiece.x, currentPiece.y)) {
            // Le jeu est terminé
            game->game_over = true;
        }

        // Mettre à jour le délai de frame en fonction du nombre de lignes effacées
        game->frame_delay = calculateFrameDelay(game->score, multipleLines);
    }
}

bool canMoveDown(Tetromino* ghostPiece, Board board) {
    int newX = ghostPiece->x;
    int newY = ghostPiece->y + 1;

    // Vérifier si le déplacement est valide
    if (!checkCollision(&board, ghostPiece, newX, newY)) {
        return true;  // Le déplacement vers le bas est possible
    }

    return false;  // Le déplacement vers le bas n'est pas possible
}



void movePiece(Tetromino* ghostPiece, Board board, GameState* game) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_KEYDOWN) {
            switch (event.key.keysym.sym) {
                case SDLK_DOWN:
                    // Vérifier si le tétromino peut descendre d'une ligne
                    if (canMoveDown(ghostPiece, board) ){
                        ghostPiece->y++; // Déplacer le tétromino vers le bas
                    } else {
                        // Le tétromino ne peut pas descendre plus bas, donc il est fixé à sa position actuelle
                        fixPiece(ghostPiece,game);
                    }
                    break;
                // Gérer les autres touches si nécessaire
                // ...
            }
        }
    }
}




void drawGhost(Board board, Tetromino piece, SDL_Renderer *renderer, GameState* game) {
    Tetromino ghostPiece = piece;  // Copie de la pièce actuelle pour le fantôme
    while (isValidMove(board, ghostPiece, ghostPiece.x, ghostPiece.y)) {
        // Déplacer la pièce fantôme vers le bas jusqu'à ce qu'elle ne puisse plus descendre
        movePiece(&ghostPiece, board ,game);
    }
    // Dessiner la pièce fantôme sur le rendu du jeu
    drawTetromino(ghostPiece, renderer, true, game);
}


void endGame(Board *board) {
    int row, col;

    // Remplir la grille avec des blocs gris pour indiquer la fin du jeu
    for (row = 0; row < PLAYFIELD_WIDTH; row++) {
        for (col = 0; col < PLAYFIELD_HEIGHT; col++) {
            board->grid[row][col] = 8; // Indice pour la couleur des blocs gris
        }
    }
}

void drawScoreBoard(SDL_Renderer *renderer, int score, int lines) {
    SDL_Color textColor = {255, 255, 255}; // Couleur du texte (blanc dans cet exemple)
    TTF_Font *font = TTF_OpenFont("font.ttf", 28);

    if (font == NULL) {
        printf("Erreur lors du chargement de la police de caractères : %s\n", TTF_GetError());
        return;
    }

    char scoreText[20];
    sprintf(scoreText, "Score: %d", score); // Formatage du texte du score

    SDL_Surface *scoreSurface = TTF_RenderText_Solid(font, scoreText, textColor); // Créer une surface à partir du texte
    if (scoreSurface == NULL) {
        printf("Erreur lors de la création de la surface du score : %s\n", TTF_GetError());
        TTF_CloseFont(font);
        return;
    }

    SDL_Texture *scoreTexture = SDL_CreateTextureFromSurface(renderer, scoreSurface); // Créer une texture à partir de la surface
    if (scoreTexture == NULL) {
        printf("Erreur lors de la création de la texture du score : %s\n", SDL_GetError());
        SDL_FreeSurface(scoreSurface);
        TTF_CloseFont(font);
        return;
    }

    SDL_Rect scoreRect;
    scoreRect.x = 10; // Position x du score
    scoreRect.y = 10; // Position y du score
    scoreRect.w = scoreSurface->w; // Largeur de la surface
    scoreRect.h = scoreSurface->h; // Hauteur de la surface

    SDL_RenderCopy(renderer, scoreTexture, NULL, &scoreRect); // Dessiner la texture du score sur le rendu

    // Afficher également le nombre de lignes effacées si nécessaire
    char linesText[20];
    sprintf(linesText, "Lines: %d", lines); // Formatage du texte des lignes

    SDL_Surface *linesSurface = TTF_RenderText_Solid(font, linesText, textColor); // Créer une surface à partir du texte des lignes
    if (linesSurface == NULL) {
        printf("Erreur lors de la création de la surface des lignes : %s\n", TTF_GetError());
        SDL_DestroyTexture(scoreTexture);
        TTF_CloseFont(font);
        return;
    }

    SDL_Texture *linesTexture = SDL_CreateTextureFromSurface(renderer, linesSurface); // Créer une texture à partir de la surface des lignes
    if (linesTexture == NULL) {
        printf("Erreur lors de la création de la texture des lignes : %s\n", SDL_GetError());
        SDL_FreeSurface(scoreSurface);
        SDL_FreeSurface(linesSurface);
        TTF_CloseFont(font);
        return;
    }

    SDL_Rect linesRect;
    linesRect.x = 10; // Position x des lignes
    linesRect.y = 50; // Position y des lignes
    linesRect.w = linesSurface->w; // Largeur de la surface des lignes
    linesRect.h = linesSurface->h; // Hauteur de la surface des lignes

    SDL_RenderCopy(renderer, linesTexture, NULL, &linesRect); // Dessiner la texture des lignes sur le rendu

    // Libérer les ressources
    SDL_FreeSurface(scoreSurface);
    SDL_FreeSurface(linesSurface);
    SDL_DestroyTexture(scoreTexture);
    SDL_DestroyTexture(linesTexture);
    TTF_CloseFont(font);
}
       

int main(int argc, char* args[]) {
    srand(time(NULL));

    if (SDL_Init(SDL_INIT_EVERYTHING) == -1) {
        printf("Erreur lors de l'initialisation de SDL\n");
        return 1;
    }
    TTF_Init();
    TTF_Font* font = TTF_OpenFont("arial.ttf", 30); // Remplacez "arial.ttf" par le chemin d'accès à votre police de caractères

    SDL_Window* window = SDL_CreateWindow("Tetris", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT + SCORE_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);
    SDL_Surface* surfaceMessage = TTF_RenderText_Solid(font, "Game Over", color);
    SDL_Texture* message = SDL_CreateTextureFromSurface(renderer, surfaceMessage);
    SDL_Rect message_rect;
    message_rect.x = (WINDOW_WIDTH - 200) / 2;
    message_rect.y = (WINDOW_HEIGHT - 200) / 2;
    message_rect.w = 200;
    message_rect.h = 200;
    
    //Setup for the game to work 
    Board board={0};
    Tetromino piece;
    Tetromino hold;
    bool newpiece = true;
    bool holding = false;
    int score={0};
    int lines= 0;
    int levelCounter=0;
    int multipleLines=0;
    int frameToDrop = 60;
    int framesSinceDrop = 0;
    bool quit = false;
    bool endgame = false;    
    Bag bag={0,0,0};
    init_bag(&bag);
    GameState gamestate = {0,0,0,0,0};
    GameState* game = &gamestate;
    game->grid= board;
    //  Creates the window where the game will be played
    window = SDL_CreateWindow("Tetris", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
if (window == NULL) {
    printf("Erreur lors de la création de la fenêtre : %s\n", SDL_GetError());
    TTF_Quit();
    SDL_Quit();
    return 1;
}
    // Renderer for the game
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
   renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == NULL) {
    printf("Erreur lors de la création du renderer : %s\n", SDL_GetError());
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    return 1;
}
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    while(!quit){
        if(inHiddenLayer(board)) endgame=true;
        if (newpiece){ // Create a newpiece when "newpiece" is set to through
            int choice = updateBag(&bag);
            piece=Tetroarray[choice];
            setCoords(&piece,5,2);
            newpiece=false;
        }
        int value=0;
        SDL_Event event; 
        while(SDL_PollEvent(&event) != 0) {
            if (event.type==SDL_QUIT){
                quit=true;
            }
            else if (event.type == SDL_KEYDOWN && !endgame) {
                switch (event.key.keysym.sym) {
                    // char* mouvement = SDL_KEYDOWN;
                    case SDLK_ESCAPE: 
                        quit=true;
                        break;
                    case SDLK_UP:
                        updateRotate(&board,&piece);
                        break;
                    case SDLK_DOWN:
                        updateDown(&board,&piece,&newpiece,false);
                        break;
                    case SDLK_LEFT:
                        updateLeft(&board,&piece);
                        break;
                    case SDLK_RIGHT:
                        updateRight(&board,&piece);
                        break;
                    case SDLK_SPACE:
                        hardDrop(&board,&piece,&newpiece,false);
                        break;
                    case SDLK_c:
                        value = piece.color;
                        // int offset_x = (value==&Tetromino)? 2 : 0;
                        if (piece.y<6){
                            if (holding){
                                piece=hold;
                                setCoords(&piece,5,2);
                            }else{
                                holding=true;
                                newpiece=true;
                            }
                            hold=Tetroarray[value];
                            setCoords(&hold,PLAYFIELD_HEIGHT+4+value,9);//offset_x
                        }
                        break;
                    default:
                        break;
                }
            }
        }
        if (framesSinceDrop >= frameToDrop) {
            updateDown(&board, &piece, &newpiece, false);
            framesSinceDrop = 0; 
        }
        clearScoreBoard(renderer);
        drawGrid(renderer);
        drawBoard(board,renderer);
        if (!endgame){
            drawBag(bag,renderer,game);
            score+=lineClear(&board,&multipleLines);
            lines+=multipleLines;
            levelCounter+=multipleLines;
            drawTetromino(piece, renderer, false, game);
            if (holding) drawTetromino(hold,renderer,false, game);
            //drawGhostCell(board, piece, *renderer); // Normalement c'est drawghost
        }
        else{
            piece.color=7;
            endGame(&board);
        }
        if(levelCounter>10){
            frameToDrop/=1.3;
            levelCounter%=10;
        }
        drawScoreBoard(renderer,score,lines);
        SDL_RenderPresent(renderer);
        SDL_Delay(16);
        framesSinceDrop++;
    }
  TTF_CloseFont(font);
    TTF_Quit();
    SDL_DestroyTexture(message);
    SDL_FreeSurface(surfaceMessage);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
