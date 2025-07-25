#include "includes.h"


static wchar_t *c_wall_diag_r 	= L"╱";
// static wchar_t *c_wall_diag_l 	= L"╲";
// static wchar_t *c_horiz_one 	= L"─";
static wchar_t *c_horiz_two 	= L"═";
static wchar_t *c_vert_one 	= L"│";
// static wchar_t *c_vert_two 	= L"║";

static int check_wall_type(Game *game, int x, int y){
	Map *mp = &game->map;
	char *mps = mp->map;

	char mL = 0, mR = 0, mU = 0, mD = 0;

	if (x > 0) 			mL = mps[y * mp->w + (x - 1)];
	if (x < mp->w - 1) 	mR = mps[y * mp->w + (x + 1)];
	if (y > 0)			mU = mps[(y - 1) * mp->w + x];
	if (y < mp->h - 1)	mD = mps[(y + 1) * mp->w + x];

	char mdirs[4] = {mL, mR, mU, mD};

	int nb_connections = 0;
	for(int i = 0; i < 4; i++){
		if (mdirs[i] == '1' || mdirs[i] == '2') nb_connections++;
	}

	if (nb_connections > 2) return 0;

	if ((mL == '1' || mL == '2') && (mR == '1' || mR == '2')) return 1;
	if ((mU == '1' || mU == '2') && (mD == '1' || mD == '2')) return 2;

	return 0;
}

void clear_emoji_if_needed(WINDOW *win, int y, int x) {
    cchar_t cval;
    wchar_t wc;

    // Check (y, x - 1)
    if (x > 0 && mvwin_wch(win, y, x - 1, &cval) != ERR) {
        wc = cval.chars[0];
        if (wcwidth(wc) == 2) {
            mvwaddch(win, y, x - 1, ' ');
            mvwaddch(win, y, x, ' ');
        }
    }

    // Else check (y, x)
    else if (mvwin_wch(win, y, x, &cval) != ERR) {
        wc = cval.chars[0];
        if (wcwidth(wc) == 2) {
            mvwaddch(win, y, x, ' ');
            mvwaddch(win, y, x + 1, ' ');
        }
    }
}

static const int wall_h = 3;
static const int small_wall_h = 2;

void set_map_color(Game *game, int pair){
	if (game->connected_to_server){
		wattron(game->display.main_win, COLOR_PAIR(pair));
	} else {
		wattron(game->display.main_win, COLOR_PAIR(9));
	}
}

