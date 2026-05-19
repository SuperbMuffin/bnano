#define _POSIX_C_SOURCE 200809L

#include "buffer.h"
#include "rope.h"
#include "terminal.h"
#include <stdlib.h>
#include <string.h>

void buffer_init(Buffer *b)
{
  b->cursor = 0;
  b->saved_col = 0;
  b->rowoff = 0;
  b->coloff = 0;
  b->cursor_cx = 0;
  b->cursor_cy = 0;
  b->mode = MODE_NORMAL;
  b->cmdlen = 0;
  b->cmdbuf[0] = '\0';
  b->statusmsg[0] = '\0';
  b->filename = NULL;
  b->dirty = 0;
  b->rope = rope_create("");
}

void buffer_insert_char(Buffer *b, char c)
{
  b->dirty = 1;
  char str[2] = {c, '\0'};
  b->rope = rope_insert(b->rope, b->cursor, str);
  b->cursor++;

  if (c == '\n')
  {
    b->cursor_cy++;
    b->cursor_cx = 0;
  }
  else
  {
    b->cursor_cx++;
    if (b->cursor_cx >= term_cols)
    {
      b->cursor_cy++;
      b->cursor_cx = 0;
    }
  }
  b->saved_col = b->cursor_cx;
}

int buffer_visual_line_start(Buffer *b, int target_line)
{
  int len = rope_length(b->rope);
  int line = 0;
  int col = 0;
  for (int i = 0; i < len; i++)
  {
    if (line == target_line)
      return i;
    char c = rope_index(b->rope, i);
    if (c == '\n')
    {
      line++;
      col = 0;
    }
    else
    {
      col++;
      if (col >= term_cols)
      {
        line++;
        col = 0;
      }
    }
  }
  return len;
}

// Recompute cx/cy from scratch — call after any jump that changes cursor position non-incrementally
static void buffer_recompute_cursor(Buffer *b)
{
  int line = 0, col = 0;
  for (int i = 0; i < b->cursor; i++)
  {
    if (rope_index(b->rope, i) == '\n')
    {
      line++;
      col = 0;
    }
    else
    {
      col++;
      if (col >= term_cols)
      {
        line++;
        col = 0;
      }
    }
  }
  b->cursor_cy = line;
  b->cursor_cx = col;
}

// cursor_line and cursor_col just return cached values
int cursor_line(Buffer *b)
{
  return b->cursor_cy;
}

int cursor_col(Buffer *b)
{
  return b->cursor_cx;
}

char buffer_get_char(Buffer *b, int index)
{
  if (index < 0 || index >= rope_length(b->rope))
    return '\0';
  return rope_index(b->rope, index);
}

int buffer_length(Buffer *b)
{
  return rope_length(b->rope);
}

void buffer_move_cursor(Buffer *b, int dx, int dy)
{
  int len = rope_length(b->rope);

  if (dy == 0)
  {
    b->cursor += dx;
    if (b->cursor < 0)
      b->cursor = 0;
    if (b->cursor > len)
      b->cursor = len;
    buffer_recompute_cursor(b);
    b->saved_col = b->cursor_cx;
  }
  else
  {
    int target_line = b->cursor_cy + dy;
    if (target_line < 0)
      target_line = 0;

    int line_start = buffer_visual_line_start(b, target_line);
    int next_line_start = buffer_visual_line_start(b, target_line + 1);

    int target_line_len = next_line_start - line_start;
    if (target_line_len > 0 && rope_index(b->rope, line_start + target_line_len - 1) == '\n')
      target_line_len--;

    int col = b->saved_col < target_line_len ? b->saved_col : target_line_len;
    b->cursor = line_start + col;
    if (b->cursor < 0)
      b->cursor = 0;
    if (b->cursor > len)
      b->cursor = len;
    buffer_recompute_cursor(b);
  }
}

void buffer_delete_char(Buffer *b)
{
  b->dirty = 1;
  if (b->cursor == 0)
    return;
  b->cursor--;
  b->rope = rope_delete(b->rope, b->cursor, b->cursor + 1);
  buffer_recompute_cursor(b);
  b->saved_col = b->cursor_cx;
}

void buffer_scroll(Buffer *b)
{
  int cy = cursor_line(b);

  if (cy < b->rowoff)
  {
    b->rowoff = cy;
  }
  if (cy >= b->rowoff + term_rows - 1)
  {
    b->rowoff = cy - term_rows + 2;
  }
}

void buffer_free(Buffer *b)
{
  free(b->filename);
  b->filename = NULL;
}

void buffer_set_filename(Buffer *b, const char *name)
{
  free(b->filename);
  b->filename = name ? strdup(name) : NULL;
}
