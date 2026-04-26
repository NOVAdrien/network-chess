# Jeu d'Échecs en Réseau

Ce projet est une implémentation d'un jeu d'échecs en réseau, où deux joueurs peuvent s'affronter à distance. Le jeu est composé d'un code serveur et d'un code client qui permettent la connexion via TCP (Transmission Control Protocol) ou UDP (User Datagram Protocol) de deux joueurs. Le serveur gère la logique du jeu et la communication entre les deux clients, tandis que chaque client gère l'interface graphique et les interactions utilisateur.

## Prérequis

Pour compiler et exécuter ce projet, vous aurez besoin des éléments suivants :

- Compilateur C : GCC ou un autre compilateur C.
- Bibliothèques SDL2 et SDL2_image : Pour l'interface graphique.
- Bibliothèques réseau : Les bibliothèques standard pour la gestion des sockets (arpa/inet.h, sys/socket.h).

## Installation des dépendances

Sur une distribution Linux basée sur Debian (comme Ubuntu), vous pouvez installer les dépendances nécessaires avec les commandes suivantes :

- sudo apt update
- sudo apt install build-essential libsdl2-dev libsdl2-image-dev

Vous avez à votre disposition deux codes que vous pouvez installer. L'un utilise le protocole TCP (ClientTCP.c et ServerTCP.c) et l'autre utilise le protocole UDP (ClientUDP.c et ServerUDP.c).

## Compilation

### Compilation du Serveur

Pour compiler le serveur, utilisez une des commandes suivantes selon le type de connexion TCP ou UDP utilisé :

- gcc ServerTCP.c -o servertcp
- gcc ServerUDP.c -o serverudp

### Compilation du Client

Pour compiler le client, utilisez une des commandes suivantes selon le type de connexion TCP ou UDP utilisé :

- gcc ClientTCP.c -o clienttcp -lSDL2 -lSDL2_image -lSDL2_ttf
- gcc ClientUDP.c -o clientudp -lSDL2 -lSDL2_image -lSDL2_ttf

## Exécution

### Lancement du Serveur

Lancez le serveur en exécutant une des commandes suivantes selon le type de connexion TCP ou UDP utilisé :

- ./servertcp
- ./serverudp

Le serveur écoutera sur le port 30000 et attendra que deux clients se connectent.

### Lancement des Clients

Lancez le premier client en exécutant une des commandes suivantes selon le type de connexion TCP ou UDP utilisé :

- ./clienttcp
- ./clientudp

Le client se connectera automatiquement au serveur en utilisant l'adresse IPV4 codée en dur dans le fichier client. Pour trouver cette adresse IP, il faut entrer la commande ifconfig dans le terminal de la machine où le serveur est lancé (dans notre cas "172.26.136.239"), et l'insérer dans le code client dans le main à la ligne "const char *server_ip = "172.26.136.239";".

Lancez le deuxième client de la même manière :

- ./clienttcp
- ./clientudp

Le deuxième client se connectera au serveur et le jeu commencera.

## Fonctionnement du Jeu

Le client 1 joue avec les pièces blanches et le client 2 joue avec les pièces noires. Les joueurs alternent les tours pour déplacer leurs pièces. Le serveur gère la synchronisation des mouvements entre les deux clients.

## Règles du Jeu

Le jeu suit les règles classiques des échecs, y compris les mouvements spéciaux comme le roque, la prise en passant, et la promotion de pions.

## Gestion des Erreurs

Si un client se déconnecte, le serveur en informe l'autre client et met fin à la partie. Si le serveur rencontre une erreur lors de la communication, il ferme les connexions et termine proprement.

## Nettoyage

Après la fin de la partie, le serveur et les clients ferment leurs sockets et libèrent les ressources.

## Améliorations Possibles

- Interface graphique améliorée : Ajouter des animations, des effets sonores.
- Gestion des erreurs réseau : Améliorer la gestion des déconnexions inattendues.
- Mode spectateur : Permettre à des observateurs de regarder la partie en cours.
- Autre mode de jeu : Permettre à un seul client de jouer contre l'ordinateur aux échecs.
- Challenge dans le jeu : Ajouter un temps de jeu limité pour chaque joueur.

## Auteurs

Ce projet a été développé par moi-même, Adrien PANGUEL, dans le cadre d'un projet académique, que j'ai poursuivi pour apporter les dernières finitions, notamment la possibilité de rejouer des parties à l'infini.

Profitez de votre partie d'échecs en réseau !!!
