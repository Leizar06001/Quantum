#include "includes.h"

static wchar_t *c_wall_diag_r 	= L"╱";
// static wchar_t *c_wall_diag_l 	= L"╲";
// static wchar_t *c_corner_ur 	= L"╭";
// static wchar_t *c_corner_ul 	= L"╮";
// static wchar_t *c_corner_dr 	= L"╯";
// static wchar_t *c_corner_dl 	= L"╰";
static wchar_t *c_horiz_one 	= L"─";
static wchar_t *c_horiz_two 	= L"═";
static wchar_t *c_vert_one 	= L"│";
// static wchar_t *c_vert_two 	= L"║";

void draw_map(Game *game){
	int center_x = game->display.width / 2;
	int center_y = game->display.height / 2;

	for (size_t y = 1; y < game->display.height - 1; y++) {
		for (size_t x = 1; x < game->display.width - 1; x++) {

			int map_x = game->player.x + (x - center_x);
			int map_y = game->player.y + (y - center_y);

			if (map_x >= 0 && map_x < (int)game->map.w && map_y >= 0 && map_y <= (int)game->map.h) {
				// if (map_y * game->map.w + map_x > game->map.map_len) continue;

				char ch = 0;
				switch (game->map.map[map_y * game->map.w + map_x]){
					case ' ':
						ch = ' ';
						break;

					case '1':
						ch = '.';
						break;
					
					default:
						ch = ' ';
						break;
				}
				if (ch) mvwaddch(game->display.chat, y, x, ch); // Dessiner le caractère
			} else {
				mvwaddch(game->display.chat, y, x, ' '); // Hors des limites de la carte
			}
		}
	}
	mvwaddch(game->display.chat, center_y, center_x, '@'); // Hors des limites de la carte
	wrefresh(game->display.chat);
}

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
		if (mdirs[i] == '1') nb_connections++;
	}

	if (nb_connections > 2) return 0;

	if (mL == '1' && mR == '1') return 1;
	if (mU == '1' && mD == '1') return 2;

	return 0;
}

static const int wall_h = 3;

