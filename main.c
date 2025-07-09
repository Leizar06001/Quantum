#include "includes.h"
#include "map.h"

atomic_int server_running = 1; // Variable atomique pour contrôler l'état du serveur

int init_player(Player *player) {
	if (!player) return -1; // Vérification de la validité du pointeur
	// Get a random position on the map, ensuring it's not a wall
	srand(time(NULL)); // Initialiser le générateur de nombres aléatoires
	do {
		player->x = rand() % (map_w - 2) + 1; // Générer une position aléatoire
		player->y = rand() % (map_h - 2) + 1; // Générer une position aléatoire
	} while (map[player->y * map_w + player->x] != map_c_empty); // Assurer que ce n'est pas un mur
	player->color = rand() % 6; // Choisir une couleur aléatoire entre 1 et 7
	return 0;
}

int game_loop(Game *game) {
	if (!game) return -1; // Vérification de la validité du pointeur

	update_display(game);

	mvprintw(0, 0, "[ESC] Exit | [F1] Change Color | [Arrows] Move | [Backspace] Delete Last Char | [Enter] Send Message");
	refresh();

	// Boucle de jeu principale
	int ret = 0;
	while (server_running) {
		ret = get_user_input(game); // Obtenir l'entrée de l'utilisateur

		print_messages(game); // Afficher les messages dans la fenêtre de chat

		pthread_mutex_lock(&game->display.m_display_update);
		if (game->display.need_main_update) {
			update_display(game); // Mettre à jour l'affichage principal
			game->display.need_main_update = 0; // Réinitialiser le besoin de mise à jour
		}
		pthread_mutex_unlock(&game->display.m_display_update);

		if (ret == -1){
			server_running = 0; // Quitter la boucle de jeu
		} else if (ret == 1){
			update_display(game); // Mettre à jour l'affichage
		} else if (ret == 2) {
			// Afficher le texte dans la fenêtre de texte
			size_t len = strlen(game->chat.text_buffer);
			if (len > 0){
				mvwprintw(game->display.self_text, 1, len + 1, "  "); // Afficher le texte
				mvwprintw(game->display.self_text, 1, len + 1, "%c", game->chat.text_buffer[len - 1]); // Afficher le texte
			} else {
				mvwprintw(game->display.self_text, 1, 2, " "); // Effacer la ligne si le tampon est vide
			}
			wrefresh(game->display.self_text); // Rafraîchir la fenêtre de texte
		} else if (ret == 3) {
			// Effacer la ligne de texte
			wclear(game->display.self_text);
			box(game->display.self_text, 0, 0); // Redessiner la bordure
			wrefresh(game->display.self_text); // Rafraîchir la fenêtre de texte
		}
		// mvprintw(1, 0, "Position du joueur: (%d, %d) - C: %d", game->player.x, game->player.y, game->player.color);
		usleep(10000); // Pause pour éviter une boucle trop rapide
	}

	return 0;
}

int init_struct(Game *game){
	game->chat.m_send_text = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
	game->chat.m_chat_box = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
	game->chat.ready_to_send = 0;
	game->chat.text_size = 0;
	game->chat.text_buffer[0] = '\0'; // Initialiser le tampon de texte

	game->display.m_display_update = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
	game->display.need_main_update = 0;

	game->print_error = 0;
	game->exit_error[0] = '\0'; // Initialiser le tampon d'erreur

	game->server_mode = 0;

	return 0;
}