static void draw_map_iso(Game *game){
	int center_x = game->display.width / 2;
	int center_y = game->display.height / 2;

	wattron(game->display.main_win, A_BOLD);
	set_map_color(game, 8);

	for (size_t y = 1; y < game->display.height - 1; y++) {
		for (size_t x = 1; x < game->display.width - 1; x++) {

			int map_x = game->player.x - game->player.y + (x - center_x);
			int map_y = game->player.y + (y - center_y);

			if (map_x + map_y >= 0 && map_x + map_y < (int)game->map.w && map_y >= 0 && map_y <= (int)game->map.h) {
				// if (map_y * game->map.w + map_x > game->map.map_len) continue;

				char ch = 0;
				int wtype;
				char cur_tile = game->map.map[map_y * game->map.w + (map_x + map_y)];
				int cur_wall_h = wall_h;
				switch (cur_tile){
					case ' ':
						// ch = ' ';
						// mvwaddch(game->display.main_win, y, x, ch);
						set_map_color(game, 22);
						mvwaddwstr(game->display.main_win, y, x, L"╳");
						set_map_color(game, 8);
						break;

					case '1':
					case '2':
						if (cur_tile == '2') cur_wall_h = small_wall_h;
						wtype = check_wall_type(game, (map_x + map_y), map_y);

						switch (wtype){
							case 0:
								mvwaddwstr(game->display.main_win, y, x, c_vert_one);
								for(int w = 0; w < cur_wall_h; w++){
									if (y - w > 0) mvwaddwstr(game->display.main_win, y - w, x, c_vert_one);
								}
								if (y - cur_wall_h > 0 && map_y - 1 > 0) {
									if (game->map.map[(map_y - 1) * game->map.w + (map_x + map_y)] == '1'){
										mvwaddwstr(game->display.main_win, y - cur_wall_h, x, L"╒");
										continue;
									}
								}
								if (y - cur_wall_h > 0)
									mvwaddch(game->display.main_win, y - cur_wall_h, x, ',');
								break;

							case 1:
								mvwaddwstr(game->display.main_win, y, x, L"_");
								
								for(int w = 1; w < cur_wall_h; w++){
									if (y - w > 0) mvwaddch(game->display.main_win, y - w, x, ' ');
								}
								// rch = (mvwinch(game->display.main_win, y - cur_wall_h, x) & A_CHARTEXT);
								// if (y - cur_wall_h > 0 && rch != '/') mvwaddch(game->display.main_win, y - cur_wall_h, x, '_');

								cchar_t wcval;
								wchar_t wch;
								mvwin_wch(game->display.main_win, y - cur_wall_h, x, &wcval);

								// Le tableau `wcval.chars` contient les caractères, terminé par L'\0'
								wch = wcval.chars[0];

								if (y - cur_wall_h > 0 && wch != L'╲') {
									mvwaddwstr(game->display.main_win, y - cur_wall_h, x, c_horiz_two);
								}

								break;

							case 2:
								mvwaddwstr(game->display.main_win, y, x, c_wall_diag_r);
								for(int w = 1; w < cur_wall_h; w++){
									if (y - w > 0) mvwaddch(game->display.main_win, y - w, x, ' ');
								}
								if (y - cur_wall_h > 0) mvwaddwstr(game->display.main_win, y - cur_wall_h, x, c_wall_diag_r); //c_wall_diag_r);
								break;

						}
						break;
					
					default:
						ch = ' ';
						mvwaddch(game->display.main_win, y, x, ch);
						break;
				}
				// if (ch) mvwaddch(game->display.main_win, y, x, ch); // Dessiner le caractère
			} else {
				mvwaddch(game->display.main_win, y, x, ' '); // Hors des limites de la carte
			}
		}
	}
	

	int dist_wall = wall_h;
	int hide_player_body = 0;
	int hide_player_legs = 0;


	// Draw other clients
	// uint64_t t_now = millis();
	dist_wall = wall_h;
	for (int i = 0; i < MAX_CLIENTS; i++) {
		if (game->clients[i].connected) {
			// wprintw(game->display.info, "Update client %d\n", i);
			int client_x = game->clients[i].x - game->player.x + center_x + game->player.y - game->clients[i].y;
			int client_y = game->clients[i].y - game->player.y + center_y;
			if (client_x >= 1 && client_x < (int)game->display.width - 1 &&
				client_y >= 1 && client_y < (int)game->display.height - 1) {
				
				// int color = MIN_COLOR;
				// if (game->clients[i].color >= MIN_COLOR && game->clients[i].color <= MAX_COLOR){
				// 	color = game->clients[i].color;
				// }
				// wattron(game->display.main_win, COLOR_PAIR(color)); // Activer la couleur du client
				// if (t_now - game->clients[i].last_msg < DURATION_MSG_NOTIF){
				// 	mvwaddch(game->display.main_win, client_y - 1, client_x, '!');
				// }
				// wattroff(game->display.main_win, COLOR_PAIR(color)); // Désactiver la couleur

				for(int w = 0; w < wall_h; w++){
					int off_x = w + 1;
					int off_y = w;
					if (game->map.map[(game->clients[i].y + off_y) * game->map.w + game->clients[i].x + off_x] == '1'){
						dist_wall = w;
						break;
					}
				}
				// For drawing, we check its not hidden by walls or not hidden by player
				if (client_y - 2 > 0) 
					mvwaddwstr(game->display.main_win, client_y - 2, client_x, faces[game->clients[i].face_id]); 
				
				if (client_y - 1 > 0 && dist_wall > wall_h - 2 && !(client_y == center_y - 1 && abs(client_x - center_x) < 2))
					mvwaddwstr(game->display.main_win, client_y - 1, client_x, bodies[game->clients[i].body_id]); 

				if (dist_wall > wall_h - 1 && !((client_y == center_y - 2 || client_y == center_y - 1) && abs(client_x - center_x) < 2))
					mvwaddwstr(game->display.main_win, client_y - 0, client_x, legs[game->clients[i].legs_id]); 

				if (client_y == center_y + 1 && abs(client_x - center_x) < 2)
					hide_player_body = 1;
					
				if ((client_y == center_y + 2 || client_y == center_y + 1) && abs(client_x - center_x) < 2) 
					hide_player_legs = 1;

				wrefresh(game->display.info);
			}
		}
	}

 
	// Draw player
	int cur_wall_h = wall_h;
	dist_wall = wall_h;
	for(int i = 0; i < wall_h; i++){
		int off_x = i + 1;
		int off_y = i;
		char mp_tile = game->map.map[(game->player.y + off_y) * game->map.w + game->player.x + off_x];
		if (mp_tile == '1'){
			dist_wall = i;
			break;
		}
		if (mp_tile == '2'){
			cur_wall_h = small_wall_h;
			dist_wall = i;
			break;
		}
	}
	// DRAW
	mvwaddwstr(game->display.main_win, center_y - 2, center_x, faces[game->player.face_id]); 
	if (dist_wall > cur_wall_h - 2 && !hide_player_body) 
		mvwaddwstr(game->display.main_win, center_y - 1, center_x, bodies[game->player.body_id]); 
	if (dist_wall > cur_wall_h - 1 && !hide_player_legs) 
		mvwaddwstr(game->display.main_win, center_y - 0, center_x, legs[game->player.legs_id]);


	wattroff(game->display.main_win, COLOR_PAIR(8));
	wrefresh(game->display.main_win);
}









