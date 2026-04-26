#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

#define PORT 30000
#define SCREEN_SIZE 900
#define SQUARE_SIZE (SCREEN_SIZE / 8)
#define BOARD_SIZE 8

// Variables globales

// Variables pour gérer le tour de jeu
int client_id;
bool play = false;
bool simul = false;

// Tests qui vérifient si les pièces utilisées dans le roque ont déjà bougé
bool white_rook1_moved = false;
bool white_rook2_moved = false;
bool black_rook1_moved = false;
bool black_rook2_moved = false;
bool white_king_moved = false;
bool black_king_moved = false;

// Variables utilisées pour gérer la sélection ou non d'une pièce adéquate (vaut -1 si aucune pièce n'est sélectionnée)
int selected_piece_x = -1;
int selected_piece_y = -1;

// Tableau et textures pour interface graphique
SDL_Texture* white_pawn, * white_rook, * white_knight, * white_bishop, * white_king, * white_queen;
SDL_Texture* black_pawn, * black_rook, * black_knight, * black_bishop, * black_king, * black_queen;

// Structure des données à échanger entre clients et serveur
typedef struct {
    int board[BOARD_SIZE][BOARD_SIZE];
    bool quit;
    bool is_white_turn;
    bool game_over;
    bool restart_choice;
    int en_passant_blanc;
    int en_passant_noir;
    bool capture_en_passant_blanc;
    bool capture_en_passant_noir;
} GameState;

// Variable de la structure de données pour les manipuler plus facilement
GameState game_state;

// Déclaration des fonctions
SDL_Texture* load_texture(const char *file, SDL_Renderer *renderer);
bool is_horizontal_path_clear(int start_row, int start_col, int end_col);
bool is_vertical_path_clear(int start_row, int start_col, int end_row);
bool is_diagonal_path_clear(int start_row, int start_col, int end_row, int end_col);
bool is_valid_bishop_move(int start_row, int start_col, int end_row, int end_col);
bool is_valid_rook_move(int start_row, int start_col, int end_row, int end_col);
bool is_valid_knight_move(int start_row, int start_col, int end_row, int end_col);
bool is_valid_queen_move(int start_row, int start_col, int end_row, int end_col);
bool is_valid_king_move(int start_row, int start_col, int end_row, int end_col);
bool is_valid_pawn_move(int start_row, int start_col, int end_row, int end_col, int piece);
void show_promotion_menu(SDL_Renderer *renderer, int piece, int menu_x, int menu_y);
int handle_promotion_selection(int x, int y, int piece, int menu_x, int menu_y);
void promote_pawn(int row, int col, int piece, SDL_Renderer *renderer);
bool is_valid_move(int start_row, int start_col, int end_row, int end_col);
bool is_king_in_check();
bool is_checkmate();
void handle_game_over(SDL_Renderer *renderer);
bool is_castling_valid(int piece, int end_col);
void handle_mouse_click(int x, int y, SDL_Renderer *renderer);
void load_all_textures(SDL_Renderer* renderer);
void draw_chessboard(SDL_Renderer *renderer);
void draw_piece(SDL_Renderer *renderer, SDL_Texture *texture, int row, int col);
void draw_pieces(SDL_Renderer *renderer);
void reverse_board(int board[BOARD_SIZE][BOARD_SIZE]);
void receive_game_state_from_server(int client_socket, GameState *game_state);
void send_game_state_to_server(int client_socket, GameState *game_state);
void start_client(const char *server_ip);

// Fonction principale
int main() {
    const char *server_ip = "172.26.136.239"; // Adresse IP de la machine qui héberge le serveur ou "127.0.0.1" sur la même machine
    start_client(server_ip);
    return 0;
}

// Fonction pour charger une texture/un visuel depuis un fichier
SDL_Texture* load_texture(const char *file, SDL_Renderer *renderer) {
    SDL_Texture *texture = NULL;
    SDL_Surface *loaded_image = IMG_Load(file);
    if (loaded_image != NULL) {
        texture = SDL_CreateTextureFromSurface(renderer, loaded_image);
        SDL_FreeSurface(loaded_image);
    } else {
        printf("Erreur de chargement de l'image %s! SDL_Error: %s\n", file, SDL_GetError());
    }
    return texture;
}

// Vérifie si le chemin pour un mouvement horizontal est clair
bool is_horizontal_path_clear(int start_row, int start_col, int end_col) {
    int col_direction;
    if (end_col > start_col) {
        col_direction = 1; // Le mouvement est vers le bas
    } else {
        col_direction = -1; // Le mouvement est vers le haut
    }

    // On fait se déplacer la pièce d'une case dans la bonne direction 
    int col = start_col + col_direction;

    // Puis on re-test s'il y a une pièce qui bloque le déplacement jusqu'à ce que la pièce arrive à destination
    while (col != end_col) {
        if (game_state.board[start_row][col] != 0) {
            return false; // Une pièce bloque le chemin
        }
        col = col + col_direction;
    }
    return true; // Il n'y a pas de pièce sur le chemin
}

// Vérifie si le chemin pour un mouvement vertical est clair
bool is_vertical_path_clear(int start_row, int start_col, int end_row) {
    int row_direction;
    if (end_row > start_row) {
        row_direction = 1; // Le mouvement est vers le bas
    } else {
        row_direction = -1; // Le mouvement est vers le haut
    }

    // On fait se déplacer la pièce d'une case dans la bonne direction 
    int row = start_row + row_direction;
    // Puis on re-test s'il y a une pièce qui bloque le déplacement jusqu'à ce que la pièce arrive à destination
    while (row != end_row) {
        if (game_state.board[row][start_col] != 0) {
            return false; // Une pièce bloque le chemin
        }
        row = row + row_direction;
    }
    return true; // Il n'y a pas de pièce sur le chemin
}

