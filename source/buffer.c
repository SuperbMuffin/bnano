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
  char *text = rope_slice(b->rope, 0, len);
  if (text == NULL)
    return 0;
  int line = 0, col = 0;
  for (int i = 0; i < len; i++)
  {
    if (line == target_line)
    {
      free(text);
      return i;
    }
    if (text[i] == '\n')
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
  free(text);
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

    // Walk the rope once as a flat string to find target_line_start and
    // target_line_end. We also record the cursor cx/cy directly so we
    // don't need a second O(n) recompute pass afterwards.
    char *text = rope_slice(b->rope, 0, len);
    if (text == NULL)
      return;

    int line = 0, col = 0;
    int line_start = 0, next_line_start = len;
    int found_start = 0;

    for (int i = 0; i <= len; i++)
    {
      // Check for line boundary: newline char or wrap or end-of-buffer
      int boundary =
          (i == len) || (text[i] == '\n') || (text[i] != '\n' && col > 0 && col >= term_cols);

      if (!found_start && line == target_line)
      {
        line_start = i;
        found_start = 1;
      }

      if (found_start && boundary)
      {
        // Exclusive end of the target line's content (newline not included).
        next_line_start = i;
        break;
      }

      if (i == len)
        break;

      if (text[i] == '\n')
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

    free(text);

    // Clamp to line length, preserving saved column.
    // next_line_start is already the exclusive end of content (no newline).
    int target_line_len = next_line_start - line_start;

    int new_col = b->saved_col < target_line_len ? b->saved_col : target_line_len;
    b->cursor = line_start + new_col;
    if (b->cursor < 0)
      b->cursor = 0;
    if (b->cursor > len)
      b->cursor = len;

    // cx/cy fall directly out of the scan above — no second pass needed
    b->cursor_cy = target_line;
    b->cursor_cx = new_col;
  }
}

void buffer_delete_char(Buffer *b)
{
  b->dirty = 1;
  if (b->cursor == 0)
    return;
  b->cursor--;
  b->rope = rope_delete(b->rope, b->cursor, b->cursor + 1);

  // The deleted char is gone; use cx position to detect newline/wrap case.
  if (b->cursor_cx == 0)
    buffer_recompute_cursor(b);
  else
    b->cursor_cx--;
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
