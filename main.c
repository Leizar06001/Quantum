#include "includes.h"

atomic_int server_running = 1; // Variable atomique pour contrôler l'état du serveur

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

	game->map.map = NULL;

	return 0;
}

int main(int argc, char **argv){
	Game game = {0};
	init_struct(&game);

	int server_mode = 0;
	int erreur = 0;

	if (argc == 2){
		if (strcmp("-server", argv[1]) == 0){
			server_mode = 1;
		} else {
			erreur = 1;
		}
	} else if (argc == 3) {
		// Player name + ip
		strncpy(game.player.name, argv[1], sizeof(game.player.name) - 1);
		game.player.name[sizeof(game.player.name) - 1] = '\0';

		char tmp[16];
		if (inet_pton(AF_INET, argv[2], &tmp) <= 0) {
			fprintf(stderr, "Invalid server IP address: %s\n", argv[2]);
			return 1;
		}
		strncpy(game.server_ip, argv[2], sizeof(game.server_ip) - 1);
		game.server_ip[sizeof(game.server_ip) - 1] = '\0'; //
	} else {
		erreur = 1;
		// server_mode = 1;
	}

	if (erreur){
		fprintf(stderr, "Usage: %s <player_name> <server_ip> or %s -server\n", argv[0], argv[0]);
		return 1;
	}


	if (server_mode){
		main_server(&game);
	} else {
		main_client(&game);
	}


    return 0;
}