// Vérifie si le chemin pour un mouvement diagonal est clair
bool is_diagonal_path_clear(int start_row, int start_col, int end_row, int end_col) {
    int row_direction;
    if (end_row > start_row) {
        row_direction = 1; // Le mouvement est vers le bas
    } else {
        row_direction = -1; // Le mouvement est vers le haut
    }

    int col_direction;
    if (end_col > start_col) {
        col_direction = 1; // Le mouvement est vers le bas
    } else {
        col_direction = -1; // Le mouvement est vers le haut
    }

    // On fait se déplacer la pièce d'une case dans la bonne direction 
    int row = start_row + row_direction;
    int col = start_col + col_direction;
    // Puis on re-test s'il y a une pièce qui bloque le déplacement jusqu'à ce que la pièce arrive à destination
    while (row != end_row && col != end_col) {
        if (game_state.board[row][col] != 0) {
            return false; // Une pièce bloque le chemin
        }
        row = row + row_direction;
        col = col + col_direction;
    }
    return true; // Il n'y a pas de pièce sur le chemin
}

// Vérifie si le mouvement du fou est clair
bool is_valid_bishop_move(int start_row, int start_col, int end_row, int end_col) {
    // Vérifier si le déplacement est en diagonale
    if (abs(end_row - start_row) != abs(end_col - start_col)) {
        return false;
    }

    // Test si le mouvement diagonal est clair
    if (is_diagonal_path_clear(start_row, start_col, end_row, end_col)) {
        return true;
    }
    return false; // Le mouvement n'est pas possible pour un fou
}

// Vérifie si le mouvement de la tour est clair
bool is_valid_rook_move(int start_row, int start_col, int end_row, int end_col) {
    // Vérifier si le déplacement est horizontal ou vertical
    if (start_row != end_row && start_col != end_col) {
        return false;  // La tour ne peut se déplacer que sur une ligne ou une colonne
    }

    if (start_row - end_row != 0) { // Si le mouvement de la tour est vertical
        return is_vertical_path_clear(start_row, start_col, end_row);
    } else if (start_col - end_col != 0){ // Si le mouvement de la tour est horizontal
        return is_horizontal_path_clear(start_row, start_col, end_col);
    }
    return false; // Le mouvement n'est pas possible pour une tour
}

// Vérifie si le mouvement du cavalier est clair
bool is_valid_knight_move(int start_row, int start_col, int end_row, int end_col) {
    int row_diff = abs(end_row - start_row);
    int col_diff = abs(end_col - start_col);

    // Vérifie si le déplacement est en "L" : 2 cases dans une direction et 1 dans l'autre, ou inversement
    if ((row_diff == 2 && col_diff == 1) || (row_diff == 1 && col_diff == 2)) {
        return true;
    }
    return false; // Le mouvement n'est pas possible pour un cavalier
}

// Vérifie si le mouvement de la dame est clair
bool is_valid_queen_move(int start_row, int start_col, int end_row, int end_col) {
    // Vérifier si le déplacement est horizontal, vertical ou diagonal
    if (start_row == end_row) { // Horizontal
        return is_horizontal_path_clear(start_row, start_col, end_col);
    } else if (start_col == end_col) { // Vertical
        return is_vertical_path_clear(start_row, start_col, end_row);
    } else if (abs(end_row - start_row) == abs(end_col - start_col)) { // Diagonal
        return is_diagonal_path_clear(start_row, start_col, end_row, end_col);
    }
    return false; // Le mouvement n'est pas possible pour une dame
}

bool is_valid_king_move(int start_row, int start_col, int end_row, int end_col) {
    // Le roi peut se déplacer d'une case dans n'importe quelle direction
    int row_diff = abs(end_row - start_row);
    int col_diff = abs(end_col - start_col);

    if (row_diff <= 1 && col_diff <= 1) {
        return true; // Déplacement valide
    }
    return false; // Le mouvement n'est pas possible pour un roi
}

// Fonction pour vérifier les déplacements des pions
bool is_valid_pawn_move(int start_row, int start_col, int end_row, int end_col, int piece) {
    // Déplacer d'une ou deux cases en avant si elles sont vides
    if (start_col == end_col && game_state.board[end_row][end_col] == 0) {
        if (end_row - start_row == -1) {
            return true; // Une case
        } else if (end_row - start_row == -2) {
            if (start_row == 6 && game_state.board[5][start_col] == 0) {
                if (piece == 1) {
                    game_state.en_passant_blanc = end_col;
                    //printf("en_passant_blanc = %d\n", game_state.en_passant_blanc);
                    return true; // Premier mouvement du pion blanc de 2 cases
                } else if (piece == 7) {
                    game_state.en_passant_noir = end_col;
                    //printf("en_passant_noir = %d\n", game_state.en_passant_noir);
                    return true; // Premier mouvement du pion noir de 2 cases
                }
            }
        }
    }

    // Capturer en diagonale : on est certain que la pièce en diagonale n'est pas une pièce alliée
    if (game_state.board[end_row][end_col] != 0 && abs(end_col - start_col) == 1) {
        if ((piece == 1 || piece == 7) && end_row - start_row == -1) {
            return true; // Mouvement en diagonal d'une case qui capture une pièce adverse
        }
    }

    // Capturer en passant
    if (game_state.board[end_row][end_col] == 0 && start_row == 3 && end_row == 2 && abs(end_col - start_col) == 1) {
        if (piece == 1 && end_col == game_state.en_passant_noir) {
            game_state.board[start_row][end_col] = 0;
            return true;
        } else if (piece == 7 && end_col == game_state.en_passant_blanc) {
            game_state.board[start_row][end_col] = 0;
            return true;
        }
    }
    return false; // Le mouvement n'est pas possible pour un pion
}

