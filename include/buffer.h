#ifndef BUFFER_H
#define BUFFER_H

#include "rope.h"
#include <limits.h>

typedef enum
{
  MODE_NORMAL,
  MODE_INSERT,
  MODE_COMMAND,
} EditorMode;

#define CMDBUF_MAX 128
#define STATUSMSG_MAX 256

typedef struct
{
  Rope *rope;
  int cursor;
  int saved_col; // sticky column for up/down navigation
  int rowoff;    // row offset for scrolling
  int cursor_cx; // cached visual column (invalidated on mutation)
  int cursor_cy; // cached visual row (invalidated on mutation)
  EditorMode mode;
  char cmdbuf[CMDBUF_MAX];       // current command line input
  int cmdlen;                    // length of command so far
  char statusmsg[STATUSMSG_MAX]; // message shown after command execution
  char *filename;                // heap-allocated current file path (NULL if unsaved)
  int dirty;                     // 1 if unsaved changes exist
} Buffer;

void buffer_init(Buffer *b);
void buffer_free(Buffer *b);

void buffer_set_filename(Buffer *b, const char *filename);

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
