// terminal_colors.h

#ifndef TERMINAL_ESC_SEQ_H
#define TERMINAL_ESC_SEQ_H

#include <termios.h>

// Reset
#define RESET "\033[0m"

// Regular Colors
#define BLACK "\033[0;30m"
#define RED "\033[0;31m"
#define GREEN "\033[0;32m"
#define YELLOW "\033[0;33m"
#define BLUE "\033[0;34m"
#define MAGENTA "\033[0;35m"
#define CYAN "\033[0;36m"
#define WHITE "\033[0;37m"

// Bold
#define BOLD_BLACK "\033[1;30m"
#define BOLD_RED "\033[1;31m"
#define BOLD_GREEN "\033[1;32m"
#define BOLD_YELLOW "\033[1;33m"
#define BOLD_BLUE "\033[1;34m"
#define BOLD_MAGENTA "\033[1;35m"
#define BOLD_CYAN "\033[1;36m"
#define BOLD_WHITE "\033[1;37m"

// Underline
#define UNDERLINE_BLACK "\033[4;30m"
#define UNDERLINE_RED "\033[4;31m"
#define UNDERLINE_GREEN "\033[4;32m"
#define UNDERLINE_YELLOW "\033[4;33m"
#define UNDERLINE_BLUE "\033[4;34m"
#define UNDERLINE_MAGENTA "\033[4;35m"
#define UNDERLINE_CYAN "\033[4;36m"
#define UNDERLINE_WHITE "\033[4;37m"

// Background
#define BG_BLACK "\033[40m"
#define BG_RED "\033[41m"
#define BG_GREEN "\033[42m"
#define BG_YELLOW "\033[43m"
#define BG_BLUE "\033[44m"
#define BG_MAGENTA "\033[45m"
#define BG_CYAN "\033[46m"
#define BG_WHITE "\033[47m"

// Bright Intensity
#define B_BLACK "\033[0;90m"
#define B_RED "\033[0;91m"
#define B_GREEN "\033[0;92m"
#define B_YELLOW "\033[0;93m"
#define B_BLUE "\033[0;94m"
#define B_MAGENTA "\033[0;95m"
#define B_CYAN "\033[0;96m"
#define B_WHITE "\033[0;97m"

// Bold High Intensity
#define BOLD_HI_BLACK "\033[1;90m"
#define BOLD_HI_RED "\033[1;91m"
#define BOLD_HI_GREEN "\033[1;92m"
#define BOLD_HI_YELLOW "\033[1;93m"
#define BOLD_HI_BLUE "\033[1;94m"
#define BOLD_HI_MAGENTA "\033[1;95m"
#define BOLD_HI_CYAN "\033[1;96m"
#define BOLD_HI_WHITE "\033[1;97m"

// High Intensity backgrounds
#define BG_HI_BLACK "\033[0;100m"
#define BG_HI_RED "\033[0;101m"
#define BG_HI_GREEN "\033[0;102m"
#define BG_HI_YELLOW "\033[0;103m"
#define BG_HI_BLUE "\033[0;104m"
#define BG_HI_MAGENTA "\033[0;105m"
#define BG_HI_CYAN "\033[0;106m"
#define BG_HI_WHITE "\033[0;107m"

#define HIDE_CURSOR "\033[?25l"
#define SHOW_CURSOR "\033[?25h"

#define MOVE_CURSOR_UP "\033[A"
#define MOVE_CURSOR_DOWN "\033[B"
#define MOVE_CURSOR_RIGHT "\033[C"
#define MOVE_CURSOR_LEFT "\033[D"

#define MOVE_CURSOR_LAST_LINE "\033[1000E"
#define MOVE_CURSOR_FIRST_LINE "\033[1000F"
#define MOVE_CURSOR_LINE_START "\033[1G"

#define CLEAR_LINE "\033[2K"
#define CLEAR_FROM_CURSOR "\033[0J"
#define CLEAR_TO_CURSOR "\033[1J"

#define CLEAR_SCREEN "\033[2J"
#define CLEAR_SCREEN_DOWN "\033[J"
#define CLEAR_SCREEN_UP "\033[1J"

#define SCROLL_UP "\033[S"
#define SCROLL_DOWN "\033[T"

#endif // TERMINAL_COLORS_H