// Fonction pour afficher l'interface de promotion d'un pion
void show_promotion_menu(SDL_Renderer *renderer, int piece, int menu_x, int menu_y) {
    // Création de 4 cases pour les choix de promotion du pion
    SDL_Rect promotion_options[4];
    for (int i = 0; i < 4; i++) {
        promotion_options[i].x = menu_x;
        promotion_options[i].y = menu_y + i * SQUARE_SIZE;
        promotion_options[i].w = SQUARE_SIZE;
        promotion_options[i].h = SQUARE_SIZE;

        // Dessin d'un fond pour chaque option
        SDL_SetRenderDrawColor(renderer, (i % 2 == 0) ? 200 : 150, (i % 2 == 0) ? 200 : 150, 200, 255);
        SDL_RenderFillRect(renderer, &promotion_options[i]);
    }

    // Dessin des images des pièces pour chaque option
    SDL_Rect image_dest = {0, 0, SQUARE_SIZE, SQUARE_SIZE};
    SDL_Texture* promotion_textures[4];

    // Affichage des pièces de promotion dans la bonne couleur
    if (piece == 1) {
        promotion_textures[0] = white_queen;
        promotion_textures[1] = white_rook;
        promotion_textures[2] = white_knight;
        promotion_textures[3] = white_bishop;
    } else if (piece == 7) {
        promotion_textures[0] = black_queen;
        promotion_textures[1] = black_rook;
        promotion_textures[2] = black_knight;
        promotion_textures[3] = black_bishop;
    }
    
    for (int i = 0; i < 4; i++) {
        image_dest.x = promotion_options[i].x;
        image_dest.y = promotion_options[i].y;
        SDL_RenderCopy(renderer, promotion_textures[i], NULL, &image_dest);
    }

    // Mise à jour de l'écran
    SDL_RenderPresent(renderer);
}

// Fonction pour gérer la sélection de promotion
int handle_promotion_selection(int x, int y, int piece, int menu_x, int menu_y) {
    // Vérification des coordonnées du clic dans les limites du menu de promotion
    if ((x >= menu_x) && (x < menu_x + SQUARE_SIZE)) {
        if ((y >= menu_y) && (y < menu_y + 4 * SQUARE_SIZE)) {
            int row_offset = (y - menu_y) / SQUARE_SIZE;
            if (piece == 1) {  // Pion blanc
                if (row_offset == 0) return 5;  // White Queen
                if (row_offset == 1) return 2;  // White Rook
                if (row_offset == 2) return 3;  // White Knight
                if (row_offset == 3) return 4;  // White Bishop
            } else if (piece == 7) {  // Pion noir
                if (row_offset == 0) return 11;  // Black Queen
                if (row_offset == 1) return 8;   // Black Rook
                if (row_offset == 2) return 9;   // Black Knight
                if (row_offset == 3) return 10;  // Black Bishop
            }
        }
    }
    return 0;  // Aucun choix de promotion n'a été effectué
}

// Fonction pour promouvoir un pion avec un choix de pièce
void promote_pawn(int row, int col, int piece, SDL_Renderer *renderer) {
    // Calcul de la position du menu en fonction de la position du pion
    int menu_x = col * SQUARE_SIZE;
    int menu_y = row * SQUARE_SIZE;

    // Affichage le menu de promotion
    show_promotion_menu(renderer, piece, menu_x, menu_y);
    bool selecting = true; // Booléen pour fermer le menu de promotion une fois la pièce de promotion choisie
    while (selecting) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                selecting = false;
            }
            if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
                int selected_piece = handle_promotion_selection(e.button.x, e.button.y, piece, menu_x, menu_y);
                if (selected_piece != 0) {
                    // Modification l'image du pion par la pièce de promotion choisie
                    game_state.board[row][col] = selected_piece;
                    selecting = false; // Fermeture du menu de promotion
                }
            }
        }
    }
}

// Fonction pour vérifier si un déplacement est valide pour la pièce et la destination sélectionnées
bool is_valid_move(int start_row, int start_col, int end_row, int end_col) {

    // Tester si la case de départ et d'arrivée ne sont pas de la même couleur
    if (game_state.board[start_row][start_col] == 0 ||
       (game_state.board[start_row][start_col] >= 1 && game_state.board[start_row][start_col] <= 6 && game_state.board[end_row][end_col] >= 1 && game_state.board[end_row][end_col] <= 6) ||
       (game_state.board[start_row][start_col] >= 7 && game_state.board[start_row][start_col] <= 12 && game_state.board[end_row][end_col] >= 7 && game_state.board[end_row][end_col] <= 12)) {
        return false;
    }

    int piece = game_state.board[start_row][start_col];

    // Vérification des règles spécifiques à chaque pièce
    if (piece == 1 || piece == 7) { // Pion
        if (!is_valid_pawn_move(start_row, start_col, end_row, end_col, piece)) return false;
    } else if (piece == 2 || piece == 8) { // Tour
        if (!is_valid_rook_move(start_row, start_col, end_row, end_col)) return false;
    } else if (piece == 3 || piece == 9) { // Cavalier
        if (!is_valid_knight_move(start_row, start_col, end_row, end_col)) return false;
    } else if (piece == 4 || piece == 10) { // Fou
        if (!is_valid_bishop_move(start_row, start_col, end_row, end_col)) return false;
    } else if (piece == 5 || piece == 11) { // Dame
        if (!is_valid_queen_move(start_row, start_col, end_row, end_col)) return false;
    } else if (piece == 6 || piece == 12) { // Roi
        if (!is_valid_king_move(start_row, start_col, end_row, end_col)) return false;
    }
    return true; // Déplacement valide
}

// Fonction pour vérifier si le roi est en échec
bool is_king_in_check() {
    // Choix du roi de la couleur du joueur dont c'est le tour de jouer
    int king_row, king_col, king_piece;
    if (game_state.is_white_turn) {
        king_piece = 6;
    } else {
        king_piece = 12;
    }
    printf("king_piece = %d\n", king_piece);

    // Recherche du roi dans le tableau
    for (int row = 0; row < BOARD_SIZE; row++) {
        for (int col = 0; col < BOARD_SIZE; col++) {
            if (game_state.board[row][col] == king_piece) {
                king_row = row;
                king_col = col;
                break;
            }
        }
    }
    printf("King_piece in %d %d\n", king_row, king_col);

    // Vérification si une pièce adverse peut atteindre le roi sachant qu'un déplacement a été proposé
    for (int start_row = 0; start_row < BOARD_SIZE; start_row++) {
        for (int start_col = 0; start_col < BOARD_SIZE; start_col++) {
            // On regarde pour chaque pièce adverse sur l'échiquier si elle peut atteindre le roi après le déplacement proposé
            if ((game_state.is_white_turn && game_state.board[start_row][start_col] >= 7 && game_state.board[start_row][start_col] <= 12) ||
               (!game_state.is_white_turn && game_state.board[start_row][start_col] >= 1 && game_state.board[start_row][start_col] <= 6)) {
                if (is_valid_move(start_row, start_col, king_row, king_col)) {
                    printf("This piece %d can capture king from %d %d to %d %d\n", game_state.board[start_row][start_col], start_row, start_col, king_row, king_col);
                    return true; // Le roi est en échec
                }
            }
        }
    }
    printf("King not in check in that situation\n");
    return false; // Le roi n'est pas en échec dans cette situation
}

