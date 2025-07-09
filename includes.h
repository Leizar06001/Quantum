#ifndef INCLUDES_H
#define INCLUDES_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <ncurses.h>
#include <time.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <stdatomic.h>
#include <pthread.h>

extern atomic_int server_running;

#define PORT 		18467
#define MAX_CLIENTS 10
#define BUFFER_SIZE 512

typedef struct {
	int 	x, y; // Position du joueur
	int 	color;
	char 	name[32]; // Nom du joueur
	int		connected;
	int		lastx, lasty;
} Player;

typedef struct s_messages {
	char 	name[32]; 			// Nom du joueur
	int 	color; 			// Couleur du joueur (0-5)
	char 	text_buffer[256]; 	// Tampon pour le texte
	struct s_messages *next; 		// Pointeur vers le message suivant
} Messages;

typedef struct {
	char 		text_buffer[256]; // Tampon pour les messages
	size_t 		text_size; // Taille du tampon de texte
	char		text_to_send[256]; // Tampon pour le texte à envoyer
	uint8_t 	ready_to_send;
	uint8_t		new_pos;
	Messages 	*messages; // Liste des messages
	pthread_mutex_t m_send_text;
	pthread_mutex_t m_chat_box;
} Chat;

typedef struct {
	WINDOW *main_win; // Fenêtre ncurses pour l'affichage
	WINDOW *self_text;
	WINDOW *chat; // Fenêtre pour le chat
	WINDOW *chat_box;
	WINDOW *info;
	size_t height;  // Hauteur de l'affichage
	size_t width;   // Largeur de l'affichage
	int 			need_main_update;
	pthread_mutex_t m_display_update;
} Display;

typedef struct {
	char 	*map; // Carte du jeu
	size_t 	w; // Largeur de la carte
	size_t 	h; // Hauteur de la carte
	char map_c_empty; // Caractère représentant un espace vide
} Map;

typedef struct {
	int *selffd;
	int *client_fds;
} ComThread;

typedef struct {
	Display display; // Affichage du jeu
	Player 	player; // Le joueur
	Player	clients[100];
	Map 	map;     // Carte du jeu
	Chat 	chat;
	ComThread com; // Thread de communication

	int server_mode;
	char server_ip[16]; // Adresse IP du serveur

	char exit_error[256];
	int print_error;
} Game;

int get_user_input(Game *game);
int update_display(Game *game);

void *chat_server_thread(void *arg);
void *chat_client_thread(void *arg);

void add_message(Game *game, const char *name, const int color, const char *text);
void clear_messages(Game *game);
void print_messages(Game *game);

#endif