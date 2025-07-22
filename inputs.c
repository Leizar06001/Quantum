#include "includes.h"


#define KEY_COLOR_CH 	265	// F1 key for changing color
#define KEY_CHANGE_DOOR 266	// F2
#define KEY_QUIT 		27 	// ESC key

int check_for_move(Game *game, int x, int y){
	if (game->map.map[y * game->map.w + x] != game->map.map_c_empty && 	// empty
		game->map.map[y * game->map.w + x] != 'v') {					// opened door
			return -1;
	}

	for(int i = 1; i < MAX_CLIENTS; i++){
		if (game->clients[i].connected){
			if (game->clients[i].x == x && game->clients[i].y == y){
				return -2;
			}
		}
	}
	return 0;
}

int get_user_input(Game *game) {
	if (!game) return -1; // Vérification de la validité du pointeur
	int ch = getch(); // Obtenir l'entrée de l'utilisateur
	if (ch == ERR) {
		return 0; // Aucune touche pressée, on ne fait rien
	}

	int moved = 0; // Variable pour vérifier si le joueur a bougé
	int newx = game->player.x;
	int newy = game->player.y;
	switch (ch) {
		case KEY_UP: // Haut
			newy--;
			if (check_for_move(game, newx, newy) == 0){
				game->player.y = newy;
				moved = 1;
			}
			break;
		case KEY_DOWN: // Bas
			newy++;
			if (check_for_move(game, newx, newy) == 0){
				game->player.y = newy;
				moved = 1;
			}
			break;
		case KEY_LEFT: // Gauche
			newx--;
			if (check_for_move(game, newx, newy) == 0){
				game->player.x = newx;
				moved = 1;
			}
			break;
		case KEY_RIGHT: // Droite
			newx++;
			if (check_for_move(game, newx, newy) == 0){
				game->player.x = newx;
				moved = 1;
			}
			break;
		case KEY_COLOR_CH: // F1 Changer de couleur
			pthread_mutex_lock(&game->chat.m_send_text);
			game->player.color++;
			if (game->player.color > MAX_COLOR) game->player.color = MIN_COLOR;
			game->chat.new_color = 1;
			pthread_mutex_unlock(&game->chat.m_send_text);
			return IN_KEY_COLOR;

		case KEY_CHANGE_DOOR:
			for(int i = 0; i < game->map.nb_doors; i++){
				if (distance(game->player.x, game->player.y, game->map.doors[i].x, game->map.doors[i].y) <= 1){
					if (game->map.doors[i].state == 0){
						game->map.doors[i].state = 1;
						game->map.map[game->map.doors[i].y * game->map.w + game->map.doors[i].x] = 'V';
					} else {
						game->map.doors[i].state = 0;
						game->map.map[game->map.doors[i].y * game->map.w + game->map.doors[i].x] = 'v';
					}
					pthread_mutex_lock(&game->chat.m_send_text);
					game->chat.door_change_id = i;
					game->chat.new_door = 1;
					pthread_mutex_unlock(&game->chat.m_send_text);
					break;
				}
			}
			return IN_KEY_DOOR;

		case KEY_QUIT: // ESC Quitter le jeu 
			return IN_KEY_EXIT;

		case KEY_BACKSPACE: // Touche de retour arrière
		case 127: // Touche de suppression (pour certains terminaux)
			// Supprimer le dernier caractère du tampon de texte
			if (game->chat.text_size > 0) {
				game->chat.text_buffer[--game->chat.text_size] = '\0'; // Réduire la taille du tampon et terminer la chaîne
			}
			return IN_KEY_TEXT;

		case '\n': // Touche Entrée
			// Envoi le texte et efface le tampon
			if (game->chat.text_size > 0) {
				pthread_mutex_lock(&game->chat.m_send_text); // Obtenir le mutex pour envoyer le texte
				if (game->chat.ready_to_send == 0){
					game->chat.text_buffer[game->chat.text_size] = '\0'; // Assurer la terminaison de la chaîne
					add_message(game, game->player.name, game->player.color, game->chat.text_buffer, -1); // Ajouter le message à la liste
					strncpy(game->chat.text_to_send, game->chat.text_buffer, game->chat.text_size);
					game->chat.text_to_send[game->chat.text_size] = '\0'; // Assurer la terminaison de la chaîne
					game->chat.ready_to_send = 1; // Indiquer que le texte est prêt à être envoyé
					// Réinitialiser le tampon de texte
					game->chat.text_buffer[0] = '\0'; // Réinitialiser le tampon de texte
					game->chat.text_size = 0; // Réinitialiser la taille du tampon

					pthread_mutex_unlock(&game->chat.m_send_text); // Libérer le mutex
					return IN_KEY_SEND; // Indiquer que le texte a été envoyé
				}
				pthread_mutex_unlock(&game->chat.m_send_text); // Libérer le mutex
			}
			break;
		default:
			// On check si la touche est ascii imprimable
			if (ch >= 32 && ch <= 126) {
				// Ajout au tampon de texte
				if (game->chat.text_size < 97) {
					game->chat.text_buffer[game->chat.text_size++] = (char)ch; // Ajouter le caractère au tampon
					game->chat.text_buffer[game->chat.text_size] = '\0'; // Terminer la chaîne
				}
				
				return IN_KEY_TEXT;
			}
			// Si une touche non gérée est pressée, on ne fait rien
			// mvprintw(3, 0, "Key pressed: %d\n", ch);
			// refresh();
			return IN_KEY_UNKN;
	}

	if (moved){
		pthread_mutex_lock(&game->chat.m_send_text);
		game->player.lastx = game->player.x; // Sauvegarder la position précédente
		game->player.lasty = game->player.y; // Sauvegarder la position précédente
		game->chat.new_pos = 1; // Indiquer que la position du joueur a changé
		pthread_mutex_unlock(&game->chat.m_send_text);
		return IN_KEY_MOVE;
	}
	return IN_KEY_UNKN;
}