// Fonction pour vérifier si le roi est échec et mat
bool is_checkmate() {
    printf("Test du checkmate !\n");
    if (!is_king_in_check()) {
        printf("King is not in check so there is no checkmate\n");
        return false; // Pas d'échec, donc pas d'échec et mat
    }
    printf("King is in check\n"); // Ok jusque-là

    // Simulation de tous les mouvements possibles
    for (int start_row = 0; start_row < BOARD_SIZE; start_row++) {
        for (int start_col = 0; start_col < BOARD_SIZE; start_col++) {
            if ((game_state.is_white_turn && game_state.board[start_row][start_col] >= 1 && game_state.board[start_row][start_col] <= 6) ||
               (!game_state.is_white_turn && game_state.board[start_row][start_col] >= 7 && game_state.board[start_row][start_col] <= 12)) {
                for (int end_row = 0; end_row < BOARD_SIZE; end_row++) {
                    for (int end_col = 0; end_col < BOARD_SIZE; end_col++) {
                        if (is_valid_move(start_row, start_col, end_row, end_col)) {
                            // Simulation du déplacement
                            int temp_piece = game_state.board[end_row][end_col];
                            int piece = game_state.board[start_row][start_col];
                            game_state.board[start_row][start_col] = 0;
                            game_state.board[end_row][end_col] = piece;

                            bool still_in_check = is_king_in_check();

                            // Annulation du mouvement
                            game_state.board[start_row][start_col] = piece;
                            game_state.board[end_row][end_col] = temp_piece;

                            if (!still_in_check) {
                                return false; // Mouvement possible pour sortir de l'échec
                            }
                        }
                    }
                }
            }
        }
    }
    return true; // Aucun mouvement possible, échec et mat
}

// Demander aux clients de rejouer une partie ou de quitter
void handle_game_over(SDL_Renderer *renderer) {
    printf("open handle game over\n");
    SDL_Color text_color = {255, 255, 255, 255}; // Couleur du texte (blanc)
    SDL_Color bg_color = {0, 0, 0, 255}; // Couleur de fond (noir)
    TTF_Font *font = TTF_OpenFont("arial.ttf", 24); // Charger une police (assurez-vous d'avoir une police TTF)

    if (!font) {
        printf("Erreur lors du chargement de la police: %s\n", TTF_GetError());
        return;
    }

    // Créer les surfaces de texte pour les options
    SDL_Surface *replay_surface = TTF_RenderText_Solid(font, "Rejouer", text_color);
    SDL_Surface *quit_surface = TTF_RenderText_Solid(font, "Quitter", text_color);

    // Convertir les surfaces en textures
    SDL_Texture *replay_texture = SDL_CreateTextureFromSurface(renderer, replay_surface);
    SDL_Texture *quit_texture = SDL_CreateTextureFromSurface(renderer, quit_surface);

    // Définir les rectangles pour les options
    SDL_Rect replay_rect = {SCREEN_SIZE / 2 - 50, SCREEN_SIZE / 2 - 50, 100, 50};
    SDL_Rect quit_rect = {SCREEN_SIZE / 2 - 50, SCREEN_SIZE / 2 + 50, 100, 50};

    // Effacer l'écran
    SDL_SetRenderDrawColor(renderer, bg_color.r, bg_color.g, bg_color.b, bg_color.a);
    SDL_RenderClear(renderer);

    // Afficher les options
    SDL_RenderCopy(renderer, replay_texture, NULL, &replay_rect);
    SDL_RenderCopy(renderer, quit_texture, NULL, &quit_rect);
    SDL_RenderPresent(renderer);

    // Gestion des événements pour le choix de l'utilisateur
    bool choosing = true;
    while (choosing) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                choosing = false;
                game_state.restart_choice = false; // Quitter
            }
            if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
                int x = e.button.x;
                int y = e.button.y;

                // Vérifier si l'utilisateur a cliqué sur "Rejouer"
                if (x >= replay_rect.x && x <= replay_rect.x + replay_rect.w &&
                    y >= replay_rect.y && y <= replay_rect.y + replay_rect.h) {
                    game_state.restart_choice = true; // Rejouer
                    choosing = false;
                }

                // Vérifier si l'utilisateur a cliqué sur "Quitter"
                if (x >= quit_rect.x && x <= quit_rect.x + quit_rect.w &&
                    y >= quit_rect.y && y <= quit_rect.y + quit_rect.h) {
                    game_state.restart_choice = false; // Quitter
                    choosing = false;
                }
            }
        }
    }

    // Libérer les ressources
    SDL_FreeSurface(replay_surface);
    SDL_FreeSurface(quit_surface);
    SDL_DestroyTexture(replay_texture);
    SDL_DestroyTexture(quit_texture);
    TTF_CloseFont(font);
}

