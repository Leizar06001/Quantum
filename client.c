#include "includes.h"

void del_windows(Game *game);
void create_windows(Game *game);
void draw_windows(Game *game);


static char buffer[BUFFER_SIZE];
static char buf_len = 0;

int connect_to_server(Game *game, int *sockfd){
	pinfo(game, "Connecting to server at %s...\n", game->server_ip);

	struct sockaddr_in serv_addr;

    // 1. Create socket
    *sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (*sockfd < 0) { 
		perror("socket"); 
		strcpy(game->exit_error, "Failed to create socket.");
		game->print_error = 1; // Set print error flag
		return -2; 
	}

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, game->server_ip, &serv_addr.sin_addr) <= 0) {
        perror("inet_pton"); 
		close(*sockfd);
		strcpy(game->exit_error, "Invalid server IP address");
		game->print_error = 1; // Set print error flag
		return -2;
    }

    // 2. Connect to server
    if (connect(*sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
		close(*sockfd); 
		pinfo(game, "Failed to connect to server: %s\n", strerror(errno));
		return -1;
    }

	game->clients[0].connected = 0;
	game->clients[0].color = 2;
	strncpy(game->clients[0].name, "Server", sizeof(game->clients[0].name) - 1);

	buf_len = strlen(game->player.name) + 10;
	buffer[0] = buf_len;
	buffer[1] = (char)B_NEW_CLIENT;
	buffer[2] = 0;	// id
	buffer[3] = 0;	// connected
	buffer[4] = game->player.color;
	buffer[5] = game->player.x;
	buffer[6] = game->player.y;
	buffer[7] = game->player.face_id;
	buffer[8] = game->player.body_id;
	buffer[9] = game->player.legs_id;

	strncpy(buffer + 10, game->player.name, sizeof(buffer) - 10);

	if (send(*sockfd, buffer, buf_len, 0) <= 0){
		close(*sockfd); 
		// strcpy(game->exit_error, "Failed to send to server.");
		// game->print_error = 1; // Set print error flag
		pinfo(game, "Failed to send to server");
		return -1;
	}

	pinfo(game, "Connection success !\n");

	return 1;
}

void *chat_client_thread(void *arg) {
	Game *game = (Game *)arg;
	int sockfd;

	for(int i = 0; i < 10; i++){
		int conn = connect_to_server(game, &sockfd);
		if (conn == 1){
			game->connected_to_server = 1;
			break;
		} else if (conn == -2){
			return NULL;
		}
		pinfo(game, "Connection failed, retry %d/%d\n", i + 1, 10);
		usleep(3000000);
	}
	if (game->connected_to_server == 0) return NULL;

    // 3. Main loop with select()
    while (server_running) {
        fd_set readfds;
        FD_ZERO(&readfds);
        // FD_SET(0, &readfds);        // stdin
        FD_SET(sockfd, &readfds);   // server socket
        int maxfd = sockfd;

		struct timeval timeout = {0, 1000};  // 0 seconds, 0 microseconds

        int activity = select(maxfd + 1, &readfds, NULL, NULL, &timeout);
        if (activity < 0 && errno != EINTR) {
            perror("select");
            break;
        }

		// Get mutex lock for sending text
		pthread_mutex_lock(&game->chat.m_send_text);

		// NEW TEXT TO SEND
		if (game->chat.ready_to_send) {
			buf_len = strlen(game->chat.text_to_send) + 3;
			sprintf(buffer, "%c%c%c%s", buf_len, (char)B_MESSAGE, 0, game->chat.text_to_send);

			if (send(sockfd, buffer, buf_len, 0) < 0) {
				perror("send");
				break;
			}
			game->chat.text_to_send[0] = '\0'; // Clear the text to send
			game->chat.ready_to_send = 0; // Reset the flag
			game->chat.text_size = 0;
		}

		// NEW POS TO SEND
		if (game->chat.new_pos) {
			buf_len = L_CLIENT_POS;
			sprintf(buffer, "%c%c%c%c%c", buf_len, (char)B_POS, 0, (char)game->player.lastx, (char)game->player.lasty);
			if (send(sockfd, buffer, buf_len, 0) < 0) { perror("send"); break; }
			game->chat.new_pos = 0; // Reset the new position flag
		}

		if (game->chat.new_color){
			buf_len = L_CLIENT_COLOR;
			sprintf(buffer, "%c%c%c%c", buf_len, (char)B_COLOR, 0, (char)game->player.color);
			if (send(sockfd, buffer, buf_len, 0) < 0) { perror("send"); break; }
			game->chat.new_color = 0;
		}

		if (game->chat.new_door){
			buf_len = L_DOOR_CHANGE;
			char door_state = game->map.doors[game->chat.door_change_id].state;
			sprintf(buffer, "%c%c%c%c", buf_len, (char)B_DOOR_CHANGE, game->chat.door_change_id, door_state);
			if (send(sockfd, buffer, buf_len, 0) < 0) { perror("send"); break; }
			game->chat.new_door = 0;
		}

		if (game->chat.new_perso){
			buf_len = L_PERSO_CHANGE;
			sprintf(buffer, "%c%c%c%c%c%c", buf_len, (char)B_PERSO_CHANGE, 0, (char)game->player.face_id, (char)game->player.body_id, (char)game->player.legs_id);
			if (send(sockfd, buffer, buf_len, 0) < 0) { perror("send"); break; }
			game->chat.new_perso = 0;
		}

		pthread_mutex_unlock(&game->chat.m_send_text); // Release the mutex if no text to send

        // 5. Read from server
        if (FD_ISSET(sockfd, &readfds)) {
            int len = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
            if (len <= 0) {
				// strcpy(game->exit_error, "Disconnected from server.");
				// game->print_error = 1; // Set print error flag
                // break;
				game->connected_to_server = 0;
				pinfo(game, "Disconnected from server, trying to reconnect...\n");
				for(int i = 0; i < 100; i++){
					int conn = connect_to_server(game, &sockfd);
					if (conn == 1){
						game->connected_to_server = 1;
						break;
					} else if (conn == -2){
						return NULL;
					}
					pinfo(game, "Connection failed, retry %d/100\n", i + 1);
					usleep(3000000);
				}
				if (game->connected_to_server == 0) return NULL;
            }
            buffer[len] = '\0';

			recv_msg(game, buffer, len); // Process received message
        }
    }
	server_running = 0;

	mvprintw(42, 0, "EXIT COM THREAD"); // Print sent message
	refresh(); // Refresh to show the message

    close(sockfd);
    return NULL;
}



