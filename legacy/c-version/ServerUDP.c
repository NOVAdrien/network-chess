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
int send_game_state_to_client(int client_socket, struct sockaddr_in *client_addr);
int receive_game_state_from_client(int client_socket, struct sockaddr_in *client_addr);
void handle_clients(int server_socket);
void start_server();

// Fonction principale
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
int send_game_state_to_client(int client_socket, struct sockaddr_in *client_addr) {
    int bytes_sent = sendto(client_socket, &game_state, sizeof(GameState), 0, (struct sockaddr *)client_addr, sizeof(*client_addr));
    if (bytes_sent < 0) {
        perror("Erreur lors de l'envoi de l'échiquier");
        return -1;
    }
    return 0;
}

// Fonction pour recevoir la struture de données d'un client
int receive_game_state_from_client(int client_socket, struct sockaddr_in *client_addr) {
    socklen_t addr_len = sizeof(*client_addr);
    int bytes_received = recvfrom(client_socket, &game_state, sizeof(GameState), 0, (struct sockaddr *)client_addr, &addr_len);
    if (bytes_received <= 0) {
        if (bytes_received == 0) {
            printf("Client déconnecté.\n");
        } else {
            perror("Erreur lors de la réception de l'échiquier");
        }
        return -1;
    }
    return 0;
}

// Fonction pour gérer la communication avec les clients
void handle_clients(int server_socket) {

    // Initialisation de la structure de données en début de partie
    init_game_state();

    struct sockaddr_in client1_addr, client2_addr;
    socklen_t addr_len = sizeof(client1_addr);

    printf("En attente des clients...\n");

    // Recevoir le premier client
    if (recvfrom(server_socket, NULL, 0, 0, (struct sockaddr *)&client1_addr, &addr_len) < 0) {
        perror("Erreur lors de la réception du premier client\n");
        return;
    }

    printf("Client 1 connecté\n");

    // Après avoir reçu le message initial du client 1
    int client_id = 0;
    if (sendto(server_socket, &client_id, sizeof(client_id), 0, (struct sockaddr *)&client1_addr, sizeof(client1_addr)) < 0) {
        perror("Erreur lors de l'envoi de l'identifiant au client 1");
        return;
    }

    // Recevoir le deuxième client
    if (recvfrom(server_socket, NULL, 0, 0, (struct sockaddr *)&client2_addr, &addr_len) < 0) {
        perror("Erreur lors de la réception du deuxième client");
        return;
    }

    printf("Client 2 connecté\n");

    // Après avoir reçu le message initial du client 2
    client_id = 1;
    if (sendto(server_socket, &client_id, sizeof(client_id), 0, (struct sockaddr *)&client2_addr, sizeof(client2_addr)) < 0) {
        perror("Erreur lors de l'envoi de l'identifiant au client 2");
        return;
    }

    printf("Envoi de l'échiquier\n");
    
    // Envoyer l'échiquier initial aux deux clients
    if (send_game_state_to_client(server_socket, &client1_addr) < 0 || send_game_state_to_client(server_socket, &client2_addr) < 0) {
        printf("Erreur lors de l'envoi initial. Fermeture de la connexion.\n");
        return;
    }

    printf("Echiquiers envoyés\n");

    // Variables pour gérer l'alternance du tour de jeu
    struct sockaddr_in *current_client = &client1_addr;
    struct sockaddr_in *other_client = &client2_addr;

    while (1) {
        // Recevoir les données du joueur à qui c'est le tour de jouer
        if (receive_game_state_from_client(server_socket, current_client) < 0) {
            printf("Le joueur s'est déconnecté. Fin de la partie.\n");
            break;
        }

        // Envoyer les données à l'autre joueur
        if (send_game_state_to_client(server_socket, other_client) < 0) {
            printf("Erreur lors de l'envoi des données à l'autre joueur. Fin de la partie.\n");
            break;
        }

        // Changer de tour
        struct sockaddr_in *temp = current_client;
        current_client = other_client;
        other_client = temp;

        // Si game_over == True
        if (game_state.game_over == true) {
            receive_game_state_from_client(server_socket, other_client);
            int restart_choice_other_client = game_state.restart_choice;

            printf("J'ai reçu le Game_state du joueur qui a mis Game over\n");

            receive_game_state_from_client(server_socket, current_client);
            int restart_choice_current_client = game_state.restart_choice;

            printf("J'ai reçu le Game_state du joueur qui est mis Game over\n");

            printf("restart_choice_player_socket = %d, restart_choice_other_player_socket = %d\n", restart_choice_current_client, restart_choice_other_client);

            // Les deux joueurs souhaitent rejouer une partie
            if (restart_choice_current_client == 1 && restart_choice_other_client == 1) {
                printf("ils veulent rejouer\n");

                // Réinitialisation de la structure de données en début de partie
                init_game_state();

                printf("game_state.is_white_turn = %d\n", game_state.is_white_turn);
                
                // Envoi de l'état réinitialisé du jeu aux deux clients
                send_game_state_to_client(server_socket, current_client);
                send_game_state_to_client(server_socket, other_client);

                printf("Game_state envoyée aux deux client\n");

                // Variables pour gérer l'alternance du tour de jeu
                current_client = &client1_addr;
                other_client = &client2_addr;
            } else {
                printf("un joueur a voulu quit\n");
                game_state.game_over = false;
                game_state.quit = true;

                printf("game_state.is_white_turn = %d\n", game_state.is_white_turn);

                // Envoi de l'état réinitialisé du jeu aux deux clients
                send_game_state_to_client(server_socket, current_client);
                send_game_state_to_client(server_socket, other_client);

                printf("Game_state envoyée aux deux client\n");
                
                break;
            }
        }
    }
}

// Fonction principale du serveur
void start_server() {

    // Déclaration des variables
    int server_socket;
    struct sockaddr_in server_addr;

    // Création du socket UDP du serveur
    server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_socket == -1) {
        perror("Erreur lors de la création du socket");
        exit(EXIT_FAILURE);
    }

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

    printf("Serveur démarré et en écoute\n");

    // Gestion de la communication entre les deux clients
    handle_clients(server_socket);

    // Fermeture du socket après la fin du jeu
    close(server_socket);

    printf("Serveur fermé proprement.\n");
}