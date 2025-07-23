#include <stdint.h>

extern const size_t map_w;
extern const size_t map_h;
extern const char map_c_empty;
extern const char map[];
extern const char map_test[];

int init_map(Game *game, const char *mapStr);