static void draw_map_iso(Game *game){
	int center_x = game->display.width / 2;
	int center_y = game->display.height / 2;

	wattron(game->display.main_win, COLOR_PAIR(8));

	for (size_t y = 1; y < game->display.height - 1; y++) {
		for (size_t x = 1; x < game->display.width - 1; x++) {

			int map_x = game->player.x - game->player.y + (x - center_x);
			int map_y = game->player.y + (y - center_y);

			if (map_x + map_y >= 0 && map_x + map_y < (int)game->map.w && map_y >= 0 && map_y <= (int)game->map.h) {
				// if (map_y * game->map.w + map_x > game->map.map_len) continue;

				char ch = 0;
				int wtype;
				switch (game->map.map[map_y * game->map.w + (map_x + map_y)]){
					case ' ':
						ch = ' ';
						mvwaddch(game->display.main_win, y, x, ch);
						break;

					case '1':
						wtype = check_wall_type(game, (map_x + map_y), map_y);

						switch (wtype){
							case 0:
								mvwaddwstr(game->display.main_win, y, x, c_vert_one);
								for(int w = 0; w < wall_h; w++){
									if (y - w > 0) mvwaddwstr(game->display.main_win, y - w, x, c_vert_one);
								}
								if (y - wall_h > 0 && map_y - 1 > 0) {
									if (game->map.map[(map_y - 1) * game->map.w + (map_x + map_y)] == '1'){
										mvwaddwstr(game->display.main_win, y - wall_h, x, L"╒");
										continue;
									}
								}
								if (y - wall_h > 0)
									mvwaddch(game->display.main_win, y - wall_h, x, ',');
								break;

							case 1:
								mvwaddwstr(game->display.main_win, y, x, c_horiz_one);
								
								for(int w = 1; w < wall_h; w++){
									if (y - w > 0) mvwaddch(game->display.main_win, y - w, x, ' ');
								}
								// rch = (mvwinch(game->display.main_win, y - wall_h, x) & A_CHARTEXT);
								// if (y - wall_h > 0 && rch != '/') mvwaddch(game->display.main_win, y - wall_h, x, '_');

								cchar_t wcval;
								wchar_t wch;
								mvwin_wch(game->display.main_win, y - wall_h, x, &wcval);

								// Le tableau `wcval.chars` contient les caractères, terminé par L'\0'
								wch = wcval.chars[0];

								if (y - wall_h > 0 && wch != L'╲') {
									mvwaddwstr(game->display.main_win, y - wall_h, x, c_horiz_two);
								}

								break;

							case 2:
								mvwaddwstr(game->display.main_win, y, x, c_wall_diag_r);
								for(int w = 1; w < wall_h; w++){
									if (y - w > 0) mvwaddch(game->display.main_win, y - w, x, ' ');
								}
								if (y - wall_h > 0) mvwaddwstr(game->display.main_win, y - wall_h, x, c_wall_diag_r); //c_wall_diag_r);
								break;

							// case 3:
							// 	mvwaddch(game->display.main_win, y, x, '1');
							// 	for(int w = 0; w < wall_h; w++){
							// 		if (y - w > 0) mvwaddch(game->display.main_win, y - w, x, '|');
							// 	}
							// 	if (y - wall_h > 0 && mvwinch(game->display.main_win, y - wall_h, x) != '/') 
							// 		mvwaddch(game->display.main_win, y - wall_h, x, ',');
							// 	break;
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
	wattroff(game->display.main_win, COLOR_PAIR(8));

	int dist_wall = wall_h;
	for(int i = 0; i < wall_h; i++){
		// if (game->map.map[(game->player.y + i) * game->map.w + game->player.x + i] == '1'){
		// 	dist_wall = i;
		// 	break;
		// }
		int off_x = i + 1;
		int off_y = i;
		if (game->map.map[(game->player.y + off_y) * game->map.w + game->player.x + off_x] == '1'){
			mvwaddch(game->display.chat, center_y + off_y, center_x + off_x, 'H');
			wrefresh(game->display.chat);
			dist_wall = i;
			break;
		}
	}

	mvwaddwstr(game->display.main_win, center_y - 2, center_x, faces[game->player.face_id]); 
	if (dist_wall > wall_h - 2) mvwaddwstr(game->display.main_win, center_y - 1, center_x, L"🦺"); 
	if (dist_wall > wall_h - 1) mvwaddwstr(game->display.main_win, center_y - 0, center_x, L"👖"); 

	// mvwaddch(game->display.main_win, center_y - 1, center_x, '|'); 
	// mvwaddch(game->display.main_win, center_y, center_x, '|'); 
	wrefresh(game->display.main_win);
}

int start_test(Game *game){
	init_map(game, map_test);

	initscr();              // Démarrer ncurses
    cbreak();               // Lecture caractère par caractère
    noecho();               // Ne pas afficher les touches
    keypad(stdscr, TRUE);   // Activer les touches spéciales
	start_color();          // Activer les couleurs
	nodelay(stdscr, TRUE);
	setlocale(LC_ALL, "");

	init_color(8, 400, 200, 200);	// dark red
	init_color(9, 50, 50, 50);		// dark gray
	init_pair(8, 8, 9);				// Map

	const int w = 50, h = 30;

	game->display.height 	= h;
	game->display.width 	= w;

	game->display.main_win = newwin(h, w, 1, 1);
    box(game->display.main_win, 0, 0);

	game->display.chat = newwin(h, w, 1, 2 + w);
    box(game->display.chat, 0, 0);

	game->display.info = newwin(10, w * 2, 1 + h, 2);
	scrollok(game->display.info, TRUE);

	refresh();                 // Rafraîchir l'écran principal
    wrefresh(game->display.main_win);
	wrefresh(game->display.chat);
	wrefresh(game->display.info);

	game->player.x = 20;
	game->player.y = 1;

	draw_map(game);
	draw_map_iso(game);

	int center_x = game->display.width / 2;
	int center_y = game->display.height / 2;

	while (server_running){
		int ret = get_user_input(game);

		switch (ret){
			case IN_KEY_EXIT:
				server_running = 0;
				break;

			case IN_KEY_MOVE:
				draw_map(game);
				draw_map_iso(game);
				pinfo(game, "Pos: %d, %d. Map: %c, L: %c, R: %c, U: %c, D: %c, win: %c\n", 
					game->player.x, game->player.y, 
					game->map.map[game->player.y * game->map.w + game->player.x],
					game->map.map[game->player.y * game->map.w + game->player.x - 1],
					game->map.map[game->player.y * game->map.w + game->player.x + 1],
					game->map.map[(game->player.y - 1) * game->map.w + game->player.x],
					game->map.map[(game->player.y + 1) * game->map.w + game->player.x],
					mvwinch(game->display.main_win, center_y, center_x - 1) & A_CHARTEXT
				);
				break;

			case IN_KEY_COLOR:
				game->player.face_id++;
				if (game->player.face_id > NB_FACES - 1) game->player.face_id = 0;
				draw_map_iso(game);
				break;
		}


		usleep(10000);
	}

	delwin(game->display.main_win);                // Supprimer la fenêtre
    clear();             // Clear all ncurses windows
	refresh();           // Force draw to terminal
	endwin();            // Restore terminal state
	fflush(stdout);

	return 0;
}