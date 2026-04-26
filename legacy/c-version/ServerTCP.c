#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <SDL2/SDL_ttf.h>

#define PORT 30000
#define BOARD_SIZE 8

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
} GameState;

GameState game_state;

// Déclaration des fonctions
void init_board();
void init_game_state();
int send_game_state_to_client(int client_socket);
int receive_game_state_from_client(int client_socket);
void handle_clients(int client1_socket, int client2_socket);
void start_server();

// Fonction princpiale
int main() {
    start_server();
    return 0;
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
    init_board();
    game_state.is_white_turn = true;
    game_state.quit = false;
    game_state.game_over = false;
    game_state.restart_choice = 0;
    game_state.en_passant_blanc = 100;
    game_state.en_passant_noir = 100;
    game_state.capture_en_passant_blanc = false;
    game_state.capture_en_passant_noir = false;
}

// Fonction pour envoyer la structure de données à un client
int send_game_state_to_client(int client_socket) {
    int bytes_sent = send(client_socket, &game_state, sizeof(GameState), 0);
    if (bytes_sent < 0) {
        perror("Erreur lors de l'envoi de l'état du jeu");
        return -1;
    }
    return 0;
}

// Fonction pour recevoir la struture de données d'un client
int receive_game_state_from_client(int client_socket) {
    int bytes_received = recv(client_socket, &game_state, sizeof(GameState), 0);
    if (bytes_received <= 0) {
        if (bytes_received == 0) {
            printf("Client déconnecté.\n");
        } else {
            perror("Erreur lors de la réception de l'état du jeu");
        }
        return -1;
    }
    return 0;
}

// Fonction pour gérer la communication avec les clients
void handle_clients(int client1_socket, int client2_socket) {
    // Initialisation de la structure de données en début de partie
    init_game_state();

    printf("game_state.is_white_turn = %d\n", game_state.is_white_turn);

    // Envoi de l'état initial du jeu aux deux clients
    send_game_state_to_client(client1_socket);
    send_game_state_to_client(client2_socket);

    printf("Game_state envoyée aux deux client\n");
    printf("Game over = %d\n", game_state.game_over);

    // Variables pour gérer l'alternance du tour de jeu
    int client_socket[2] = {client1_socket, client2_socket};
    int player_socket = 0;
    int other_player_socket = 1;

    while (1) {
        // Reception des données du joueur à qui c'est le tour de jouer
        receive_game_state_from_client(client_socket[player_socket]);

        printf("J'ai reçu le Game_state du joueur qui vient de jouer\n");
        printf("Game over = %d\n", game_state.game_over);

        // Envoi des données au joueur à qui c'est le tour de jouer
        send_game_state_to_client(client_socket[other_player_socket]);

        printf("J'ai envoyé le Game_state au joueur à qui c'est le tour de jouer\n");
        printf("Game over = %d\n", game_state.game_over);

        // Changement du tour de joueur
        player_socket = 1 - player_socket;
        other_player_socket = 1 - other_player_socket;

        // Si game_over == True
        if (game_state.game_over == true) {
            receive_game_state_from_client(client_socket[other_player_socket]);
            int restart_choice_other_player_socket = game_state.restart_choice;

            printf("J'ai reçu le Game_state du joueur qui a mis Game over\n");

            receive_game_state_from_client(client_socket[player_socket]);
            int restart_choice_player_socket = game_state.restart_choice;

            printf("J'ai reçu le Game_state du joueur qui est mis Game over\n");

            printf("restart_choice_player_socket = %d, restart_choice_other_player_socket = %d\n", restart_choice_player_socket, restart_choice_other_player_socket);

            // Les deux joueurs souhaitent rejouer une partie
            if (restart_choice_player_socket == 1 && restart_choice_other_player_socket == 1) {
                // Réinitialisation de la structure de données en début de partie
                init_game_state();

                printf("game_state.is_white_turn = %d\n", game_state.is_white_turn);
                
                // Envoi de l'état réinitialisé du jeu aux deux clients
                send_game_state_to_client(client1_socket);
                send_game_state_to_client(client2_socket);

                printf("Game_state envoyée aux deux client\n");

                // Changement du tour de joueur pour bien recommencer une partie
                player_socket = 0;
                other_player_socket = 1;
            } else {
                game_state.game_over = false;
                game_state.quit = true;

                printf("game_state.is_white_turn = %d\n", game_state.is_white_turn);

                // Envoi de l'état réinitialisé du jeu aux deux clients
                send_game_state_to_client(client1_socket);
                send_game_state_to_client(client2_socket);

                printf("Game_state envoyée aux deux client\n");
                
                break;
            }
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

        printf("I'm sending the id to the clients\n");
        
        // Envoi de l'identifiant 1 ou 2 aux clients
        int client_id = i;
        if (send(client_socket[i], &client_id, sizeof(client_id), 0) < 0) {
            perror("Erreur lors de l'envoi de l'identifiant du client");
            close(client_socket[i]);
            close(server_socket);
            exit(EXIT_FAILURE);
        }
        
        printf("Identifiant %d envoyé au client\n", client_id);
    }

    printf("Launch handle client\n");

    // Gestion de la communication entre les deux clients
    handle_clients(client_socket[0], client_socket[1]);

    // Fermeture des sockets après la fin du jeu
    close(client_socket[0]);
    close(client_socket[1]);
    close(server_socket);

    printf("Serveur fermé proprement.\n");
}