void print_header(Game *game){
	mvprintw(0, 0, "[ESC] Exit | [F1] Action | [F2] Menu Perso | [F3] Menu Chat | [F4] Notifs: %s | [Arrows] Move", game->notif_enabled ? "ON " : "OFF");
	refresh();
}

void check_terminal_resize(Game *game){
	int w,h;
	getmaxyx(stdscr, h, w);

	if (w != game->display.term_w || h != game->display.term_h){
		pthread_mutex_lock(&game->display.m_display_update);
		pthread_mutex_lock(&game->chat.m_chat_box);

		del_windows(game);
		clear();
		create_windows(game);
		draw_windows(game);
		print_header(game);
		refresh();

		pthread_mutex_unlock(&game->chat.m_chat_box);
		pthread_mutex_unlock(&game->display.m_display_update);
	}
}

int game_loop(Game *game) {
	if (!game) return -1; // Vérification de la validité du pointeur

	pthread_mutex_lock(&game->display.m_display_update);
	update_display(game);
	print_header(game);
	pthread_mutex_unlock(&game->display.m_display_update);

	// Boucle de jeu principale
	int ret = 0;
	size_t len = 0;
	while (server_running) {
		
		check_terminal_resize(game);

		ret = get_user_input(game); // Obtenir l'entrée de l'utilisateur

		switch (ret){
			case IN_KEY_EXIT:
				server_running = 0;
				break;

			case IN_KEY_MOVE:
				pinfo(game, "Pos: %d, %d\n", game->player.x, game->player.y);
				ask_for_display_update(game);
				break;

			case IN_KEY_TEXT:
				// Afficher le texte dans la fenêtre de texte
				len = strlen(game->chat.text_buffer);
				if (len > 0){
					mvwprintw(game->display.self_text, 2, len + 1, "  "); // Afficher le texte
					mvwprintw(game->display.self_text, 2, len + 1, "%c", game->chat.text_buffer[len - 1]); // Afficher le texte
				} else {
					mvwprintw(game->display.self_text, 2, 2, " "); // Effacer la ligne si le tampon est vide
				}
				wrefresh(game->display.self_text); // Rafraîchir la fenêtre de texte
				move_cursor_back(game);
				break;

			case IN_KEY_SEND:
				// Effacer la ligne de texte
				wclear(game->display.self_text);
				wattron(game->display.self_text, COLOR_PAIR(24));
				box(game->display.self_text, 0, 0); // Redessiner la bordure
				wattroff(game->display.self_text, COLOR_PAIR(24));
				mvwprintw(game->display.self_text, 1, 2, "%s:", game->player.name);
				wrefresh(game->display.self_text); // Rafraîchir la fenêtre de texte
				move_cursor_back(game);
				break;

			case IN_KEY_COLOR:
			case IN_KEY_DOOR:
				ask_for_display_update(game);
				break;

			case IN_KEY_NOTIF:
				pthread_mutex_lock(&game->display.m_display_update);
				print_header(game);
				pthread_mutex_unlock(&game->display.m_display_update);
				break;
		}

		if (print_messages(game)){ // Afficher les messages dans la fenêtre de chat
			move_cursor_back(game);
		}

		pthread_mutex_lock(&game->display.m_display_update);
		if (game->display.need_main_update || millis() > game->display.t_next_update) {
			update_display(game); // Mettre à jour l'affichage principal

			game->display.t_next_update = millis() + 500;
			game->display.need_main_update = 0; // Réinitialiser le besoin de mise à jour
		}
		pthread_mutex_unlock(&game->display.m_display_update);

		// mvprintw(1, 0, "Position du joueur: (%d, %d) - C: %d", game->player.x, game->player.y, game->player.color);
		usleep(10000); // Pause pour éviter une boucle trop rapide
	}

	return 0;
}