// Fonction pour vérifier si le roque est valide
bool is_castling_valid(int piece, int end_col) {
    // Vérification que le roi n'est pas en échec avant de roquer
    if (is_king_in_check()) {
        return false;
    }

    int start_row = 7;

    if (piece == 6 && !white_king_moved) { // Roi blanc
        // Roque court (roi vers la droite)
        if (end_col == 6 && game_state.board[start_row][7] == 2 && !white_rook2_moved) {
            if (game_state.board[start_row][5] == 0 && game_state.board[start_row][6] == 0) {
                // Tester si le roi n'est pas en échec sur une des position intermédiaires du roque
                game_state.board[start_row][4] = 0; // Supprimer le roi de sa position initiale
                game_state.board[start_row][5] = 6; // Déplacer le roi à une position intermédiare au mouvement
                if (!is_king_in_check()) {
                    game_state.board[start_row][5] = 0; // Supprimer le roi de sa position initiale
                    game_state.board[start_row][6] = 6; // Déplacer le roi à sa position finale
                    if (!is_king_in_check()) {
                        // Le roi peut roquer sans soucis d'être échec à découvert, donc on peut bouger la tour
                        game_state.board[start_row][7] = 0; // Supprimer la tour de sa position initiale
                        game_state.board[start_row][5] = 2; // Déplacer la tour à sa position finale
                        white_king_moved = true;
                        white_rook2_moved = true;
                        simul = true;
                        return true;
                    }
                }
                
                // Annuler le mouvement simulé
                game_state.board[start_row][4] = 6;
                game_state.board[start_row][5] = 0;
                game_state.board[start_row][6] = 0;
            }
        // Roque long (roi vers la gauche)
        } else if (end_col == 2 && game_state.board[start_row][0] == 2 && !white_rook1_moved) {
            if (game_state.board[start_row][1] == 0 && game_state.board[start_row][2] == 0 && game_state.board[start_row][3] == 0) {
                // Tester si le roi n'est pas en échec sur une des position intermédiaires du roque
                game_state.board[start_row][4] = 0; // Supprimer le roi de sa position initiale
                game_state.board[start_row][3] = 6; // Déplacer le roi à une position intermédiare au mouvement
                if (!is_king_in_check()) {
                    game_state.board[start_row][3] = 0; // Supprimer le roi de sa position initiale
                    game_state.board[start_row][2] = 6; // Déplacer le roi à sa position finale
                    if (!is_king_in_check()) {
                        // Le roi peut roquer sans soucis d'être échec à découvert, donc on peut bouger la tour
                        game_state.board[start_row][0] = 0; // Supprimer la tour de sa position initiale
                        game_state.board[start_row][3] = 2; // Déplacer la tour à sa position finale
                        white_king_moved = true;
                        white_rook1_moved = true;
                        simul = true;
                        return true;
                    }
                }

                // Annuler le mouvement simulé
                game_state.board[start_row][4] = 6;
                game_state.board[start_row][3] = 0;
                game_state.board[start_row][2] = 0;
            }
        }
    } else if (piece == 12 && !black_king_moved) { // Roi blanc
        // Roque court (roi vers la droite)
        if (end_col == 6 && game_state.board[start_row][7] == 8 && !black_rook2_moved) {
            if (game_state.board[start_row][5] == 0 && game_state.board[start_row][6] == 0) {
                // Tester si le roi n'est pas en échec sur une des position intermédiaires du roque
                game_state.board[start_row][4] = 0; // Supprimer le roi de sa position initiale
                game_state.board[start_row][5] = 12; // Déplacer le roi à une position intermédiare au mouvement
                if (!is_king_in_check()) {
                    game_state.board[start_row][5] = 0; // Supprimer le roi de sa position initiale
                    game_state.board[start_row][6] = 12; // Déplacer le roi à sa position finale
                    if (!is_king_in_check()) {
                        // Le roi peut roquer sans soucis d'être échec à découvert, donc on peut bouger la tour
                        game_state.board[start_row][7] = 0; // Supprimer la tour de sa position initiale
                        game_state.board[start_row][5] = 8; // Déplacer la tour à sa position finale
                        black_king_moved = true;
                        black_rook2_moved = true;
                        simul = true;
                        return true;
                    }
                }
                
                // Annuler le mouvement simulé
                game_state.board[start_row][4] = 12;
                game_state.board[start_row][5] = 0;
                game_state.board[start_row][6] = 0;
            }
        // Roque long (roi vers la gauche)
        } else if (end_col == 2 && game_state.board[start_row][0] == 8 && !black_rook1_moved) {
            if (game_state.board[start_row][1] == 0 && game_state.board[start_row][2] == 0 && game_state.board[start_row][3] == 0) {
                // Tester si le roi n'est pas en échec sur une des position intermédiaires du roque
                game_state.board[start_row][4] = 0; // Supprimer le roi de sa position initiale
                game_state.board[start_row][3] = 12; // Déplacer le roi à une position intermédiare au mouvement
                if (!is_king_in_check()) {
                    game_state.board[start_row][3] = 0; // Supprimer le roi de sa position initiale
                    game_state.board[start_row][2] = 12; // Déplacer le roi à sa position finale
                    if (!is_king_in_check()) {
                        // Le roi peut roquer sans soucis d'être échec à découvert, donc on peut bouger la tour
                        game_state.board[start_row][0] = 0; // Supprimer la tour de sa position initiale
                        game_state.board[start_row][3] = 8; // Déplacer la tour à sa position finale
                        black_king_moved = true;
                        black_rook1_moved = true;
                        simul = true;
                        return true;
                    }
                }

                // Annuler le mouvement simulé
                game_state.board[start_row][4] = 12;
                game_state.board[start_row][3] = 0;
                game_state.board[start_row][2] = 0;
            }
        }
    }
    return false; // Aucun roque valide
}

