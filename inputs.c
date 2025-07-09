#include "includes.h"


#define KEY_COLOR_CH 	265	// F1 key for changing color
#define KEY_QUIT 		27 	// ESC key


int get_user_input(Game *game) {
	if (!game) return -1; // Vérification de la validité du pointeur
	int ch = getch(); // Obtenir l'entrée de l'utilisateur
	if (ch == ERR) {
		return 0; // Aucune touche pressée, on ne fait rien
	}

	int moved = 0; // Variable pour vérifier si le joueur a bougé
	switch (ch) {
		case KEY_UP: // Haut
			if (game->map.map[(game->player.y - 1) * game->map.w + game->player.x] == game->map.map_c_empty) {
				game->player.y--;
				moved = 1; // Le joueur a bougé
			}
			break;
		case KEY_DOWN: // Bas
			if (game->map.map[(game->player.y + 1) * game->map.w + game->player.x] == game->map.map_c_empty) {
				game->player.y++;
				moved = 1;
			}
			break;
		case KEY_LEFT: // Gauche
			if (game->map.map[game->player.y * game->map.w + (game->player.x - 1)] == game->map.map_c_empty) {
				game->player.x--;
				moved = 1;
			}
			break;
		case KEY_RIGHT: // Droite
			if (game->map.map[game->player.y * game->map.w + (game->player.x + 1)] == game->map.map_c_empty) {
				game->player.x++;
				moved = 1;
			}
			break;
		case KEY_COLOR_CH: // F1 Changer de couleur
			game->player.color = (game->player.color + 1) % 6; // Changer la couleur du joueur
			break;
		case KEY_QUIT: // ESC Quitter le jeu 
			return -1;
		case KEY_BACKSPACE: // Touche de retour arrière
		case 127: // Touche de suppression (pour certains terminaux)
			// Supprimer le dernier caractère du tampon de texte
			if (game->chat.text_size > 0) {
				game->chat.text_buffer[--game->chat.text_size] = '\0'; // Réduire la taille du tampon et terminer la chaîne
			}
			return 2;
		case '\n': // Touche Entrée
			// Envoi le texte et efface le tampon
			if (game->chat.text_size > 0) {
				pthread_mutex_lock(&game->chat.m_send_text); // Obtenir le mutex pour envoyer le texte
				if (game->chat.ready_to_send == 0){
					game->chat.text_buffer[game->chat.text_size] = '\0'; // Assurer la terminaison de la chaîne
					add_message(game, game->player.name, game->player.color, game->chat.text_buffer); // Ajouter le message à la liste
					strncpy(game->chat.text_to_send, game->chat.text_buffer, game->chat.text_size);
					game->chat.text_to_send[game->chat.text_size] = '\0'; // Assurer la terminaison de la chaîne
					game->chat.ready_to_send = 1; // Indiquer que le texte est prêt à être envoyé
					// Réinitialiser le tampon de texte
					game->chat.text_buffer[0] = '\0'; // Réinitialiser le tampon de texte
					game->chat.text_size = 0; // Réinitialiser la taille du tampon

					pthread_mutex_unlock(&game->chat.m_send_text); // Libérer le mutex
					return 3; // Indiquer que le texte a été envoyé
				}
				pthread_mutex_unlock(&game->chat.m_send_text); // Libérer le mutex
			}
			break;
		default:
			// On check si la touche est ascii imprimable
			if (ch >= 32 && ch <= 126) {
				// Ajout au tampon de texte
				if (game->chat.text_size < game->display.width - 4) {
					game->chat.text_buffer[game->chat.text_size++] = (char)ch; // Ajouter le caractère au tampon
					game->chat.text_buffer[game->chat.text_size] = '\0'; // Terminer la chaîne
				}
				return 2;
			}
			// Si une touche non gérée est pressée, on ne fait rien
			mvprintw(3, 0, "Key pressed: %d\n", ch);
			refresh();
			return 0;
	}

	if (moved){
		pthread_mutex_lock(&game->chat.m_send_text);
		game->player.lastx = game->player.x; // Sauvegarder la position précédente
		game->player.lasty = game->player.y; // Sauvegarder la position précédente
		game->chat.new_pos = 1; // Indiquer que la position du joueur a changé
		pthread_mutex_unlock(&game->chat.m_send_text);
	}
	return 1;
}