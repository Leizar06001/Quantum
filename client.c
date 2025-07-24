#include "includes.h"

void *chat_client_thread(void *arg) {
	Game *game = (Game *)arg;
	int sockfd;
    struct sockaddr_in serv_addr;

    char buffer[BUFFER_SIZE];
	char buf_len = 0;

	// mvprintw(40, 0, "Connecting to %s", game->server_ip);

    // 1. Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) { 
		perror("socket"); 
		strcpy(game->exit_error, "Failed to create socket.");
		game->print_error = 1; // Set print error flag
		return NULL; 
	}

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, game->server_ip, &serv_addr.sin_addr) <= 0) {
        perror("inet_pton"); 
		close(sockfd);
		strcpy(game->exit_error, "Invalid server IP address.");
		game->print_error = 1; // Set print error flag
		return NULL;
    }

    // 2. Connect to server
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect"); 
		close(sockfd); 
		strcpy(game->exit_error, "Failed to connect to server.");
		game->print_error = 1; // Set print error flag
		return NULL;
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

	if (send(sockfd, buffer, buf_len, 0) <= 0){
		close(sockfd); 
		strcpy(game->exit_error, "Failed to send to server.");
		game->print_error = 1; // Set print error flag
		return NULL;
	}

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
				strcpy(game->exit_error, "Disconnected from server.");
				game->print_error = 1; // Set print error flag
                break;
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

int init_player(Player *player) {
	if (!player) return -1; // Vérification de la validité du pointeur
	// Get a random position on the map, ensuring it's not a wall
	srand(time(NULL)); // Initialiser le générateur de nombres aléatoires
	do {
		player->x = rand() % (map_w - 2) + 1; // Générer une position aléatoire
		player->y = rand() % (map_h - 2) + 1; // Générer une position aléatoire
	} while (map[player->y * map_w + player->x] != map_c_empty); // Assurer que ce n'est pas un mur
	player->color = rand() % (MAX_COLOR - MIN_COLOR) + MIN_COLOR; // Choisir une couleur aléatoire entre 1 et MAX_COLOR

	return 0;
}

void print_header(Game *game){
	mvprintw(0, 0, "[ESC] Exit | [F1] Color | [F2] Action | [F3] Notifs: %s | [F4] Menu | [Arrows] Move | [Enter] Send Message", game->notif_enabled ? "ON " : "OFF");
	refresh();
}

