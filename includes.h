#ifndef INCLUDES_H
#define INCLUDES_H

#define _XOPEN_SOURCE_EXTENDED 1

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <locale.h>
#include <ncurses.h>
#include <time.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <stdatomic.h>
#include <pthread.h>
#include <signal.h>
#include "terminal_esc_seq.h"
#include "config.h"
#include <wchar.h>
#include "globals.h"
#include "file.h"

extern atomic_int server_running;

#define PORT 		18467
#define MAX_CLIENTS 50
#define BUFFER_SIZE 512

#define DURATION_MSG_NOTIF	1200 // ms

#define IN_KEY_EXIT -1
#define IN_KEY_UNKN	0
#define IN_KEY_MOVE	1
#define IN_KEY_TEXT	2
#define IN_KEY_SEND 3
#define IN_KEY_COLOR 4
#define IN_KEY_DOOR	5
#define IN_KEY_NOTIF 6
#define IN_KEY_PERSO 7

#define B_NEW_CLIENT 	0x0F
#define B_CLIENT_INFOS	0x1F

#define B_MESSAGE 		0xE0
#define B_POS 			0xE1
#define B_COLOR			0xE2
#define B_SERVER_MSG 	0xE3
#define B_DOOR_CHANGE	0xE4
#define B_PERSO_CHANGE	0xE5

#define L_CLIENT_INFO	20
#define L_CLIENT_POS	5
#define L_CLIENT_COLOR	4
#define L_DOOR_CHANGE	4
#define L_PERSO_CHANGE	6

#define SEND_TO_ALL		0
#define DONT_SEND_ORIG	1

#define MIN_COLOR 10
#define MAX_COLOR 19

#define DIST_NOISE		15	// When messages start to cramble
#define DIST_NO_HEAR	25	// When you dont hear anything

#define MAX_DOORS	20

#define PAIR_CYAN 		1
#define PAIR_RED		2
#define PAIR_GREEN		3
#define PAIR_YELLOW		4
#define PAIR_MAGENTA 	5
#define PAIR_BLUE		6
#define PAIR_WHITE		7


typedef struct {
	int 	x, y; // Position du joueur
	int 	color;
	int		face_id;
	int		body_id;
	int		legs_id;
	char 	name[32]; // Nom du joueur
	int		connected;
	int		lastx, lasty;
	uint64_t last_msg;
} Player;

typedef struct s_messages {
	char 	name[32]; 			// Nom du joueur
	int 	color; 			// Couleur du joueur (0-5)
	char 	text_buffer[256]; 	// Tampon pour le texte
	int		player_id;
	struct s_messages *next; 		// Pointeur vers le message suivant
} Messages;

typedef struct {
	char 		text_buffer[256]; // Tampon pour les messages
	size_t 		text_size; // Taille du tampon de texte
	char		text_to_send[256]; // Tampon pour le texte à envoyer
	uint8_t 	ready_to_send;
	uint8_t		new_pos;
	uint8_t		new_color;
	uint8_t		new_door;
	uint8_t		new_perso;
	int			door_change_id;
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
	size_t	term_h;
	size_t	term_w;
	int 			need_main_update;
	pthread_mutex_t m_display_update;
	uint64_t	t_next_update;
	int chat_x, chat_y, chat_w, chat_h;
} Display;

typedef struct {
	int x;
	int y;
	int state;
	int vertical;
	int enabled;
} Door;

typedef struct {
	char 	*map; // Carte du jeu
	size_t	map_len;
	size_t 	w; // Largeur de la carte
	size_t 	h; // Hauteur de la carte
	char map_c_empty; // Caractère représentant un espace vide
	Door doors[MAX_DOORS];
	int nb_doors;
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
	uint16_t server_port;

	char exit_error[256];
	int print_error;
	int notif_enabled;
	int connected_to_server;
} Game;

#include "map.h"

int start_test(Game *game);

int main_server(Game *game);
int main_client(Game *game);

int get_user_input(Game *game);
int update_display(Game *game);
int ask_for_display_update(Game *game);

// void *chat_server_thread(void *arg);
void *chat_client_thread(void *arg);

void add_message(Game *game, const char *name, const int color, const char *text, int player_id);
void clear_messages(Game *game);
int print_messages(Game *game);

void move_cursor_back(Game *game);

int recv_msg(Game *game, const char *msg, int len);
int send_msg_all(int *fds, const char *msg, int len, int fd_orig, uint8_t dont_send_to_orig);

uint64_t millis();
double distance(int x1, int y1, int x2, int y2);
int shortest_distance(const char *map, int map_w, int map_h,
                      int x_start, int y_start, int x_end, int y_end);
int min(int a, int b);
void pinfo(Game *game, const char *fmt, ...);
void send_notification(const char *title, const char *message);

#endif