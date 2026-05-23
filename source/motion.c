#include "motion.h"
#include "buffer.h"
#include "rope.h"

#include <ctype.h>

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
  int i = b->cursor;

  if (i >= len)
    return;

  // Skip current word chars or punctuation run
  if (is_word_char(rope_index(b->rope, i)))
    while (i < len && is_word_char(rope_index(b->rope, i)))
      i++;
  else if (!isspace((unsigned char) rope_index(b->rope, i)))
    while (i < len && !is_word_char(rope_index(b->rope, i)) &&
           !isspace((unsigned char) rope_index(b->rope, i)))
      i++;

  // Skip whitespace
  while (i < len && isspace((unsigned char) rope_index(b->rope, i)))
    i++;

  buffer_move_cursor(b, i - b->cursor, 0);
}

void motion_word_back(Buffer *b)
{
  int i = b->cursor;

  if (i == 0)
    return;

  // Step back past whitespace
  while (i > 0 && isspace((unsigned char) rope_index(b->rope, i - 1)))
    i--;

  if (i == 0)
  {
    buffer_move_cursor(b, -b->cursor, 0);
    return;
  }

  // Step back through word or punctuation run
  if (is_word_char(rope_index(b->rope, i - 1)))
    while (i > 0 && is_word_char(rope_index(b->rope, i - 1)))
      i--;
  else
    while (i > 0 && !is_word_char(rope_index(b->rope, i - 1)) &&
           !isspace((unsigned char) rope_index(b->rope, i - 1)))
      i--;

  buffer_move_cursor(b, i - b->cursor, 0); // delta is negative
}

void motion_word_end(Buffer *b)
{
  int len = buffer_length(b);
  int i = b->cursor;

  if (i >= len)
    return;

  // Step forward one to get off the current position
  if (i + 1 < len)
    i++;

  // Skip whitespace
  while (i < len && isspace((unsigned char) rope_index(b->rope, i)))
    i++;

  // Advance to end of word or punctuation run
  if (is_word_char(rope_index(b->rope, i)))
    while (i + 1 < len && is_word_char(rope_index(b->rope, i + 1)))
      i++;
  else
    while (i + 1 < len && !is_word_char(rope_index(b->rope, i + 1)) &&
           !isspace((unsigned char) rope_index(b->rope, i + 1)))
      i++;

  buffer_move_cursor(b, i - b->cursor, 0);
}

// ---------------------------------------------------------------------------
// File motions
// ---------------------------------------------------------------------------

void motion_file_top(Buffer *b)
{
  // Jump to absolute start
  buffer_move_cursor(b, -b->cursor, 0);
}

void motion_file_end(Buffer *b)
{
  int len = buffer_length(b);
  // Jump to last line start
  buffer_move_cursor(b, len - b->cursor, 0);
  // Walk back to line start
  while (b->cursor > 0 && buffer_get_char(b, b->cursor - 1) != '\n')
    buffer_move_cursor(b, -1, 0);
}