int main(int argc, char **argv){
	Game game = {0};
	init_struct(&game); // Initialiser la structure du jeu

	if (argc == 2){
	} else if (argc == 3){
		if (argv[2][0] != '@'){
			fprintf(stderr, "Usage: %s <player_name>\n", argv[0]);
			return 1;
		} else {
			game.server_mode = 1;
		}
	} else {
		fprintf(stderr, "Usage: %s <player_name>\n", argv[0]);
		return 1;
	}
	// On suppose que le premier argument est le nom du joueur
	strncpy(game.player.name, argv[1], sizeof(game.player.name) - 1);
	game.player.name[sizeof(game.player.name) - 1] = '\0'; // Assurer la terminaison de la chaîne
	strncpy(game.server_ip, "10.0.50.117", sizeof(game.server_ip) - 1);

	// We need player name as arg
	// char tmp[16];
	// if (argc == 2) {
	// 	strncpy(game.player.name, argv[1], sizeof(game.player.name) - 1);
	// 	game.player.name[sizeof(game.player.name) - 1] = '\0'; // Assurer la terminaison de la chaîne
	// 	server_mode = 1; // Activer le mode serveur
	// } else if (argc == 3) {
	// 	// Arg 2 is server's ip for client mode
	// 	strncpy(game.player.name, argv[1], sizeof(game.player.name) - 1);
	// 	game.player.name[sizeof(game.player.name) - 1] = '\0'; //
	// 	// Copie de l'ip
	// 	if (inet_pton(AF_INET, argv[2], &tmp) <= 0) {
	// 		fprintf(stderr, "Invalid server IP address: %s\n", argv[2]);
	// 		return 1; // Quitter avec une erreur
	// 	}
	// 	strncpy(game.server_ip, argv[2], sizeof(game.server_ip) - 1);
	// 	game.server_ip[sizeof(game.server_ip) - 1] = '\0'; //
	// } else {
	// 	// erreur print help
	// 	strncpy(game.player.name, "Server", sizeof(game.player.name) - 1);
	// 	server_mode = 1;

	// 	// strncpy(game.player.name, "Client", sizeof(game.player.name) - 1);
	// 	// if (inet_pton(AF_INET, "127.0.0.1", &tmp) <= 0) {
	// 	// 	fprintf(stderr, "Invalid server IP address: %s\n", argv[2]);
	// 	// 	return 1; // Quitter avec une erreur
	// 	// }
	// 	// strncpy(game.server_ip, "127.0.0.1", sizeof(game.server_ip) - 1);
	// 	// game.server_ip[sizeof(game.server_ip) - 1] = '\0'; //
	// 	// fprintf(stderr, "Usage: %s <player_name>\n", argv[0]);
	// 	// return 1; // Quitter avec une erreur
	// }

	initscr();              // Démarrer ncurses
    cbreak();               // Lecture caractère par caractère
    noecho();               // Ne pas afficher les touches
    keypad(stdscr, TRUE);   // Activer les touches spéciales
	start_color();          // Activer les couleurs
	nodelay(stdscr, TRUE);

    int main_y = 5, 	main_x = 2;
    int main_h = 20, 	main_w = 50;

	int chat_y = main_y;
	int chat_x = main_x + main_w + 1; // Position de la fenêtre de chat
	int chat_h = main_h;
	int chat_w = 50; // Largeur de la fenêtre de chat
	
	int txt_y = main_y + main_h;
	int txt_x = chat_x;
	int txt_h = 4, txt_w = main_w;

	game.display.height = main_h;
	game.display.width 	= main_w;

	game.map.map = (char *)map; // Assigner la carte au jeu
	game.map.w = map_w; // Assigner la largeur de la carte
	game.map.h = map_h; // Assigner la hauteur de la carte
	game.map.map_c_empty = map_c_empty; // Assigner le caractère vide de la carte

	init_pair(1, COLOR_CYAN, COLOR_BLACK); // Définir une paire de couleurs
	init_pair(2, COLOR_RED, COLOR_BLACK);  // Définir une autre paire de couleurs
	init_pair(3, COLOR_GREEN, COLOR_BLACK); // Définir une paire de couleurs pour le joueur
	init_pair(4, COLOR_YELLOW, COLOR_BLACK); // Définir une paire de couleurs pour le score
	init_pair(5, COLOR_MAGENTA, COLOR_BLACK); // Définir une paire de couleurs pour le niveau
	init_pair(6, COLOR_BLUE, COLOR_BLACK); // Définir une paire de couleurs pour les vies
	init_pair(7, COLOR_WHITE, COLOR_BLACK); // Définir une paire de couleurs pour les messages

    // Créer une fenêtre
    game.display.main_win = newwin(main_h, main_w, main_y, main_x);
    box(game.display.main_win, 0, 0);             // Dessiner une bordure

	game.display.self_text = newwin(txt_h, txt_w, txt_y, txt_x); // Fenêtre pour les messages
	box(game.display.self_text, 0, 0);

	game.display.chat_box = newwin(chat_h, chat_w, chat_y, chat_x); 
	box(game.display.chat_box, 0, 0);
	game.display.chat = derwin(game.display.chat_box, chat_h - 2, chat_w - 2, 1, 1); // Fenêtre pour le chat
	scrollok(game.display.chat, TRUE); // Permettre le défilement dans la fenêtre de chat

	game.display.info = newwin(10, main_w + chat_w, txt_y + txt_h + 1, main_x); // Fenêtre pour les informations
	scrollok(game.display.info, TRUE);
    
	refresh();                 // Rafraîchir l'écran principal
    wrefresh(game.display.main_win);              // Afficher la fenêtre
	wrefresh(game.display.self_text);             // Afficher la fenêtre de texte
	wrefresh(game.display.chat_box);
	wrefresh(game.display.chat);                 // Afficher la fenêtre de chat
	wrefresh(game.display.info);                 // Afficher la fenêtre d'informations

	init_player(&game.player);

	pthread_t com_tid;
	if (game.server_mode) {
		mvprintw(0, 0, "Starting chat server...");
		refresh(); // Rafraîchir l'écran principal
		if (pthread_create(&com_tid, NULL, chat_server_thread, &game) != 0) {
			perror("pthread_create");
			return 1;
		}
	} else {
		mvprintw(0, 0, "Connecting to chat server at %s...", game.server_ip);
		refresh(); // Rafraîchir l'écran principal
		if (pthread_create(&com_tid, NULL, chat_client_thread, &game) != 0) {
			perror("pthread_create");
			return 1;
		}
	}

	game_loop(&game); // Lancer la boucle de jeu

	server_running = 0; // Arrêter le serveur de chat
	pthread_cancel(com_tid); // Annuler le thread du serveur de chat
	pthread_join(com_tid, NULL); // Attendre la fin du thread du serveur de chat
    delwin(game.display.main_win);                // Supprimer la fenêtre
    clear();             // Clear all ncurses windows
	refresh();           // Force draw to terminal
	endwin();            // Restore terminal state
	fflush(stdout);

	// Libérer les ressources
	clear_messages(&game); // Libérer les messages du chat

	if (game.print_error) {
		printf("Error: %s\n", game.exit_error);
		return 1; // Quitter avec une erreur
	}

    return 0;
}