void init_pnjs(Game *game){
	game->clients[MAX_CLIENTS - 1].connected = 2;
	strcpy(game->clients[MAX_CLIENTS - 1].name, "BOSS");
	game->clients[MAX_CLIENTS - 1].color 	= 18;
	game->clients[MAX_CLIENTS - 1].x		= 21;
	game->clients[MAX_CLIENTS - 1].y		= 83;
	game->clients[MAX_CLIENTS - 1].face_id	= 32;
	game->clients[MAX_CLIENTS - 1].body_id	= 0;
	game->clients[MAX_CLIENTS - 1].legs_id	= 0;
}

int init_player(Player *player) {
	if (!player) return -1; // Vérification de la validité du pointeur
	// Get a random position on the map, ensuring it's not a wall
	srand(time(NULL)); // Initialiser le générateur de nombres aléatoires

	int xmin = 1;
	int xmax = map_w - 2;
	int xrange = (xmax - xmin) / 2 + 1;
	int ymin = 75, ymax = 80;

	do {
		player->x = xmin + 2 * (rand() % xrange); 
		player->y = rand() % (ymax - ymin + 1) + ymin;
	} while (map[player->y * map_w + player->x] != map_c_empty); // Assurer que ce n'est pas un mur

	player->color = rand() % (MAX_COLOR - MIN_COLOR) + MIN_COLOR; // Choisir une couleur aléatoire entre 1 et MAX_COLOR

	player->face_id = read_config_int("face", 0);
	player->body_id = read_config_int("body", 0);
	player->legs_id = read_config_int("legs", 0);

	return 0;
}

void del_windows(Game *game){
	delwin(game->display.main_win);
	delwin(game->display.self_text);
	delwin(game->display.chat_box);
	delwin(game->display.info);
}

void create_windows(Game *game){
	int w,h;
	getmaxyx(stdscr, h, w);
	(void)h;

	game->display.term_h = h;
	game->display.term_w = w;

	int main_y, main_x, main_h, main_w;
	int txt_y, txt_x, txt_h, txt_w;

	main_y = 3; main_x = 2;
	main_h = 20;
	main_w = min(50, (w - 5) / 2);

	game->display.chat_y = main_y;
	game->display.chat_x = main_w + main_x + 1;
	game->display.chat_h = main_h;
	game->display.chat_w = w - main_w - main_x - 1;

	txt_y = main_y + main_h;
	txt_x = main_x;
	txt_h = 4;
	txt_w = main_w + game->display.chat_w + 1;

	game->display.height 	= main_h;
	game->display.width 	= main_w;

    // Créer une fenêtre
    game->display.main_win 	= newwin(main_h, main_w, main_y, main_x);
	game->display.self_text = newwin(txt_h, txt_w, txt_y, txt_x); // Fenêtre pour les messages
	game->display.chat_box 	= newwin(game->display.chat_h, game->display.chat_w, game->display.chat_y, game->display.chat_x); 
	game->display.chat 		= derwin(game->display.chat_box, game->display.chat_h - 2, game->display.chat_w - 2, 1, 1);
	scrollok(game->display.chat, TRUE);
	game->display.info 		= newwin(4, main_w + game->display.chat_w, txt_y + txt_h + 1, main_x); // Fenêtre pour les informations
	scrollok(game->display.info, TRUE);
	wattron(game->display.info, COLOR_PAIR(9));
}

void draw_windows(Game *game){
	const int box_color = 24;

	wattron(game->display.main_win, COLOR_PAIR(box_color));
    box(game->display.main_win, 0, 0);
	wattroff(game->display.main_win, COLOR_PAIR(box_color));

	wattron(game->display.self_text, COLOR_PAIR(box_color));
	box(game->display.self_text, 0, 0);
	wattroff(game->display.self_text, COLOR_PAIR(box_color));
	mvwprintw(game->display.self_text, 1, 2, "%s:", game->player.name);

	wattron(game->display.chat_box, COLOR_PAIR(box_color));
	box(game->display.chat_box, 0, 0);
	wattroff(game->display.chat_box, COLOR_PAIR(box_color));

	refresh();                 // Rafraîchir l'écran principal
    wrefresh(game->display.main_win);              // Afficher la fenêtre
	wrefresh(game->display.self_text);             // Afficher la fenêtre de texte
	wrefresh(game->display.chat_box);
	wrefresh(game->display.chat);                 // Afficher la fenêtre de chat
	wrefresh(game->display.info);                 // Afficher la fenêtre d'informations
}

