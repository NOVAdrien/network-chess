# Compilateur
CC = gcc

# Options de compilation
CFLAGS = -Wall

# Bibliothèques SDL2 et SDL2_image
SDL2_LIBS = -lSDL2 -lSDL2_image

# Cible par défaut
all : client server

# Règle pour compiler le client
client : ClientTCP.c ClientUDP.c
	$(CC) $(CFLAGS) -o clienttcp ClientTCP.c $(SDL2_LIBS)
	$(CC) $(CFLAGS) -o clientudp ClientUDP.c $(SDL2_LIBS)

# Règle pour compiler le serveur
server : ServerTCP.c ServerUDP.c
	$(CC) $(CFLAGS) -o servertcp ServerTCP.c
	$(CC) $(CFLAGS) -o serverudp ServerUDP.c

# Nettoyage des fichiers générés
clean :
	rm -f client server

# Phony targets pour éviter les conflits avec des fichiers du même nom
.PHONY : all clean
