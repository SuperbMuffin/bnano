#include "buffer.h"
#include "rope.h"
#include "terminal.h"

void buffer_init(Buffer *b)
{
  b->cursor = 0;
  b->saved_col = 0;
  b->rowoff = 0;
  b->coloff = 0;
  b->rope = rope_create("");
}

void buffer_insert_char(Buffer *b, char c)
{
  char str[2] = {c, '\0'};
  b->rope = rope_insert(b->rope, b->cursor, str);
  b->cursor++;
  b->saved_col = cursor_col(b);
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

// cursor_line and cursor_col are visual — used by terminal for screen positioning
int cursor_line(Buffer *b)
{
  int line = 0;
  int col = 0;
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
  return line;
}

int cursor_col(Buffer *b)
{
  int col = 0;
  int line_start = 0;
  for (int i = 0; i < b->cursor; i++)
  {
    if (rope_index(b->rope, i) == '\n')
    {
      col = 0;
      line_start = i + 1;
    }
    else
    {
      col++;
      if (col >= term_cols)
      {
        col = 0;
        line_start = i + 1;
      }
    }
  }
  return b->cursor - line_start;
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
    b->saved_col = cursor_col(b);
  }
  else
  {
    int target_line = cursor_line(b) + dy;
    if (target_line < 0)
      target_line = 0;

    int line_start = buffer_visual_line_start(b, target_line);
    int next_line_start = buffer_visual_line_start(b, target_line + 1);

    // length of target visual line, excluding trailing newline if present
    int target_line_len = next_line_start - line_start;
    if (target_line_len > 0 && rope_index(b->rope, line_start + target_line_len - 1) == '\n')
      target_line_len--;

    int col = b->saved_col < target_line_len ? b->saved_col : target_line_len;
    b->cursor = line_start + col;
  }

  if (b->cursor < 0)
    b->cursor = 0;
  if (b->cursor > len)
    b->cursor = len;
}

void buffer_set_char(Buffer *b, int x, int y, char c)
{
  if (x < 0 || x >= term_cols || y < 0 || y >= term_rows)
    return;
  int index = buffer_visual_line_start(b, y) + x;
  if (index >= rope_length(b->rope))
    return;
  b->rope = rope_delete(b->rope, index, index + 1);
  b->rope = rope_insert(b->rope, index, (char[]){c, '\0'});
}

void buffer_delete_char(Buffer *b)
{
  if (b->cursor == 0)
    return;
  b->cursor--;
  b->rope = rope_delete(b->rope, b->cursor, b->cursor + 1);
  b->saved_col = cursor_col(b);
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