int main_client(Game *game){
	initscr();              // Démarrer ncurses
    cbreak();               // Lecture caractère par caractère
    noecho();               // Ne pas afficher les touches
    keypad(stdscr, TRUE);   // Activer les touches spéciales
	start_color();          // Activer les couleurs
	nodelay(stdscr, TRUE);
	setlocale(LC_ALL, "");

	if (init_map(game, map) == -1) goto exit_point;

	init_pair(1, COLOR_CYAN, COLOR_BLACK); // Définir une paire de couleurs
	init_pair(2, COLOR_RED, COLOR_BLACK);  // Définir une autre paire de couleurs
	init_pair(3, COLOR_GREEN, COLOR_BLACK); // Définir une paire de couleurs pour le joueur
	init_pair(4, COLOR_YELLOW, COLOR_BLACK); // Définir une paire de couleurs pour le score
	init_pair(5, COLOR_MAGENTA, COLOR_BLACK); // Définir une paire de couleurs pour le niveau
	init_pair(6, COLOR_BLUE, COLOR_BLACK); // Définir une paire de couleurs pour les vies
	init_pair(7, COLOR_WHITE, COLOR_BLACK); // Définir une paire de couleurs pour les messages

	init_color(8, 600, 200, 200);	// dark red
	init_color(9, 70, 70, 70);		// dark gray
	init_pair(8, 8, 9);				// Map

	init_color(20, 200, 200, 200);		// dark gray
	init_pair(9, 20, COLOR_BLACK);		// dark text

	short pale_colors[10] = {10, 11, 12, 13, 14, 15, 16, 17, 18, 19};
    init_color(10, 300, 300, 600);  // steel blue
    init_color(11, 600, 600, 200);  // dull mustard
    init_color(12, 450, 300, 600);  // dusty purple
    init_color(13, 300, 600, 600);  // dark aqua
    init_color(14, 600, 500, 250);  // clay brown
    init_color(15, 500, 500, 600);  // shadow lilac
    init_color(16, 600, 400, 400);  // rust pink
    init_color(17, 500, 600, 350);  // dull lime
    init_color(18,  600, 300, 300);  // brick rose
    init_color(19,  300, 600, 300);  // murky green
	for (short i = 0; i < 10; i++) {
        init_pair(pale_colors[i], pale_colors[i], COLOR_BLACK);
    }

	init_pair(20, 17, 9);			// Opened door
	init_pair(21, COLOR_RED, 9);	// Closed door

	init_color(22, 300, 100, 100);	// darker red, wall shades
	init_pair(22, 22, 9);	// dark walls

	init_color(23, 90*1000/255, 63*1000/255, 40*1000/255);  // Dark brown
	init_pair(23, 23, 9);	// mobilier

	init_color(24, 400, 400, 400);		// dark light gray
	init_pair(24, 24, COLOR_BLACK);		// dark light borders


	// WINDOWS 
	create_windows(game);
	pthread_mutex_lock(&game->display.m_display_update);
	draw_windows(game);
	pthread_mutex_unlock(&game->display.m_display_update);
	
	// CHARACTERS
	init_pnjs(game);
	init_player(&game->player);

	pinfo(game, "You: Name: %s, Color %d, x %d, y %d\n", game->player.name, game->player.color, game->player.x, game->player.y);

	pthread_t com_tid;
	if (pthread_create(&com_tid, NULL, chat_client_thread, game) != 0) {
		perror("pthread_create");
		return 1;
	}

	game_loop(game); // Lancer la boucle de jeu

exit_point:

	server_running = 0; // Arrêter le serveur de chat
	pthread_cancel(com_tid); // Annuler le thread du serveur de chat
	pthread_join(com_tid, NULL); // Attendre la fin du thread du serveur de chat
    delwin(game->display.main_win);                // Supprimer la fenêtre
    clear();             // Clear all ncurses windows
	refresh();           // Force draw to terminal
	endwin();            // Restore terminal state
	fflush(stdout);

	// Libérer les ressources
	clear_messages(game); // Libérer les messages du chat
	if (game->map.map) free(game->map.map);

	if (game->print_error) {
		printf("\nError: %s\n", game->exit_error);
		return 1; // Quitter avec une erreur
	}

	return 0;
}