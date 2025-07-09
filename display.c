#include "includes.h"

int update_display(Game *game){
	if (!game) return -1;

	// The player will always be at the center of the window
	int center_x = game->display.width / 2;
	int center_y = game->display.height / 2;
	// Draw the map around the player
	for (size_t y = 1; y < game->display.height - 1; y++) {
		for (size_t x = 1; x < game->display.width - 1; x++) {
			int map_x = game->player.x + (x - center_x);
			int map_y = game->player.y + (y - center_y);
			if (map_x >= 0 && map_x < (int)game->map.w && map_y >= 0 && map_y < (int)game->map.h) {
				char ch = game->map.map[map_y * game->map.w + map_x] == ' ' ? ' ' : '█';
				mvwaddch(game->display.main_win, y, x, ch); // Dessiner le caractère
			} else {
				mvwaddch(game->display.main_win, y, x, ' '); // Hors des limites de la carte
			}
		}
	}

	// Dessiner le joueur au centre de la fenêtre en rouge
	wattron(game->display.main_win, COLOR_PAIR(game->player.color + 1)); // Activer la couleur du joueur
	mvwaddch(game->display.main_win, center_y, center_x, '@'); // Dessiner le joueur au centre
	wattroff(game->display.main_win, COLOR_PAIR(game->player.color + 1)); // Désactiver la couleur

	// Draw clients
	for (int i = 0; i < MAX_CLIENTS; i++) {
		if (game->clients[i].connected) {
			int client_x = game->clients[i].x - game->player.x + center_x;
			int client_y = game->clients[i].y - game->player.y + center_y;
			if (client_x >= 1 && client_x < (int)game->display.width - 1 &&
				client_y >= 1 && client_y < (int)game->display.height - 1) {
				wattron(game->display.main_win, COLOR_PAIR(game->clients[i].color + 1)); // Activer la couleur du client
				mvwaddch(game->display.main_win, client_y, client_x, '@'); // Dessiner le client
				wattroff(game->display.main_win, COLOR_PAIR(game->clients[i].color + 1)); // Désactiver la couleur
			}
		}
	}

	wmove(game->display.self_text, 1, game->chat.text_size + 2); // Positionner le curseur à la fin du texte
	wrefresh(game->display.main_win); // Rafraîchir la fenêtre principale
	wrefresh(game->display.self_text); // Rafraîchir la fenêtre de texte
	return 0;
}