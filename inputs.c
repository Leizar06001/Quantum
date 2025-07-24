#include "includes.h"


#define KEY_COLOR_CH 	265	// F1 key for changing color
#define KEY_CHANGE_DOOR 266	// F2
#define KEY_TOGGLE_NOTIFS 267	// F3
#define KEY_F4	268
#define KEY_QUIT 		27 	// ESC key

int check_for_move(Game *game, int x, int y){
	// return 0;	// for tests

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

int menu_character(Game *game){
	// Report mouse events
	mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
	mouseinterval(0);

	// Crée une fenêtre
    int height = 18, width = 30, starty = 4, startx = 3;
	const int valid_x = 3;
	const int cancel_x = valid_x + 10;
	const int btns_y = height - 2;
    WINDOW *win = newwin(height, width, starty, startx);
    box(win, 0, 0);
    mvwprintw(win, 1, 7, "Menu personnage");
	mvwprintw(win, btns_y, valid_x, "Valider");
	mvwprintw(win, btns_y, cancel_x, "Annuler");

	const int icons_per_row = (width - 4) / 2;
	int y = 0, x = 0;
	// Draw faces
	const int f_start_y = 3, f_start_x = 2;
	const int nb_faces_rows = (NB_FACES + icons_per_row - 1) / icons_per_row;
	for(int i = 0; i < NB_FACES; i++){
		mvwaddwstr(win, f_start_y + y, f_start_x + x * 2, faces[i]);
		x++;
		if (x >= icons_per_row){
			x = 0; y++;
		}
	}

	// Draw bodies
	y = 0, x = 0;
	const int b_start_y = f_start_y + nb_faces_rows + 1, b_start_x = 2;
	const int nb_bodies_rows = (NB_BODYS + icons_per_row - 1) / icons_per_row;
	for(int i = 0; i < NB_BODYS; i++){
		mvwaddwstr(win, b_start_y + y, b_start_x + x * 2, bodies[i]);
		x++;
		if (x >= icons_per_row){
			x = 0; y++;
		}
	}

	// Draw legs
	y = 0, x = 0;
	const int l_start_y = b_start_y + nb_bodies_rows + 1, l_start_x = 2;
	const int nb_legs_rows = (NB_LEGS + icons_per_row - 1) / icons_per_row;
	for(int i = 0; i < NB_LEGS; i++){
		mvwaddwstr(win, l_start_y + y, l_start_x + x * 2, legs[i]);
		x++;
		if (x >= icons_per_row){
			x = 0; y++;
		}
	}

	int choosen_face = game->player.face_id;
	int choosen_body = game->player.body_id;
	int choosen_legs = game->player.legs_id;

	mvwaddwstr(win, height - 4, width - 4, faces[choosen_face]);
	mvwaddwstr(win, height - 3, width - 4, bodies[choosen_body]);
	mvwaddwstr(win, height - 2, width - 4, legs[choosen_legs]);

    wrefresh(win);

	int ret = 0;

    MEVENT event;
    int ch;
    while ((ch = wgetch(stdscr)) != KEY_QUIT) {
        if (ch == KEY_MOUSE) {
            if (getmouse(&event) == OK) {
                // Vérifie si le clic est dans la fenêtre
                if (wenclose(win, event.y, event.x)) {
                    int local_y = event.y - starty;
                    int local_x = event.x - startx;

					// Boutons
					if (local_y == btns_y){
						if (local_x >= valid_x && local_x < valid_x + 7){
							game->player.face_id = choosen_face;
							game->player.body_id = choosen_body;
							game->player.legs_id = choosen_legs;
							write_config_int("face", choosen_face);
							write_config_int("body", choosen_body);
							write_config_int("legs", choosen_legs);
							ret = 1;
							break;
						} else if (local_x >= cancel_x && local_x < cancel_x + 7){
							break;
						}
					}

					// Faces
					if (local_y >= f_start_y && local_y < f_start_y + nb_faces_rows){
						if (local_x >= f_start_x && local_x <= f_start_x + icons_per_row * 2){
							int face_row = local_y - f_start_y;
							int face_row_ind = (local_x - f_start_x) / 2;
							int new_face = face_row * icons_per_row + face_row_ind;
							if (new_face >= 0 && new_face < NB_FACES) choosen_face = new_face;
						}
					}
					// Bodies
					if (local_y >= b_start_y && local_y < b_start_y + nb_bodies_rows){
						if (local_x >= b_start_x && local_x <= b_start_x + icons_per_row * 2){
							int body_row = local_y - b_start_y;
							int body_row_ind = (local_x - b_start_x) / 2;
							int new_body = body_row * icons_per_row + body_row_ind;
							if (new_body >= 0 && new_body < NB_BODYS) choosen_body = new_body;
						}
					}
					// Legs
					if (local_y >= l_start_y && local_y < l_start_y + nb_legs_rows){
						if (local_x >= l_start_x && local_x <= l_start_x + icons_per_row * 2){
							int leg_row = local_y - l_start_y;
							int leg_row_ind = (local_x - l_start_x) / 2;
							int new_legs = leg_row * icons_per_row + leg_row_ind;
							if (new_legs >= 0 && new_legs < NB_LEGS) choosen_legs = new_legs;
						}
					}

					mvwaddwstr(win, height - 4, width - 4, faces[choosen_face]);
					mvwaddwstr(win, height - 3, width - 4, bodies[choosen_body]);
					mvwaddwstr(win, height - 2, width - 4, legs[choosen_legs]);

                    wrefresh(win);
                }
            }
        }
    }

    delwin(win);
	mousemask(0, NULL);
	refresh();
	
	return ret;
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
		
		case KEY_TOGGLE_NOTIFS:
			if (game->notif_enabled){
				game->notif_enabled = 0;
			} else {
				game->notif_enabled = 1;
			}
			return IN_KEY_NOTIF;

		case KEY_F4:
			if (menu_character(game)) {
				pthread_mutex_lock(&game->chat.m_send_text);
				game->chat.new_perso = 1;
				pthread_mutex_unlock(&game->chat.m_send_text);
			}
			return IN_KEY_PERSO;

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