#include "includes.h"
#include <wchar.h>

char *empty = " ";
wchar_t *wall 			= L"█";

wchar_t *desk 			= L"▒";

wchar_t *vOpenDoor 		= L"░";

wchar_t *vClosedDoor 	= L"▓";

wchar_t *lowLcorner 	= L"▙";
wchar_t *lowRcorner 	= L"◢";
wchar_t *topBlock		= L"▀";
wchar_t *botBlock		= L"▄";



int update_display(Game *game){
	if (!game) return -1;

	// The player will always be at the center of the window
	int center_x = game->display.width / 2;
	int center_y = game->display.height / 2;
	// Draw the map around the player
	wattron(game->display.main_win, COLOR_PAIR(8));
	for (size_t y = 1; y < game->display.height - 1; y++) {
		for (size_t x = 1; x < game->display.width - 1; x++) {
			int map_x = game->player.x + (x - center_x);
			int map_y = game->player.y + (y - center_y);
			if (map_x >= 0 && map_x < (int)game->map.w && map_y >= 0 && map_y <= (int)game->map.h) {
				wchar_t *ch = NULL;
				switch (game->map.map[map_y * game->map.w + map_x]){
					case ' ':
						if (game->map.map[(map_y + 1) * game->map.w + map_x] == 'c'){
							ch = topBlock;
							wattron(game->display.main_win, COLOR_PAIR(22));
						} else {
							mvwaddch(game->display.main_win, y, x, ' ');
						}
						
						break;
					case '1':
						wattron(game->display.main_win, COLOR_PAIR(8));
						ch = wall;
						break;
					case '2':
						wattron(game->display.main_win, COLOR_PAIR(22));
						ch = wall;
						break;
					case 'v':
						wattron(game->display.main_win, COLOR_PAIR(20));
						mvwaddwstr(game->display.main_win, y, x, vOpenDoor);
						wattron(game->display.main_win, COLOR_PAIR(8));
						break;
					case 'V':
						wattron(game->display.main_win, COLOR_PAIR(21));
						mvwaddwstr(game->display.main_win, y, x, vClosedDoor);
						wattron(game->display.main_win, COLOR_PAIR(8));
						break;
					case 'c':
						wattron(game->display.main_win, COLOR_PAIR(22));
						ch = lowLcorner;
						// ch = botBlock;
						break;
				}
				if (ch) mvwaddwstr(game->display.main_win, y, x, ch); // Dessiner le caractère
			} else {
				mvwaddch(game->display.main_win, y, x, ' '); // Hors des limites de la carte
			}
		}
	}
	wattroff(game->display.main_win, COLOR_PAIR(8));

	// Dessiner le joueur au centre de la fenêtre en rouge
	wattron(game->display.main_win, COLOR_PAIR(game->player.color)); // Activer la couleur du joueur
	mvwaddch(game->display.main_win, center_y, center_x, '@'); // Dessiner le joueur au centre
	wattroff(game->display.main_win, COLOR_PAIR(game->player.color)); // Désactiver la couleur

	// // Draw clients
	uint64_t t_now = millis();
	for (int i = 0; i < MAX_CLIENTS; i++) {
		if (game->clients[i].connected) {
			// wprintw(game->display.info, "Update client %d\n", i);
			int client_x = game->clients[i].x - game->player.x + center_x;
			int client_y = game->clients[i].y - game->player.y + center_y;
			if (client_x >= 1 && client_x < (int)game->display.width - 1 &&
				client_y >= 1 && client_y < (int)game->display.height - 1) {
				
				int color = 0;
				if (game->clients[i].color >= MIN_COLOR && game->clients[i].color <= MAX_COLOR){
					color = game->clients[i].color;
				}
				wattron(game->display.main_win, COLOR_PAIR(color)); // Activer la couleur du client
				mvwaddch(game->display.main_win, client_y, client_x, '@'); // Dessiner le client

				if (t_now - game->clients[i].last_msg < DURATION_MSG_NOTIF){
					mvwaddch(game->display.main_win, client_y - 1, client_x, '!');
				}

				wattroff(game->display.main_win, COLOR_PAIR(color)); // Désactiver la couleur
			}
		}
	}

	// refresh();
	wrefresh(game->display.main_win); // Rafraîchir la fenêtre principale
	// wrefresh(game->display.self_text); // Rafraîchir la fenêtre de texte
	move_cursor_back(game);

	return 0;
}

void move_cursor_back(Game *game){
	// pthread_mutex_lock(&game->display.m_display_update);
	wmove(game->display.self_text, 2, game->chat.text_size + 2);
	wrefresh(game->display.self_text);
	// pthread_mutex_unlock(&game->display.m_display_update);
}

int ask_for_display_update(Game *game){
	if (game == NULL) return -1; // Invalid game pointer

	pthread_mutex_lock(&game->display.m_display_update);
	game->display.need_main_update = 1; // Set the flag to update the main display
	pthread_mutex_unlock(&game->display.m_display_update);

	return 0;
}