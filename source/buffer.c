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

// Recompute cx/cy from scratch — only call after non-incremental jumps (file open, goto, etc.)
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

void buffer_insert_char(Buffer *b, char c)
{
  b->dirty = 1;
  char str[2] = {c, '\0'};
  b->rope = rope_insert(b->rope, b->cursor, str);
  b->cursor++;

  // Incremental update: just advance cx/cy by one character
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
    // Incremental horizontal move: update cx/cy by walking only the stepped characters
    int new_cursor = b->cursor + dx;
    if (new_cursor < 0)
      new_cursor = 0;
    if (new_cursor > len)
      new_cursor = len;

    // Only do incremental update for single-step moves; fall back for big jumps (0/$)
    if (new_cursor == b->cursor + 1)
    {
      char stepped = rope_index(b->rope, b->cursor);
      b->cursor = new_cursor;
      if (stepped == '\n')
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
    }
    else if (new_cursor == b->cursor - 1)
    {
      b->cursor = new_cursor;
      char prev = rope_index(b->rope, b->cursor);
      if (prev == '\n' || b->cursor_cx == 0)
      {
        // Stepped back over a newline or wrap — need full recompute
        buffer_recompute_cursor(b);
      }
      else
      {
        b->cursor_cx--;
      }
    }
    else
    {
      b->cursor = new_cursor;
      buffer_recompute_cursor(b);
    }
    b->saved_col = b->cursor_cx;
  }
  else
  {
    int target_line = b->cursor_cy + dy;
    if (target_line < 0)
      target_line = 0;

    // For dy=1: we know the current line starts at (cursor - cursor_cx), so the target
    // line starts where the current line ends — only need one scan for target+1.
    // For dy=-1: we know the current line start is (cursor - cursor_cx), which equals
    // next_line_start for the target — only need one scan for target.
    int line_start, next_line_start;
    if (dy == 1)
    {
      line_start = b->cursor - b->cursor_cx; // current line start = target line start when dy=1
      next_line_start = buffer_visual_line_start(b, target_line + 1);
    }
    else if (dy == -1)
    {
      line_start = buffer_visual_line_start(b, target_line);
      next_line_start = b->cursor - b->cursor_cx; // current line start = next_line_start for target
    }
    else
    {
      line_start = buffer_visual_line_start(b, target_line);
      next_line_start = buffer_visual_line_start(b, target_line + 1);
    }

    int target_line_len = next_line_start - line_start;
    if (target_line_len > 0 && rope_index(b->rope, line_start + target_line_len - 1) == '\n')
      target_line_len--;

    int col = b->saved_col < target_line_len ? b->saved_col : target_line_len;
    b->cursor = line_start + col;
    if (b->cursor < 0)
      b->cursor = 0;
    if (b->cursor > len)
      b->cursor = len;
    // cx/cy must be recomputed after vertical jump — no cheap incremental path here
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

  // The deleted char was at b->cursor (before decrement). Stepping back over it:
  char deleted = rope_index(b->rope, b->cursor); // char now at that position is the one after
  // We need the char that WAS there — but it's gone. Use cx position to detect newline case.
  if (b->cursor_cx == 0)
  {
    // We stepped back over a newline or a wrap boundary — full recompute
    buffer_recompute_cursor(b);
  }
  else
  {
    b->cursor_cx--;
    (void) deleted;
  }
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
  rope_free(b->rope);
  b->rope = NULL;
  free(b->filename);
  b->filename = NULL;
}

void buffer_set_filename(Buffer *b, const char *name)
{
  free(b->filename);
  b->filename = name ? strdup(name) : NULL;
}