int game_loop(Game *game) {
	if (!game) return -1; // Vérification de la validité du pointeur

	update_display(game);
	print_header(game);

	// Boucle de jeu principale
	int ret = 0;
	size_t len = 0;
	while (server_running) {
		ret = get_user_input(game); // Obtenir l'entrée de l'utilisateur

		switch (ret){
			case IN_KEY_EXIT:
				server_running = 0;
				break;

			case IN_KEY_MOVE:
				wprintw(game->display.info, "Pos: %d, %d\n", game->player.x, game->player.y);
				wrefresh(game->display.info);
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
				box(game->display.self_text, 0, 0); // Redessiner la bordure
				mvwprintw(game->display.self_text, 1, 2, "%s:", game->player.name);
				wrefresh(game->display.self_text); // Rafraîchir la fenêtre de texte
				move_cursor_back(game);
				break;

			case IN_KEY_COLOR:
			case IN_KEY_DOOR:
				ask_for_display_update(game);
				break;

			case IN_KEY_NOTIF:
				print_header(game);
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
	game->clients[MAX_CLIENTS - 1].x		= 20;
	game->clients[MAX_CLIENTS - 1].y		= 83;
	game->clients[MAX_CLIENTS - 1].face_id	= 32;
	game->clients[MAX_CLIENTS - 1].body_id	= 0;
	game->clients[MAX_CLIENTS - 1].legs_id	= 0;
}

int main_client(Game *game){
	initscr();              // Démarrer ncurses
    cbreak();               // Lecture caractère par caractère
    noecho();               // Ne pas afficher les touches
    keypad(stdscr, TRUE);   // Activer les touches spéciales
	start_color();          // Activer les couleurs
	nodelay(stdscr, TRUE);
	setlocale(LC_ALL, "");

	game->player.face_id = read_config_int("face", 0);
	game->player.body_id = read_config_int("body", 0);
	game->player.legs_id = read_config_int("legs", 0);

	init_pnjs(game);

	if (init_map(game, map) == -1) goto exit_point;

	init_pair(1, COLOR_CYAN, COLOR_BLACK); // Définir une paire de couleurs
	init_pair(2, COLOR_RED, COLOR_BLACK);  // Définir une autre paire de couleurs
	init_pair(3, COLOR_GREEN, COLOR_BLACK); // Définir une paire de couleurs pour le joueur
	init_pair(4, COLOR_YELLOW, COLOR_BLACK); // Définir une paire de couleurs pour le score
	init_pair(5, COLOR_MAGENTA, COLOR_BLACK); // Définir une paire de couleurs pour le niveau
	init_pair(6, COLOR_BLUE, COLOR_BLACK); // Définir une paire de couleurs pour les vies
	init_pair(7, COLOR_WHITE, COLOR_BLACK); // Définir une paire de couleurs pour les messages

	init_color(8, 600, 200, 200);	// dark red
	init_color(9, 90, 90, 90);		// dark gray
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

	int w,h;
	getmaxyx(stdscr, h, w);
	// printf("W: %d, H: %d\n", w, h);
	// usleep(5000000);

	int main_y, main_x, main_h, main_w;
	int chat_y, chat_x, chat_h, chat_w;
	int txt_y, txt_x, txt_h, txt_w;

	if (h > 30 && w > 80){	// normal screen
		main_y = 3; main_x = 2;
		main_h = 20;
		main_w = (w - 5) / 2;

		chat_y = main_y;
		chat_x = w / 2;
		chat_h = main_h;
		chat_w = main_w;

		txt_y = main_y + main_h;
		txt_x = main_x;
		txt_h = 4;
		txt_w = main_w + chat_w + 1;
	} else {
		main_y = 4; main_x = 0;
		main_h = 20;
		main_w = w - 30 - 1;

		chat_y = main_y;
		chat_x = main_x + main_w + 1;
		chat_h = main_h;
		chat_w = w - main_w - 1;

		txt_y = main_y + main_h;
		txt_x = main_x;
		txt_h = 4;
		txt_w = main_w + chat_w + 1;
	}

    // int main_h = 20, 	main_w = 50;

	// int chat_y = main_y;
	// int chat_x = main_x + main_w + 1; // Position de la fenêtre de chat
	// int chat_h = main_h;
	// int chat_w = 50; // Largeur de la fenêtre de chat
	
	// int txt_y = main_y + main_h;
	// int txt_x = main_x;
	// int txt_h = 4, txt_w = main_w + chat_w + 1;

	game->display.height 	= main_h;
	game->display.width 	= main_w;

    // Créer une fenêtre
    game->display.main_win = newwin(main_h, main_w, main_y, main_x);
    box(game->display.main_win, 0, 0);             // Dessiner une bordure

	game->display.self_text = newwin(txt_h, txt_w, txt_y, txt_x); // Fenêtre pour les messages
	box(game->display.self_text, 0, 0);
	mvwprintw(game->display.self_text, 1, 2, "%s:", game->player.name);

	game->display.chat_box = newwin(chat_h, chat_w, chat_y, chat_x); 
	box(game->display.chat_box, 0, 0);
	game->display.chat = derwin(game->display.chat_box, chat_h - 2, chat_w - 2, 1, 1); // Fenêtre pour le chat
	scrollok(game->display.chat, TRUE); // Permettre le défilement dans la fenêtre de chat

	game->display.info = newwin(10, main_w + chat_w, txt_y + txt_h + 1, main_x); // Fenêtre pour les informations
	scrollok(game->display.info, TRUE);
	wattron(game->display.info, COLOR_PAIR(9));
    
	refresh();                 // Rafraîchir l'écran principal
    wrefresh(game->display.main_win);              // Afficher la fenêtre
	wrefresh(game->display.self_text);             // Afficher la fenêtre de texte
	wrefresh(game->display.chat_box);
	wrefresh(game->display.chat);                 // Afficher la fenêtre de chat
	wrefresh(game->display.info);                 // Afficher la fenêtre d'informations

	init_player(&game->player);

	wprintw(game->display.info, "You: Name: %s, Color %d, x %d, y %d\n", game->player.name, game->player.color, game->player.x, game->player.y);

	pthread_t com_tid;

	mvprintw(0, 0, "Connecting to chat server at %s...", game->server_ip);
	refresh(); // Rafraîchir l'écran principal
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