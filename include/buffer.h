#ifndef BUFFER_H
#define BUFFER_H

#include "rope.h"

typedef struct
{
  Rope *rope;
  int cursor;
  int saved_col; // sticky column for up/down navigation
  int rowoff;    // row offset for scrolling
  int coloff;    // column offset for scrolling
  int cursor_cx; // cached visual column (invalidated on mutation)
  int cursor_cy; // cached visual row (invalidated on mutation)
} Buffer;

void buffer_init(Buffer *b);

void buffer_insert_char(Buffer *b, char c);
void buffer_move_cursor(Buffer *b, int dx, int dy);
void buffer_delete_char(Buffer *b);

void buffer_scroll(Buffer *b);

void buffer_set_char(Buffer *b, int x, int y, char c);
char buffer_get_char(Buffer *b, int index);

int cursor_line(Buffer *b);
int cursor_col(Buffer *b);

int buffer_length(Buffer *b);
int buffer_visual_line_start(Buffer *b, int target_line);

#endif // BUFFER_H
