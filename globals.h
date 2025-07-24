#ifndef GLOBALS_H
#define GLOBALS_H

#include <ncurses.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define NB_FACES 39
#define NB_BODYS 13
#define NB_LEGS 13

extern wchar_t *faces[];
extern wchar_t *bodies[];
extern wchar_t *legs[];

#endif
