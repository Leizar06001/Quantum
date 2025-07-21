#include "includes.h"

void add_message(Game *game, const char *name, const int color, const char *text, int player_id) {
	if (!game || !name || !text) return; // Vérification de la validité des pointeurs

	Messages *new_message = malloc(sizeof(Messages));
	if (!new_message) return; // Vérification de l'allocation mémoire

	int name_length = strlen(name);
	int text_length = strlen(text);
	strncpy(new_message->name, name, name_length);
	new_message->name[name_length] = '\0'; // Assurer la terminaison de la chaîne
	new_message->color = color;
	strncpy(new_message->text_buffer, text, text_length);
	new_message->text_buffer[text_length] = '\0'; // Assurer la terminaison de la chaîne
	new_message->next = NULL;
	new_message->player_id = player_id;

	pthread_mutex_lock(&game->chat.m_chat_box);
	
	if (game->chat.messages == NULL) {
		game->chat.messages = new_message; // Premier message
	} else {
		Messages *current = game->chat.messages;
		while (current->next != NULL) {
			current = current->next; // Trouver le dernier message
		}
		current->next = new_message; // Ajouter le nouveau message à la fin
	}

	pthread_mutex_unlock(&game->chat.m_chat_box);
}

void clear_messages(Game *game) {
	if (!game) return; // Vérification de la validité du pointeur

	pthread_mutex_lock(&game->chat.m_chat_box);
	Messages *current = game->chat.messages;
	while (current != NULL) {
		Messages *next = current->next;
		free(current); // Libérer la mémoire du message
		current = next; // Passer au message suivant
	}
	game->chat.messages = NULL; // Réinitialiser la liste des messages
	pthread_mutex_unlock(&game->chat.m_chat_box);
}

int print_messages(Game *game) {
	if (!game) return 0; // Vérification de la validité du pointeur

	int printed = 0;

	pthread_mutex_lock(&game->chat.m_chat_box);
	Messages *current = game->chat.messages;
	while (current != NULL) {

		// Check the distance to message origin
		// double dist = 0;
		// if (current->player_id > 0){
		// 	// This uses strait line
		// 	// dist = distance(game->player.x, game->player.y, game->clients[current->player_id].x, game->clients[current->player_id].y);
			
		// 	// This search for shortest dist avoiding walls
		// 	dist = shortest_distance(game->map.map, game->map.w, game->map.h, game->player.x, game->player.y, game->clients[current->player_id].x, game->clients[current->player_id].y);
		// }
		
		// wprintw(game->display.info, "Dist: %f\n", dist);
		// wrefresh(game->display.info);

		// if (dist < DIST_NO_HEAR && dist >= 0){
		// 	if (dist > DIST_NOISE){
		// 		// some random noise
		// 		int chances_noise = ((dist - DIST_NOISE) * 100) / (DIST_NO_HEAR - DIST_NOISE);
		// 		int ic = 0;
		// 		while (current->text_buffer[ic]){
		// 			if (rand() % 100 < chances_noise){
		// 				current->text_buffer[ic] = '.';
		// 			}
		// 			ic++;
		// 		}

		// 	} else {
		// 		// Hear good
		// 	}

			if (current->color <= MAX_COLOR){
				wattron(game->display.chat, COLOR_PAIR(current->color)); // Activer la couleur du message
			}
			wprintw(game->display.chat, " %s: %s\n", current->name, current->text_buffer); // Afficher le message
			if (current->color <= MAX_COLOR){
				wattroff(game->display.chat, COLOR_PAIR(current->color)); // Désactiver la couleur
			}
		// }

		Messages *next = current->next; // Sauvegarder le pointeur vers le message suivant
		free(current); // Libérer la mémoire du message actuel
		current = next; // Passer au message suivant
		
		printed = 1;
	}
	game->chat.messages = NULL; // Réinitialiser la liste des messages après affichage
	
	if (printed){
		pthread_mutex_lock(&game->display.m_display_update);
		wrefresh(game->display.chat); // Rafraîchir la fenêtre de chat
		pthread_mutex_unlock(&game->display.m_display_update);
	}
	pthread_mutex_unlock(&game->chat.m_chat_box);
	return printed;
}