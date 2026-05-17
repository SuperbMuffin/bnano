#ifndef TERMINAL_H
#define TERMINAL_H

#include "buffer.h"

extern int term_rows;
extern int term_cols;

extern int resize_pipe[2];

#define KEY_UP 1000
#define KEY_DOWN 1001
#define KEY_RIGHT 1002
#define KEY_LEFT 1003
#define KEY_RESIZE 1004

// Raw mode functions
void terminal_enable_raw_mode(void);
void terminal_disable_raw_mode(void);

// Alt screen buffer functions
void terminal_enable_alt_screen(void);
void terminal_disable_alt_screen(void);

// Screen manipulation functions
void terminal_clear_screen(void);
void terminal_refresh_screen(Buffer *buffer);
void terminal_get_size(int *rows, int *cols);

// Input/output functions
int terminal_read_key(void);
void terminal_write(const char *s);

#endif // TERMINAL_H
