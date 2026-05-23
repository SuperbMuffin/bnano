#include "motion.h"
#include "buffer.h"
#include "rope.h"

#include <ctype.h>
#include <stdlib.h>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static int is_word_char(char c)
{
  return isalnum((unsigned char) c) || c == '_';
}

// ---------------------------------------------------------------------------
// Word motions
// ---------------------------------------------------------------------------

void motion_word_next(Buffer *b)
{
  int len = buffer_length(b);
  if (b->cursor >= len)
    return;

  // Flatten the tail once so we walk a char* instead of O(log n) per step.
  char *text = rope_slice(b->rope, b->cursor, len);
  if (text == NULL)
    return;

  int i = 0;
  int tail = len - b->cursor;

  // Skip current word chars or punctuation run
  if (is_word_char(text[i]))
    while (i < tail && is_word_char(text[i]))
      i++;
  else if (!isspace((unsigned char) text[i]))
    while (i < tail && !is_word_char(text[i]) && !isspace((unsigned char) text[i]))
      i++;

  // Skip whitespace
  while (i < tail && isspace((unsigned char) text[i]))
    i++;

  free(text);
  buffer_move_cursor(b, i, 0);
}

void motion_word_back(Buffer *b)
{
  if (b->cursor == 0)
    return;

  // Flatten everything up to the cursor so we can walk backwards cheaply.
  char *text = rope_slice(b->rope, 0, b->cursor);
  if (text == NULL)
    return;

  int i = b->cursor;

  // Step back past whitespace
  while (i > 0 && isspace((unsigned char) text[i - 1]))
    i--;

  if (i > 0)
  {
    // Step back through word or punctuation run
    if (is_word_char(text[i - 1]))
      while (i > 0 && is_word_char(text[i - 1]))
        i--;
    else
      while (i > 0 && !is_word_char(text[i - 1]) && !isspace((unsigned char) text[i - 1]))
        i--;
  }

  free(text);
  buffer_move_cursor(b, i - b->cursor, 0); // delta is negative
}

void motion_word_end(Buffer *b)
{
  int len = buffer_length(b);
  if (b->cursor >= len)
    return;

  char *text = rope_slice(b->rope, b->cursor, len);
  if (text == NULL)
    return;

  int tail = len - b->cursor;
  int i = 0;

  // Step forward one to get off the current position
  if (i + 1 < tail)
    i++;

  // Skip whitespace
  while (i < tail && isspace((unsigned char) text[i]))
    i++;

  // Advance to end of word or punctuation run
  if (is_word_char(text[i]))
    while (i + 1 < tail && is_word_char(text[i + 1]))
      i++;
  else
    while (i + 1 < tail && !is_word_char(text[i + 1]) && !isspace((unsigned char) text[i + 1]))
      i++;

  free(text);
  buffer_move_cursor(b, i, 0);
}

// ---------------------------------------------------------------------------
// File motions
// ---------------------------------------------------------------------------

void motion_file_top(Buffer *b)
{
  buffer_move_cursor(b, -b->cursor, 0);
}

void motion_file_end(Buffer *b)
{
  int len = buffer_length(b);
  buffer_move_cursor(b, len - b->cursor, 0);
  while (b->cursor > 0 && buffer_get_char(b, b->cursor - 1) != '\n')
    buffer_move_cursor(b, -1, 0);
}
