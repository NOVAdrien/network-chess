#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 30000
#define BOARD_SIZE 8

// Variables pour gérer le tour de jeu et les simulations côté serveur
bool play = false;
bool simul = false;

// Tests qui vérifient si les pièces utilisées dans le roque ont déjà bougé
bool white_rook1_moved = false;
bool white_rook2_moved = false;
bool black_rook1_moved = false;
bool black_rook2_moved = false;
bool white_king_moved = false;
bool black_king_moved = false;

// Variables utilisées pour gérer la sélection ou non d'une pièce adéquate
int selected_piece_x = -1;
int selected_piece_y = -1;

// Structure des données échangées entre clients et serveur
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

GameState game_state;

// Déclaration des fonctions
void init_board();
void init_game_state();
int send_game_state_to_client(int client_socket);
int receive_game_state_from_client(int client_socket);
void handle_clients(int client1_socket, int client2_socket);
void start_server();
bool is_horizontal_path_clear(int start_row, int start_col, int end_col);
bool is_vertical_path_clear(int start_row, int start_col, int end_row);
bool is_diagonal_path_clear(int start_row, int start_col, int end_row, int end_col);
bool is_valid_bishop_move(int start_row, int start_col, int end_row, int end_col);
bool is_valid_rook_move(int start_row, int start_col, int end_row, int end_col);
bool is_valid_knight_move(int start_row, int start_col, int end_row, int end_col);
bool is_valid_queen_move(int start_row, int start_col, int end_row, int end_col);
bool is_valid_king_move(int start_row, int start_col, int end_row, int end_col);
bool is_valid_pawn_move(int start_row, int start_col, int end_row, int end_col, int piece);
bool is_valid_move(int start_row, int start_col, int end_row, int end_col);
bool is_king_in_check();
bool is_checkmate();
bool is_castling_valid(int piece, int end_col);
void reverse_board(int board[BOARD_SIZE][BOARD_SIZE]);
bool handle_move_from_client(int player_socket);
int get_promotion_piece(int piece, int requested_piece);
ssize_t send_all(int socket_fd, const void *buffer, size_t length);
ssize_t recv_all(int socket_fd, void *buffer, size_t length);

// Fonction princpiale
int main() {
    start_server();
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

// Fonction pour placer les pièces sur l'échiquier
void init_board() {
    // Pièces blanches
    for (int i = 0; i < BOARD_SIZE; i++) game_state.board[6][i] = 1;  // Pions
    game_state.board[7][0] = 2; game_state.board[7][7] = 2; // Tours
    game_state.board[7][1] = 3; game_state.board[7][6] = 3; // Cavaliers
    game_state.board[7][2] = 4; game_state.board[7][5] = 4; // Fous
    game_state.board[7][3] = 5;  // Dame
    game_state.board[7][4] = 6; // Roi

    // Pièces noires
    for (int i = 0; i < BOARD_SIZE; i++) game_state.board[1][i] = 7;  // Pions
    game_state.board[0][0] = 8; game_state.board[0][7] = 8; // Tours
    game_state.board[0][1] = 9; game_state.board[0][6] = 9; // Cavaliers
    game_state.board[0][2] = 10; game_state.board[0][5] = 10; // Fous
    game_state.board[0][3] = 11; // Dame
    game_state.board[0][4] = 12; // Roi

    // Cases vides
    for (int i = 2; i < 6; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            game_state.board[i][j] = 0;
        }
    }
}

// Fonction pour (ré)initialiser une partie
void init_game_state() {
    // Initialisation de la structure de données en début de partie
    memset(&game_state, 0, sizeof(GameState));
    init_board();
    game_state.is_white_turn = true;
    game_state.quit = false;
    game_state.game_over = false;
    game_state.restart_choice = false;
    game_state.en_passant_blanc = 100;
    game_state.en_passant_noir = 100;
    game_state.capture_en_passant_blanc = false;
    game_state.capture_en_passant_noir = false;
    game_state.move_valid = true;
    game_state.start_row = -1;
    game_state.start_col = -1;
    game_state.end_row = -1;
    game_state.end_col = -1;
    game_state.promotion_piece = 0;

    play = false;
    simul = false;
    selected_piece_x = -1;
    selected_piece_y = -1;
    white_rook1_moved = false;
    white_rook2_moved = false;
    black_rook1_moved = false;
    black_rook2_moved = false;
    white_king_moved = false;
    black_king_moved = false;
}