// Fonction pour gérer le clique de la souris et lui appliquer les règles d'échec
void handle_mouse_click(int x, int y, SDL_Renderer *renderer) {
    // Réinitialisation de la variable en_passant du joueur à qui c'est le tour de jouer
    if (game_state.is_white_turn) {
        game_state.en_passant_blanc = 100;
        game_state.capture_en_passant_blanc = false;
    } else if (!game_state.is_white_turn) {
        game_state.en_passant_noir = 100;
        game_state.capture_en_passant_noir = false;
    }

    int row = y / SQUARE_SIZE;
    int col = x / SQUARE_SIZE;

    //printf("Before selection : row = %d, col = %d\n", row, col);

    if (selected_piece_x == -1 && selected_piece_y == -1) { // Une pièce n'a pas encore été sélectionnée
        // Si la case sélectionnée n'est pas vide
        printf("After selection : game_state.board[row][col] = %d\n", game_state.board[row][col]);
        if (game_state.board[row][col] != 0) {
            printf("Case non_vide\n");
            printf("is_white_turn = %d\n", game_state.is_white_turn);
            printf("is_white_turn && game_state.board[row][col] <= 6 && game_state.board[row][col] >= 1 = %d\n", (game_state.is_white_turn && game_state.board[row][col] <= 6 && game_state.board[row][col] >= 1));
            printf("!is_white_turn && game_state.board[row][col] >= 7 && game_state.board[row][col] <= 12 = %d\n", (!game_state.is_white_turn && game_state.board[row][col] >= 7 && game_state.board[row][col] <= 12));
            // Si la pièce sélectionnée est de la bonne couleur pour le joueur
            if ((game_state.is_white_turn && game_state.board[row][col] <= 6 && game_state.board[row][col] >= 1) ||
               (!game_state.is_white_turn && game_state.board[row][col] >= 7 && game_state.board[row][col] <= 12)) {
                // On stocke la position de la pièce sélectionnée
                selected_piece_x = row;
                selected_piece_y = col;
                printf("After selection : slcted x = %d, slcted y = %d\n", selected_piece_x, selected_piece_y);
            }
        }
    // Une bonne pièce a été séléctionnée, row et col sont maintenant la position de la case d'arrivée
    } else {
        // On définit comme "pièce" la case sélectionnée
        int piece = game_state.board[selected_piece_x][selected_piece_y];

        // Vérifier si la case de destination n'est pas occupée par une pièce de la même couleur
        if ((game_state.is_white_turn && (game_state.board[row][col] == 0 || (game_state.board[row][col] >= 7 && game_state.board[row][col] <= 12))) || 
            (!game_state.is_white_turn && (game_state.board[row][col] == 0 || (game_state.board[row][col] >= 1 && game_state.board[row][col] <= 6)))) {
            // On définit comme "temp_piece" la case d'arrivée sélectionnée, et on stocke le contenu de cette case
            int temp_piece = 0;

            // Simule le déplacement et vérifie si le roi allié est mis en échec, auquel cas le coup est annulé et le tour recommence
            if (piece == 1 || piece == 7) {  // Pion
                if (is_valid_pawn_move(selected_piece_x, selected_piece_y, row, col, piece)){
                    // On simule le déplacement du pion
                    temp_piece = game_state.board[row][col];
                    game_state.board[selected_piece_x][selected_piece_y] = 0;
                    game_state.board[row][col] = piece;
                    simul = true;
                }
            } else if (piece == 5 || piece == 11) {  // Dame
                if (is_valid_queen_move(selected_piece_x, selected_piece_y, row, col)) {
                    // On simule le déplacement de la dame
                    temp_piece = game_state.board[row][col];
                    game_state.board[selected_piece_x][selected_piece_y] = 0;
                    game_state.board[row][col] = piece;
                    simul = true;
                }
            } else if (piece == 4 || piece == 10) {  // Fou
                if (is_valid_bishop_move(selected_piece_x, selected_piece_y, row, col)) {
                    // On simule le déplacement du fou
                    temp_piece = game_state.board[row][col];
                    game_state.board[selected_piece_x][selected_piece_y] = 0;
                    game_state.board[row][col] = piece;
                    simul = true;
                }
            } else if (piece == 2 || piece == 8) {  // Tour
                if (is_valid_rook_move(selected_piece_x, selected_piece_y, row, col)) {
                    // On simule le déplacement de la tour
                    temp_piece = game_state.board[row][col];
                    game_state.board[selected_piece_x][selected_piece_y] = 0;
                    game_state.board[row][col] = piece;
                    simul = true;
                }
            } else if (piece == 3 || piece == 9) {  // Cavalier
                if (is_valid_knight_move(selected_piece_x, selected_piece_y, row, col)) {
                    // On simule le déplacement du cavalier
                    temp_piece = game_state.board[row][col];
                    game_state.board[selected_piece_x][selected_piece_y] = 0;
                    game_state.board[row][col] = piece;
                    simul = true;
                }
            } else if (piece == 6 || piece == 12) {  // Roi
                if (is_valid_king_move(selected_piece_x, selected_piece_y, row, col)) {
                    // On simule le déplacement du roi
                    temp_piece = game_state.board[row][col];
                    game_state.board[selected_piece_x][selected_piece_y] = 0;
                    game_state.board[row][col] = piece;
                    simul = true;
                } else if (row - selected_piece_x == 0 && abs(selected_piece_y - col) == 2) {
                    if (is_castling_valid(piece,col)) {
                        // Le coup a été simulé et validé dans la fonction, rien d'autre à faire
                    }
                }
            }
            if (!is_king_in_check() && simul) { // Si le joueur a joué et que son roi n'est pas en échec, on valide le coup et on change la couleur du tour de jeu
                // Si une tour a été bougée, on doit empêcher le mouvement de roque avec cette tour
                if (selected_piece_x == 7 && selected_piece_y == 0 && piece == 2) {
                    white_rook1_moved = true;
                } else if (selected_piece_x == 7 && selected_piece_y == 7 && piece == 2) {
                    white_rook2_moved = true;
                } else if (selected_piece_x == 0 && selected_piece_y == 0 && piece == 8) {
                    black_rook1_moved = true;
                } else if (selected_piece_x == 0 && selected_piece_y == 7 && piece == 8) {
                    black_rook2_moved = true;
                // Si un roi a été bougé, on doit empêcher le mouvement de roque avec ce roi
                } else if (selected_piece_x == 7 && selected_piece_y == 4 && piece == 6) {
                    white_king_moved = true;
                } else if (selected_piece_x == 0 && selected_piece_y == 4 && piece == 12) {
                    black_king_moved = true;
                }

                // On fait la promotion du pion si elle a lieu
                if ((piece == 1 || piece == 7) && row == 0) {
                    promote_pawn(row, col, piece, renderer);
                }

                // On change les variables de tour
                printf("Turn has changed, %d is to play\n", 2 - client_id);
                game_state.is_white_turn = !game_state.is_white_turn;
                printf("is_white_turn = %d\n", game_state.is_white_turn);
                play = true;
                simul = false;

                reverse_board(game_state.board);

                // Teste si le joueur à qui ça va être le tour de jouer est en échec et mat
                if (is_checkmate()) {
                    printf("Is checkmate !!!\n");
                    reverse_board(game_state.board);
                    // Mettre game_over à true pour arrêter la partie
                    game_state.game_over = true;
                    return;
                }

                reverse_board(game_state.board);

            } else if (is_king_in_check() && simul) { // Si le joueur a joué mais que son roi est en échec, le tour recommence
                if (game_state.capture_en_passant_blanc) {
                    game_state.board[selected_piece_x][col] = 7;
                } else if (game_state.capture_en_passant_noir) {
                    game_state.board[selected_piece_x][col] = 1;
                }

                // Annuler le coup joué
                game_state.board[row][col] = temp_piece;
                game_state.board[selected_piece_x][selected_piece_y] = piece;
            }
        }

        // Réinitialiser la sélection de pièce pour le tour du joueur suivant
        selected_piece_x = -1;
        selected_piece_y = -1;
    }
}