char *empty = " ";
wchar_t *wall 			= L"█";

wchar_t *desk 			= L"▒";

wchar_t *vOpenDoor 		= L"░";

wchar_t *vClosedDoor 	= L"▓";

wchar_t *lowLcorner 	= L"▙";
wchar_t *lowRcorner 	= L"▟";
wchar_t *topLcorner 	= L"▛";
wchar_t *topRcorner 	= L"▜";

wchar_t *topBlock		= L"▀";
wchar_t *botBlock		= L"▄";


int update_display(Game *game){
	if (!game) return -1;

	curs_set(0);

	draw_map_iso(game);

	// // The player will always be at the center of the window
	// int center_x = game->display.width / 2;
	// int center_y = game->display.height / 2;
	// // Draw the map around the player
	// wattron(game->display.main_win, COLOR_PAIR(8));
	// for (size_t y = 1; y < game->display.height - 1; y++) {
	// 	for (size_t x = 1; x < game->display.width - 1; x++) {
	// 		int map_x = game->player.x + (x - center_x);
	// 		int map_y = game->player.y + (y - center_y);

	// 		if (map_x >= 0 && map_x < (int)game->map.w && map_y >= 0 && map_y <= (int)game->map.h) {
	// 			// if (map_y * game->map.w + map_x > game->map.map_len) continue;

	// 			wchar_t *ch = NULL;
	// 			switch (game->map.map[map_y * game->map.w + map_x]){
	// 				case ' ':
	// 					if (game->map.map[(map_y + 1) * game->map.w + map_x] == 'c' || 
	// 						(game->map.map[(map_y + 1) * game->map.w + map_x] == '1' && game->map.map[(map_y - 1) * game->map.w + map_x] == '2' && (game->map.map[map_y * game->map.w + map_x + 1] == 'v' || game->map.map[map_y * game->map.w + map_x - 1] == 'v'))){
	// 						ch = topBlock;
	// 						wattron(game->display.main_win, COLOR_PAIR(22));
	// 					} else {
	// 						mvwaddch(game->display.main_win, y, x, ' ');
	// 					}
						
	// 					break;
	// 				case '1':
	// 					wattron(game->display.main_win, COLOR_PAIR(8));
	// 					ch = wall;
	// 					break;
	// 				case '2':
	// 					wattron(game->display.main_win, COLOR_PAIR(22));
	// 					ch = wall;
	// 					break;
	// 				case 'v':
	// 					wattron(game->display.main_win, COLOR_PAIR(20));
	// 					mvwaddwstr(game->display.main_win, y, x, vOpenDoor);
	// 					wattron(game->display.main_win, COLOR_PAIR(8));
	// 					break;
	// 				case 'V':
	// 					wattron(game->display.main_win, COLOR_PAIR(21));
	// 					mvwaddwstr(game->display.main_win, y, x, vClosedDoor);
	// 					wattron(game->display.main_win, COLOR_PAIR(8));
	// 					break;
	// 				case 'c':
	// 					wattron(game->display.main_win, COLOR_PAIR(22));
	// 					ch = lowLcorner;
	// 					// ch = botBlock;
	// 					break;
	// 				case 'o':
	// 					wattron(game->display.main_win, COLOR_PAIR(23));
	// 					ch = topLcorner;
	// 					break;
	// 				case 'p':
	// 					wattron(game->display.main_win, COLOR_PAIR(23));
	// 					ch = topRcorner;
	// 					break;
	// 				case 'k':
	// 					wattron(game->display.main_win, COLOR_PAIR(23));
	// 					ch = lowLcorner;
	// 					break;
	// 				case 'l':
	// 					wattron(game->display.main_win, COLOR_PAIR(23));
	// 					ch = lowRcorner;
	// 					break;
	// 				case 'i':
	// 					wattron(game->display.main_win, COLOR_PAIR(23));
	// 					ch = topBlock;
	// 					break;
	// 				case 'j':
	// 					wattron(game->display.main_win, COLOR_PAIR(23));
	// 					ch = botBlock;
	// 					break;
	// 				case 'u':
	// 					wattron(game->display.main_win, COLOR_PAIR(23));
	// 					ch = vClosedDoor;
	// 					break;
	// 			}
	// 			if (ch) mvwaddwstr(game->display.main_win, y, x, ch); // Dessiner le caractère
	// 		} else {
	// 			mvwaddch(game->display.main_win, y, x, ' '); // Hors des limites de la carte
	// 		}
	// 	}
	// }
	// wattroff(game->display.main_win, COLOR_PAIR(8));

	// // Dessiner le joueur au centre de la fenêtre en rouge
	// wattron(game->display.main_win, COLOR_PAIR(game->player.color)); // Activer la couleur du joueur
	// mvwaddch(game->display.main_win, center_y - 1, center_x, '@'); // Dessiner le joueur au centre
	// mvwaddwstr(game->display.main_win, center_y, center_x, L"|");
	// wattroff(game->display.main_win, COLOR_PAIR(game->player.color)); // Désactiver la couleur

	// // Draw clients
	// uint64_t t_now = millis();
	// for (int i = 0; i < MAX_CLIENTS; i++) {
	// 	if (game->clients[i].connected) {
	// 		// wprintw(game->display.info, "Update client %d\n", i);
	// 		int client_x = game->clients[i].x - game->player.x + center_x;
	// 		int client_y = game->clients[i].y - game->player.y + center_y;
	// 		if (client_x >= 1 && client_x < (int)game->display.width - 1 &&
	// 			client_y >= 1 && client_y < (int)game->display.height - 1) {
				
	// 			int color = 0;
	// 			if (game->clients[i].color >= MIN_COLOR && game->clients[i].color <= MAX_COLOR){
	// 				color = game->clients[i].color;
	// 			}
	// 			wattron(game->display.main_win, COLOR_PAIR(color)); // Activer la couleur du client
	// 			if (client_y > 1){
	// 				mvwaddch(game->display.main_win, client_y - 1, client_x, '@'); // Dessiner le client
	// 			}
	// 			mvwaddwstr(game->display.main_win, client_y, client_x, L"|");

	// 			if (t_now - game->clients[i].last_msg < DURATION_MSG_NOTIF){
	// 				mvwaddch(game->display.main_win, client_y - 1, client_x, '!');
	// 			}

	// 			wattroff(game->display.main_win, COLOR_PAIR(color)); // Désactiver la couleur
	// 		}
	// 	}
	// }

	// refresh();
	// wrefresh(game->display.main_win); // Rafraîchir la fenêtre principale
	// wrefresh(game->display.self_text); // Rafraîchir la fenêtre de texte
	move_cursor_back(game);

	curs_set(1);

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