// Fonction pour envoyer la structure de données à un client
int send_game_state_to_client(int client_socket) {
    ssize_t bytes_sent = send_all(client_socket, &game_state, sizeof(GameState));
    if (bytes_sent <= 0) {
        perror("Erreur lors de l'envoi de l'état du jeu");
        return -1;
    }
    return 0;
}

// Fonction pour recevoir la structure de données d'un client
int receive_game_state_from_client(int client_socket) {
    GameState received_state;
    ssize_t bytes_received = recv_all(client_socket, &received_state, sizeof(GameState));
    if (bytes_received <= 0) {
        if (bytes_received == 0) {
            printf("Client déconnecté.\n");
        } else {
            perror("Erreur lors de la réception de l'état du jeu");
        }
        return -1;
    }

    // Le client ne décide plus du plateau: il transmet uniquement le coup et les choix d'interface.
    game_state.start_row = received_state.start_row;
    game_state.start_col = received_state.start_col;
    game_state.end_row = received_state.end_row;
    game_state.end_col = received_state.end_col;
    game_state.promotion_piece = received_state.promotion_piece;
    game_state.restart_choice = received_state.restart_choice;
    if (received_state.quit) {
        game_state.quit = true;
    }

    return 0;
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
    int direction;
    int initial_row;
    int en_passant_row;
    int en_passant_capture_row;

    if (piece == 1) { // Pion blanc
        direction = -1;
        initial_row = 6;
        en_passant_row = 3;
        en_passant_capture_row = 2;
    } else if (piece == 7) { // Pion noir
        direction = 1;
        initial_row = 1;
        en_passant_row = 4;
        en_passant_capture_row = 5;
    } else {
        return false;
    }

    // Déplacement d'une case vers l'avant
    if (start_col == end_col && game_state.board[end_row][end_col] == 0) {
        if (end_row - start_row == direction) {
            return true;
        }

        // Déplacement de deux cases depuis la position initiale
        if (start_row == initial_row && end_row - start_row == 2 * direction) {
            int intermediate_row = start_row + direction;

            if (game_state.board[intermediate_row][start_col] == 0) {
                return true;
            }
        }
    }

    // Capture diagonale normale
    if (game_state.board[end_row][end_col] != 0 && abs(end_col - start_col) == 1) {
        if (end_row - start_row == direction) {
            return true;
        }
    }

    // Capture en passant
    if (game_state.board[end_row][end_col] == 0 &&
        start_row == en_passant_row &&
        end_row == en_passant_capture_row &&
        abs(end_col - start_col) == 1) {

        if (piece == 1 && end_col == game_state.en_passant_noir) {
            return true;
        } else if (piece == 7 && end_col == game_state.en_passant_blanc) {
            return true;
        }
    }

    return false;
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
    int king_row = -1;
    int king_col = -1;
    int king_piece;
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

    if (king_row == -1 || king_col == -1) {
        printf("Erreur : roi introuvable sur le plateau\n");
        return true;
    }

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

// Fonction pour vérifier si le roque est valide
bool is_castling_valid(int piece, int end_col) {
    // Vérification que le roi n'est pas en échec avant de roquer
    if (is_king_in_check()) {
        return false;
    }

    int start_row = (piece == 6) ? 7 : 0;

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
    } else if (piece == 12 && !black_king_moved) { // Roi noir
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


int get_promotion_piece(int piece, int requested_piece) {
    if (piece == 1) {
        if (requested_piece == 5 || requested_piece == 2 || requested_piece == 3 || requested_piece == 4) {
            return requested_piece;
        }
        return 5;
    } else if (piece == 7) {
        if (requested_piece == 11 || requested_piece == 8 || requested_piece == 9 || requested_piece == 10) {
            return requested_piece;
        }
        return 11;
    }
    return requested_piece;
}

// Fonction pour gérer un coup reçu du client et lui appliquer les règles d'échec côté serveur
bool handle_move_from_client(int player_socket) {
    game_state.move_valid = false;
    play = false;
    simul = false;

    selected_piece_x = game_state.start_row;
    selected_piece_y = game_state.start_col;
    int row = game_state.end_row;
    int col = game_state.end_col;

    // Le client noir voit le plateau inversé verticalement.
    // Le serveur garde toujours le plateau dans l'orientation blanche.
    // On convertit donc uniquement les lignes reçues du joueur noir.
    if (player_socket == 1) {
        selected_piece_x = 7 - selected_piece_x;
        row = 7 - row;
    }

    if (selected_piece_x < 0 || selected_piece_x >= BOARD_SIZE ||
        selected_piece_y < 0 || selected_piece_y >= BOARD_SIZE ||
        row < 0 || row >= BOARD_SIZE ||
        col < 0 || col >= BOARD_SIZE) {
        selected_piece_x = -1;
        selected_piece_y = -1;
        return false;
    }

    // Réinitialisation de la variable en_passant du joueur à qui c'est le tour de jouer
    if (game_state.is_white_turn) {
        game_state.en_passant_blanc = 100;
        game_state.capture_en_passant_blanc = false;
    } else if (!game_state.is_white_turn) {
        game_state.en_passant_noir = 100;
        game_state.capture_en_passant_noir = false;
    }

    if (selected_piece_x != -1 && selected_piece_y != -1) {
        // On définit comme "pièce" la case sélectionnée
        int piece = game_state.board[selected_piece_x][selected_piece_y];

        // Vérifier si la pièce sélectionnée appartient bien au joueur dont c'est le tour
        if ((game_state.is_white_turn && !(piece >= 1 && piece <= 6)) ||
            (!game_state.is_white_turn && !(piece >= 7 && piece <= 12))) {
            selected_piece_x = -1;
            selected_piece_y = -1;
            return false;
        }

        // Vérifier si la case de destination n'est pas occupée par une pièce de la même couleur
        if ((game_state.is_white_turn && (game_state.board[row][col] == 0 || (game_state.board[row][col] >= 7 && game_state.board[row][col] <= 12))) || 
            (!game_state.is_white_turn && (game_state.board[row][col] == 0 || (game_state.board[row][col] >= 1 && game_state.board[row][col] <= 6)))) {
            // On définit comme "temp_piece" la case d'arrivée sélectionnée, et on stocke le contenu de cette case
            int temp_piece = 0;
            int en_passant_captured_piece = 0;
            int en_passant_captured_row = -1;
            bool en_passant_capture = false;
            bool pawn_double_step = false;

            // Simule le déplacement et vérifie si le roi allié est mis en échec, auquel cas le coup est annulé et le tour recommence
            if (piece == 1 || piece == 7) {  // Pion
                if (is_valid_pawn_move(selected_piece_x, selected_piece_y, row, col, piece)){
                    // On simule le déplacement du pion
                    temp_piece = game_state.board[row][col];

                    // Détection du double déplacement de pion
                    if (abs(row - selected_piece_x) == 2) {
                        pawn_double_step = true;
                    }

                    // Détection de la capture en passant :
                    // destination vide + déplacement diagonal
                    if (game_state.board[row][col] == 0 && selected_piece_y != col) {
                        en_passant_capture = true;
                        en_passant_captured_row = selected_piece_x;
                        en_passant_captured_piece = game_state.board[en_passant_captured_row][col];

                        game_state.board[en_passant_captured_row][col] = 0;

                        if (piece == 1) {
                            game_state.capture_en_passant_blanc = true;
                        } else if (piece == 7) {
                            game_state.capture_en_passant_noir = true;
                        }
                    }

                    // On simule le déplacement du pion
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

                // Si un pion vient de faire un double pas, on mémorise la colonne pour l'en passant
                if (pawn_double_step) {
                    if (piece == 1) {
                        game_state.en_passant_blanc = col;
                    } else if (piece == 7) {
                        game_state.en_passant_noir = col;
                    }
                }

                // On fait la promotion du pion si elle a lieu
                if ((piece == 1 && row == 0) || (piece == 7 && row == 7)) {
                    game_state.board[row][col] = get_promotion_piece(piece, game_state.promotion_piece);
                }

                // On change les variables de tour
                game_state.is_white_turn = !game_state.is_white_turn;
                play = true;
                simul = false;
                game_state.move_valid = true;

                // Teste si le joueur à qui ça va être le tour de jouer est en échec et mat
                if (is_checkmate()) {
                    printf("Is checkmate !!!\n");
                    game_state.game_over = true;
                    selected_piece_x = -1;
                    selected_piece_y = -1;
                    return true;
                }

                selected_piece_x = -1;
                selected_piece_y = -1;
                return true;

            } else if (is_king_in_check() && simul) { // Si le joueur a joué mais que son roi est en échec, le tour recommence
                // Annuler la capture en passant si elle avait été simulée
                if (en_passant_capture && en_passant_captured_row != -1) {
                    game_state.board[en_passant_captured_row][col] = en_passant_captured_piece;
                }

                // Annuler le coup joué
                game_state.board[row][col] = temp_piece;
                game_state.board[selected_piece_x][selected_piece_y] = piece;

                game_state.capture_en_passant_blanc = false;
                game_state.capture_en_passant_noir = false;
            }
        }

        // Réinitialiser la sélection de pièce pour le tour du joueur suivant
        selected_piece_x = -1;
        selected_piece_y = -1;
    }

    return false;
}

// Fonction pour gérer la communication avec les clients
void handle_clients(int client1_socket, int client2_socket) {
    // Initialisation de la structure de données en début de partie
    init_game_state();

    // Envoi de l'état initial du jeu aux deux clients
    send_game_state_to_client(client1_socket);
    send_game_state_to_client(client2_socket);

    // Variables pour gérer l'alternance du tour de jeu
    int client_socket[2] = {client1_socket, client2_socket};
    int player_socket = 0;
    int other_player_socket = 1;

    while (1) {
        // Reception du coup du joueur à qui c'est le tour de jouer
        if (receive_game_state_from_client(client_socket[player_socket]) < 0) {
            break;
        }

        bool valid_move = handle_move_from_client(player_socket);

        if (!valid_move) {
            printf("Coup invalide reçu du joueur %d. Le même joueur rejoue.\n", player_socket + 1);
            game_state.move_valid = false;
            send_game_state_to_client(client_socket[player_socket]);
            continue;
        }

        printf("Coup valide reçu du joueur %d.\n", player_socket + 1);
        game_state.move_valid = true;

        // Le joueur qui a joué reçoit aussi la réponse de légalité et le plateau officiel.
        send_game_state_to_client(client_socket[player_socket]);

        // L'autre joueur reçoit le même plateau; son client l'inversera pour l'affichage.
        send_game_state_to_client(client_socket[other_player_socket]);

        // Si game_over == True
        if (game_state.game_over == true) {
            if (receive_game_state_from_client(client_socket[player_socket]) < 0) {
                break;
            }
            int restart_choice_player_socket = game_state.restart_choice;

            if (receive_game_state_from_client(client_socket[other_player_socket]) < 0) {
                break;
            }
            int restart_choice_other_player_socket = game_state.restart_choice;

            printf("restart_choice_player_socket = %d, restart_choice_other_player_socket = %d\n", restart_choice_player_socket, restart_choice_other_player_socket);

            // Les deux joueurs souhaitent rejouer une partie
            if (restart_choice_player_socket == 1 && restart_choice_other_player_socket == 1) {
                // Réinitialisation de la structure de données en début de partie
                init_game_state();
                send_game_state_to_client(client1_socket);
                send_game_state_to_client(client2_socket);

                // Changement du tour de joueur pour bien recommencer une partie
                player_socket = 0;
                other_player_socket = 1;
            } else {
                game_state.game_over = false;
                game_state.quit = true;
                send_game_state_to_client(client1_socket);
                send_game_state_to_client(client2_socket);
                break;
            }
        } else {
            // Changement du tour de joueur uniquement si le coup était valide
            player_socket = 1 - player_socket;
            other_player_socket = 1 - other_player_socket;
        }
    }
}

// Fonction principale du serveur
void start_server() {

    // Déclaration des variables
    int server_socket, client_socket[2];
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;
    int opt = 1;

    // Création du socket TCP du serveur
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Erreur lors de la création du socket");
        exit(EXIT_FAILURE);
    }

    // Configuration du socket pour réutiliser l'adresse
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Configuration de l'adresse du serveur
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Liaison du socket à l'adresse du serveur
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Erreur lors du bind");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Mise en écoute du socket pour accepter les connexions entrantes
    if (listen(server_socket, 2) < 0) {
        perror("Erreur lors de l'écoute");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Serveur démarré et en écoute\n");

    // Boucle pour accepter les connexions des deux clients
    for (int i = 0; i < 2; i++) {
        addr_size = sizeof(client_addr);
        client_socket[i] = accept(server_socket, (struct sockaddr*)&client_addr, &addr_size);
        if (client_socket[i] < 0) {
            perror("Erreur lors de l'acceptation de la connexion");
            close(server_socket);
            exit(EXIT_FAILURE);
        }

        printf("Client %d connecté\n", i+1);

        // Envoi de l'identifiant 1 ou 2 aux clients
        int client_id = i;
        if (send_all(client_socket[i], &client_id, sizeof(client_id)) <= 0) {
            perror("Erreur lors de l'envoi de l'identifiant du client");
            close(client_socket[i]);
            close(server_socket);
            exit(EXIT_FAILURE);
        }

        printf("Identifiant %d envoyé au client\n", client_id);
    }

    // Gestion de la communication entre les deux clients
    handle_clients(client_socket[0], client_socket[1]);

    // Fermeture des sockets après la fin du jeu
    close(client_socket[0]);
    close(client_socket[1]);
    close(server_socket);

    printf("Serveur fermé proprement.\n");
}
