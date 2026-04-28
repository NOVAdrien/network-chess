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
    bool move_valid;
    int start_row;
    int start_col;
    int end_row;
    int end_col;
    int promotion_piece;
} GameState;

// Variable de la structure de données pour les manipuler plus facilement
GameState game_state;

// Déclaration des fonctions
SDL_Texture* load_texture(const char *file, SDL_Renderer *renderer);
void show_promotion_menu(SDL_Renderer *renderer, int piece, int menu_x, int menu_y);
int handle_promotion_selection(int x, int y, int piece, int menu_x, int menu_y);
void promote_pawn(int row, int col, int piece, SDL_Renderer *renderer);
void handle_game_over(SDL_Renderer *renderer);
void handle_mouse_click(int x, int y, SDL_Renderer *renderer);
void load_all_textures(SDL_Renderer* renderer);
void draw_chessboard(SDL_Renderer *renderer);
void draw_piece(SDL_Renderer *renderer, SDL_Texture *texture, int row, int col);
void draw_pieces(SDL_Renderer *renderer);
void reverse_board(int board[BOARD_SIZE][BOARD_SIZE]);
void receive_game_state_from_server(int client_socket, GameState *game_state);
void send_game_state_to_server(int client_socket, GameState *game_state);
void start_client(const char *server_ip);
ssize_t send_all(int socket_fd, const void *buffer, size_t length);
ssize_t recv_all(int socket_fd, void *buffer, size_t length);

// Fonction principale
int main() {
    const char *server_ip = "172.26.136.239"; // Adresse IP de la machine qui héberge le serveur ou "127.0.0.1" sur la même machine
    start_client(server_ip);
    return 0;
}

ssize_t send_all(int socket_fd, const void *buffer, size_t length) {
    size_t total_sent = 0;
    const char *data = (const char *)buffer;

    while (total_sent < length) {
        ssize_t bytes_sent = send(socket_fd, data + total_sent, length - total_sent, 0);
        if (bytes_sent <= 0) {
            return bytes_sent;
        }
        total_sent += bytes_sent;
    }
    return (ssize_t)total_sent;
}

ssize_t recv_all(int socket_fd, void *buffer, size_t length) {
    size_t total_received = 0;
    char *data = (char *)buffer;

    while (total_received < length) {
        ssize_t bytes_received = recv(socket_fd, data + total_received, length - total_received, 0);
        if (bytes_received <= 0) {
            return bytes_received;
        }
        total_received += bytes_received;
    }
    return (ssize_t)total_received;
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
    // Conservée côté client pour l'interface: le serveur applique réellement la promotion après validation.
    int menu_x = col * SQUARE_SIZE;
    int menu_y = row * SQUARE_SIZE;
    show_promotion_menu(renderer, piece, menu_x, menu_y);
    bool selecting = true;
    while (selecting) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                game_state.quit = true;
                selecting = false;
            }
            if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
                int selected_piece = handle_promotion_selection(e.button.x, e.button.y, piece, menu_x, menu_y);
                if (selected_piece != 0) {
                    game_state.promotion_piece = selected_piece;
                    selecting = false;
                }
            }
        }
    }
}

// Demander aux clients de rejouer une partie ou de quitter
void handle_game_over(SDL_Renderer *renderer) {
    printf("open handle game over\n");
    SDL_Color text_color = {255, 255, 255, 255}; // Couleur du texte (blanc)
    SDL_Color bg_color = {0, 0, 0, 255}; // Couleur de fond (noir)
    TTF_Font *font = TTF_OpenFont("LiberationSans-Regular.ttf", 24); // Charger une police (assurez-vous d'avoir une police TTF)

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

// Fonction pour gérer le clique de la souris et transmettre le coup demandé au serveur
void handle_mouse_click(int x, int y, SDL_Renderer *renderer) {
    int row = y / SQUARE_SIZE;
    int col = x / SQUARE_SIZE;

    if (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE) {
        return;
    }

    if (selected_piece_x == -1 && selected_piece_y == -1) { // Une pièce n'a pas encore été sélectionnée
        if (game_state.board[row][col] != 0) {
            // On laisse le serveur décider de la légalité; le client ne fait qu'éviter une sélection vide.
            selected_piece_x = row;
            selected_piece_y = col;
            printf("Pièce sélectionnée en %d %d\n", selected_piece_x, selected_piece_y);
        }
    } else {
        int piece = game_state.board[selected_piece_x][selected_piece_y];

        game_state.start_row = selected_piece_x;
        game_state.start_col = selected_piece_y;
        game_state.end_row = row;
        game_state.end_col = col;
        game_state.promotion_piece = 0;
        game_state.move_valid = false;

        // Le choix de promotion est une interaction d'interface; le serveur validera ensuite le coup.
        if ((piece == 1 || piece == 7) && row == 0) {
            promote_pawn(row, col, piece, renderer);
        }

        play = true;
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
    ssize_t bytes_received = recv_all(client_socket, game_state, sizeof(GameState));
    if (bytes_received <= 0) {
        perror("Erreur lors de la réception de l'état du jeu");
        exit(EXIT_FAILURE);
    }
    printf("État du jeu reçu du serveur\n");
}

// Fonction pour envoyer les données au serveur
void send_game_state_to_server(int client_socket, GameState *game_state) {
    ssize_t bytes_sent = send_all(client_socket, game_state, sizeof(GameState));
    if (bytes_sent <= 0) {
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
    if (recv_all(client_socket, &client_id, sizeof(client_id)) <= 0) {
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
        return;
    }

    // Ensuite, vous pouvez charger une police
    TTF_Font *font = TTF_OpenFont("LiberationSans-Regular.ttf", 24);
    if (!font) {
        printf("Erreur lors du chargement de la police: %s\n", TTF_GetError());
        return;
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
    draw_pieces(renderer);
    SDL_RenderPresent(renderer);

    // Initialisation de quelques variables utiles pour gérer le jeu
    SDL_Event e;

    // Boucle principale de gestion du client
    while (!game_state.quit) {
        // Le joueur à qui ce n'est pas le tour de jouer reçoit le Game_state du serveur
        if (game_state.is_white_turn != 1 - client_id) {
            // Reception des mises à jour du serveur
            receive_game_state_from_server(client_socket, &game_state);
            if (game_state.quit) {
                break;
            }
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
                    // Redessiner l'échiquier après réinitialisation
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

                    // Le joueur choisit un départ et une arrivée; le serveur validera le coup.
                    handle_mouse_click(e.button.x, e.button.y, renderer);

                    // Si deux clics ont formé un coup, l'envoyer au serveur et attendre la réponse.
                    if (play == true) {
                        printf("Envoi du coup au serveur...\n");
                        send_game_state_to_server(client_socket, &game_state);
                        play = false;

                        receive_game_state_from_server(client_socket, &game_state);

                        // La réponse à notre propre coup est déjà dans notre orientation locale.
                        draw_chessboard(renderer);
                        draw_pieces(renderer);
                        SDL_RenderPresent(renderer);

                        if (!game_state.move_valid) {
                            printf("Coup invalide: le même joueur doit rejouer.\n");
                            continue;
                        }

                        printf("Coup valide.\n");

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

                                // Redessiner l'échiquier après réinitialisation
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
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();
    close(client_socket);
}