// Fonction pour charger toutes les textures des pièces
void load_all_textures(SDL_Renderer *renderer) {
    white_pawn = IMG_LoadTexture(renderer, "white_pawn.webp");
    white_rook = IMG_LoadTexture(renderer, "white_rook.webp");
    white_knight = IMG_LoadTexture(renderer, "white_knight.webp");
    white_bishop = IMG_LoadTexture(renderer, "white_bishop.webp");
    white_king = IMG_LoadTexture(renderer, "white_king.webp");
    white_queen = IMG_LoadTexture(renderer, "white_queen.webp");

    black_pawn = IMG_LoadTexture(renderer, "black_pawn.webp");
    black_rook = IMG_LoadTexture(renderer, "black_rook.webp");
    black_knight = IMG_LoadTexture(renderer, "black_knight.webp");
    black_bishop = IMG_LoadTexture(renderer, "black_bishop.webp");
    black_king = IMG_LoadTexture(renderer, "black_king.webp");
    black_queen = IMG_LoadTexture(renderer, "black_queen.webp");
}

// Fonction pour dessiner l'échiquier
void draw_chessboard(SDL_Renderer *renderer) {
    SDL_Rect square;
    for (int row = 0; row < BOARD_SIZE; row++) {
        for (int col = 0; col < BOARD_SIZE; col++) {
            // On donne à chacune des 64 cases de l'échiquier une taille SQUARE_SIZE
            square.x = col * SQUARE_SIZE;
            square.y = row * SQUARE_SIZE;
            square.w = SQUARE_SIZE;
            square.h = SQUARE_SIZE;

            // Alternance des couleurs des cases blanches ou noires
            if ((row + col) % 2 == 0) {
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // Case blanche
            } else {
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Case noire
            }
            
            // Dessin de la case
            SDL_RenderFillRect(renderer, &square); 
        }
    }
}

// Fonction pour dessiner une pièce sur l'échiquier
void draw_piece(SDL_Renderer *renderer, SDL_Texture *texture, int row, int col) {
    // Définition la position et la taille de la pièce
    SDL_Rect dest = {col * SCREEN_SIZE / BOARD_SIZE, row * SCREEN_SIZE / BOARD_SIZE, SCREEN_SIZE / BOARD_SIZE, SCREEN_SIZE / BOARD_SIZE};

    // Dessin de la texture de la pièce sur l'échiquier
    SDL_RenderCopy(renderer, texture, NULL, &dest);
}

// Fonction pour dessiner les pièces
void draw_pieces(SDL_Renderer *renderer) {
    for (int row = 0; row < BOARD_SIZE; row++) {
        for (int col = 0; col < BOARD_SIZE; col++) {
            SDL_Texture *texture = NULL;

            // Choist de la texture de la pièce en fonction de la valeur dans le tableau "board"
            if (game_state.board[row][col] == 1) texture = white_pawn;
            if (game_state.board[row][col] == 2) texture = white_rook;
            if (game_state.board[row][col] == 3) texture = white_knight;
            if (game_state.board[row][col] == 4) texture = white_bishop;
            if (game_state.board[row][col] == 5) texture = white_queen;
            if (game_state.board[row][col] == 6) texture = white_king;
            if (game_state.board[row][col] == 7) texture = black_pawn;
            if (game_state.board[row][col] == 8) texture = black_rook;
            if (game_state.board[row][col] == 9) texture = black_knight;
            if (game_state.board[row][col] == 10) texture = black_bishop;
            if (game_state.board[row][col] == 11) texture = black_queen;
            if (game_state.board[row][col] == 12) texture = black_king;

            // Dessin de la pièce si elle existe
            if (texture != NULL) {
                draw_piece(renderer, texture, row, col);
            }
        }
    }
}

// Inverser le board du game_state
void reverse_board(int board[BOARD_SIZE][BOARD_SIZE]) {
    int temp;
    for (int row = 0; row < BOARD_SIZE / 2; row++) {
        for (int col = 0; col < BOARD_SIZE; col++) {
            // Échanger les lignes symétriques
            temp = board[row][col];
            board[row][col] = board[BOARD_SIZE - 1 - row][col];
            board[BOARD_SIZE - 1 - row][col] = temp;
        }
    }
}

// Fonction pour recevoir les données depuis le serveur
void receive_game_state_from_server(int client_socket, GameState *game_state) {
    int bytes_received = recv(client_socket, game_state, sizeof(GameState), 0);
    if (bytes_received < 0) {
        perror("Erreur lors de la réception de l'état du jeu");
        exit(EXIT_FAILURE);
    }
    printf("État du jeu reçu du serveur\n");
}

// Fonction pour envoyer les données au serveur
void send_game_state_to_server(int client_socket, GameState *game_state) {
    int bytes_sent = send(client_socket, game_state, sizeof(GameState), 0);
    if (bytes_sent < 0) {
        perror("Erreur lors de l'envoi de l'état du jeu au serveur");
        exit(EXIT_FAILURE);
    }
    printf("État du jeu envoyé au serveur\n");
}

