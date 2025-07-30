#include "includes.h"

const size_t map_w = 62;
const size_t map_h = 88;
const char map_c_empty = ' ';

const char map_test[];

const char map[] = "\
11111111111111111111111111111111111111111111111111111111111111\
1                         1        1                         1\
1                         1        1                         1\
1                         1                                  1\
1                         1                                  1\
1                         1        111111111111111111111111111\
1                         v        1        1                1\
111111111111111111111111111        1        1                1\
1                         1                 1                1\
1                         1        1111111111                1\
1                         1        v                         1\
1                         1        1                         1\
1                         1        1                         1\
1                         v        1                         1\
111111111111111111111111111        111111111111111111111111111\
1                         1                                  1\
1                         1                                  1\
1                         1        1                         1\
1                         1        1                         1\
1                         1        1                         1\
1                         v        1                         1\
111111111111111111111111111        1                         1\
1                         1        1                         1\
1                         1        1                         1\
1                         1        1                         1\
1                         1        1                         1\
1                         1        1                         1\
1                         v        1                         1\
111111111111111111111111111        1                         1\
1                1                 1                         1\
1                1                 1                         1\
1                1                 1                         1\
1                1                 1                         1\
1                1                                           1\
1                v                                           1\
111111111111111111                 111111111111111111111111111\
1                1                                           1\
1                1                                           1\
1                1                 1                         1\
1                1                 1                         1\
1                1                 1                         1\
1                v                 1                         1\
111111111111111111                 1                         1\
1                1                 1                         1\
1                1                 1                         1\
1                1                 1                         1\
1                1                 1                         1\
1                1                                           1\
1                v                                           1\
111111111111111111111111111        111111111111111111111111111\
1                         v                                  1\
1                         1                                  1\
1                         1        1                         1\
1                         1        1                         1\
1                         1        1                         1\
1                         1        1                         1\
111111111111111111111111111        1                         1\
1                         v        1                         1\
1                         1        1                         1\
1                         1                                  1\
1                         1                                  1\
1                         1        111111111111111111111111111\
1                         1        1                         1\
111111111111111111111111111        1                         1\
1                         v        1                         1\
1                         1        1                         1\
1                         1        v                         1\
1                         1        111111111111111111111111111\
1                         1        1                         1\
1                         1        1                         1\
111111111111111111111111111        v                         1\
1        1                1        1                         1\
1        1                1        1                         1\
1        1                v        1                         1\
1111111 h111111111111111111        111111111111111111111111111\
1                                                            1\
1                                                            1\
1                                                            1\
1                                                            1\
1                                                            1\
1                                                            1\
1                                                            1\
1111111111111111111111111111111111111    1                   1\
2                                        1                   1\
2                                        1                   1\
2                                        1                   1\
2                                        1                   1\
22222222222222222222222222222222222222222111111111111111111111\
                                              \
                                              \
";


int init_map(Game *game, const char *mapStr){
	game->map.map_len = strlen(mapStr);
	game->map.map = (char *)malloc(sizeof(char) * game->map.map_len + 1); // Assigner la carte au jeu
	strcpy(game->map.map, mapStr);

	int nb_doors = 0;
	for(int i = 0; i < game->map.map_len; i++){
		if (game->map.map[i] == 'v' || game->map.map[i] == 'h'){
			game->map.doors[nb_doors].enabled = 1;
			game->map.doors[nb_doors].y = i / map_w;
			game->map.doors[nb_doors].x = i % map_w;
			game->map.doors[nb_doors].state = 0;
			if (game->map.map[i] == 'v'){
				game->map.doors[nb_doors].vertical = 1;
			} else {
				game->map.doors[nb_doors].vertical = 0;
			}
			// game->map.map[i] = map_c_empty;
			nb_doors++;
		}
		if (nb_doors > 20) return -1;
	}
	game->map.nb_doors = nb_doors;

	game->map.w = map_w; // Assigner la largeur de la carte
	game->map.h = map_h; // Assigner la hauteur de la carte
	game->map.map_c_empty = map_c_empty; // Assigner le caractère vide de la carte

	return 0;
}