// Fonction principale du client
void start_client(const char *server_ip) {
    // Déclaration des variables
    int client_socket;
    struct sockaddr_in server_addr;

    // Création du socket TCP du client
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("Erreur lors de la création du socket");
        exit(EXIT_FAILURE);
    }

    // Configuration de l'adresse du serveur
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(server_ip);

    // Connexion au serveur
    if (connect(client_socket, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        perror("Erreur lors de la connexion au serveur");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    // Reception de l'identifiant du client 1 joue les blancs, et 2 joue les noirs
    if (recv(client_socket, &client_id, sizeof(client_id), 0) < 0) {
        perror("Erreur lors de la réception de l'identifiant du client");
        close(client_socket);
        exit(EXIT_FAILURE);
    }
    printf("Vous êtes le client %d\n", client_id + 1);

    // Création d'un titre de fenêtre personnalisé en fonction de l'identifiant du client
    char window_title[50];  // Taille suffisante pour stocker le titre
    snprintf(window_title, sizeof(window_title), "Jeu d'échecs %d", client_id + 1);

    // Initialisation du font ttf importé pour l'interface de fin de jeu
    if (TTF_Init() < 0) {
        printf("Erreur lors de l'initialisation de SDL_ttf: %s\n", TTF_GetError());
        return; // Ou exit(EXIT_FAILURE);
    }

    // Ensuite, vous pouvez charger une police
    TTF_Font *font = TTF_OpenFont("arial.ttf", 24);
    if (!font) {
        printf("Erreur lors du chargement de la police: %s\n", TTF_GetError());
        return; // Ou exit(EXIT_FAILURE);
    }

    // Reception des données initiales du serveur
    receive_game_state_from_server(client_socket, &game_state);
    
    // Inversion du board uniquement pour le client 2 qui joue les noirs
    if (client_id == 1) {
        reverse_board(game_state.board);
    }
    printf("Client %d a reçu le Game_state du serveur\n", client_id + 1);

    // Initialisation de SDL2
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return;
    }

    // Initialisation de SDL_image
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        printf("SDL_image could not initialize! IMG_Error: %s\n", IMG_GetError());
        SDL_Quit();
        return;
    }

    // Création de la fenêtre
    SDL_Window *window = SDL_CreateWindow(window_title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_SIZE, SCREEN_SIZE, SDL_WINDOW_SHOWN);
    if (window == NULL) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        IMG_Quit();
        SDL_Quit();
        return;
    }

    // Création du renderer
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        return;
    }

    // Chargement des textures des pièces
    load_all_textures(renderer);

    // Vérification si les textures ont été chargées correctement
    if (!white_pawn || !white_rook || !white_knight || !white_bishop || !white_king || !white_queen ||
        !black_pawn || !black_rook || !black_knight || !black_bishop || !black_king || !black_queen) {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        close(client_socket);
        return;
    }

    // Dessin de l'échiquier vide
    draw_chessboard(renderer);
    printf("Chessboard drawn\n");

    // Dessine des pièces
    draw_pieces(renderer);
    printf("Pieces drawn\n");
    
    // Affichage de l'échiquier et des pièces d'échec après le coup de l'adversaire
    SDL_RenderPresent(renderer);
    printf("Board shown\n");

    // Initialisation de quelques variables utiles pour gérer le jeu
    SDL_Event e;

    // Boucle principale de gestion du client
    while (!game_state.quit) {
        // Le joueur à qui ce n'est pas le tour de joueur reçoit le Game_state du serveur
        if (game_state.is_white_turn != 1 - client_id) {
            // Reception des mises à jour du serveur
            receive_game_state_from_server(client_socket, &game_state);
            reverse_board(game_state.board);
            printf("is_white_turn = %d\n", game_state.is_white_turn);

            // Redessiner l'échiquier avec les nouvelles données
            draw_chessboard(renderer);
            draw_pieces(renderer);
            SDL_RenderPresent(renderer);
            printf("Client %d a reçu l'état du jeu\n", client_id + 1);

            // Si ce joueur est mis game_over
            printf("Game over = %d\n", game_state.game_over);
            if (game_state.game_over) {
                handle_game_over(renderer);
                send_game_state_to_server(client_socket, &game_state);
                receive_game_state_from_server(client_socket, &game_state);
                if (client_id == 1){
                    reverse_board(game_state.board);
                }
                printf("Received new Game state from server\n");

                if (!game_state.quit) {
                    // Redessiner l'échiquier après le coup de joueur dont c'est le tour de jouer
                    draw_chessboard(renderer);
                    draw_pieces(renderer);
                    SDL_RenderPresent(renderer);
                } else {
                    break;
                }
            }
        }

        // Gestion des événements SDL comme mouvement de la souris ou les touches du clavier
        while (SDL_PollEvent(&e) != 0) {
            // Fermer l'interface si l'utilisateur clique sur la croix
            if (e.type == SDL_QUIT) {
                game_state.quit = true;
            }

            // Gestion du clique du joueur à qui c'est le tour de jouer
            if (game_state.is_white_turn == 1 - client_id) {
                if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
                    printf("Clic détecté du client %d qui doit jouer.\n", client_id + 1);

                    // Le joueur fait son mouv
                    handle_mouse_click(e.button.x, e.button.y, renderer);
                    printf("Ais_white_turn = %d\n", game_state.is_white_turn);
                    printf("play = %d\n", play);  
                    
                    // Si un bon coup a été joué, afficher échiquier MAJ + envoyer à serveur
                    if (play == true) {
                        printf("Un bon coup a été joué\n");

                        // Redessiner l'échiquier après le coup de joueur dont c'est le tour de jouer
                        draw_chessboard(renderer);
                        draw_pieces(renderer);
                        SDL_RenderPresent(renderer);

                        // Passer le tour à l'adversaire
                        play = false;
                        
                        // Envoi de l'échiquier MAJ au serveur qui l'enverra à l'autre client
                        printf("Envoi du plateau au serveur...\n");
                        send_game_state_to_server(client_socket, &game_state);
                        printf("Plateau envoyé.\n");

                        // Si ce joueur a mis game_over
                        printf("Game over = %d\n", game_state.game_over);
                        if (game_state.game_over) {
                            handle_game_over(renderer);
                            send_game_state_to_server(client_socket, &game_state);
                            receive_game_state_from_server(client_socket, &game_state);
                            printf("Received new Game state from server\n");
                            printf("quit = %d\n", game_state.quit);

                            if (!game_state.quit) {
                                if (client_id == 1) {
                                    reverse_board(game_state.board);
                                }
                                
                                // Redessiner l'échiquier après le coup de joueur dont c'est le tour de jouer
                                draw_chessboard(renderer);
                                draw_pieces(renderer);
                                SDL_RenderPresent(renderer);
                            } else {
                                break;
                            }
                        }
                    }
                }
            }
        }
    }       

    // Nettoyage
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    close(client_socket);
}
