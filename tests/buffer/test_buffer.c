#include "buffer.h"
#include "rope.h"

#include <stdio.h>

/* stub out terminal globals that buffer.c references */
int term_rows = 24;
int term_cols = 80;

static int tests_run = 0;
static int tests_failed = 0;

static void fail(const char *file, int line, const char *msg)
{
  printf("FAIL [%s:%d] %s\n", file, line, msg);
  tests_failed++;
}

#define ASSERT(cond, msg)            \
  do                                 \
  {                                  \
    tests_run++;                     \
    if (!(cond))                     \
      fail(__FILE__, __LINE__, msg); \
  } while (0)
#define ASSERT_EQ_INT(a, b, msg)     \
  do                                 \
  {                                  \
    tests_run++;                     \
    if ((a) != (b))                  \
      fail(__FILE__, __LINE__, msg); \
  } while (0)
#define ASSERT_EQ_CHAR(a, b, msg)    \
  do                                 \
  {                                  \
    tests_run++;                     \
    if ((a) != (b))                  \
      fail(__FILE__, __LINE__, msg); \
  } while (0)

/* -------------------------------------------------------------------------
 * buffer_init
 * ---------------------------------------------------------------------- */

static void test_init(void)
{
  Buffer b;
  buffer_init(&b);
  ASSERT_EQ_INT(b.cursor, 0, "init: cursor at 0");
  ASSERT_EQ_INT(b.rowoff, 0, "init: rowoff at 0");
  ASSERT_EQ_INT(buffer_length(&b), 0, "init: length 0");
  rope_free(b.rope);
}

/* -------------------------------------------------------------------------
 * buffer_insert_char / buffer_length
 * ---------------------------------------------------------------------- */

static void test_insert_char(void)
{
  Buffer b;
  buffer_init(&b);
  buffer_insert_char(&b, 'h');
  buffer_insert_char(&b, 'i');
  ASSERT_EQ_INT(buffer_length(&b), 2, "insert: length 2");
  ASSERT_EQ_INT(b.cursor, 2, "insert: cursor at 2");
  ASSERT_EQ_CHAR(buffer_get_char(&b, 0), 'h', "insert: char 0");
  ASSERT_EQ_CHAR(buffer_get_char(&b, 1), 'i', "insert: char 1");
  rope_free(b.rope);
}

static void test_insert_newline(void)
{
  Buffer b;
  buffer_init(&b);
  buffer_insert_char(&b, 'a');
  buffer_insert_char(&b, '\n');
  buffer_insert_char(&b, 'b');
  ASSERT_EQ_INT(buffer_length(&b), 3, "insert newline: length");
  ASSERT_EQ_CHAR(buffer_get_char(&b, 1), '\n', "insert newline: newline char");
  rope_free(b.rope);
}

/* -------------------------------------------------------------------------
 * buffer_delete_char
 * ---------------------------------------------------------------------- */

static void test_delete_char(void)
{
  Buffer b;
  buffer_init(&b);
  buffer_insert_char(&b, 'a');
  buffer_insert_char(&b, 'b');
  buffer_insert_char(&b, 'c');
  buffer_delete_char(&b);
  ASSERT_EQ_INT(buffer_length(&b), 2, "delete: length 2");
  ASSERT_EQ_INT(b.cursor, 2, "delete: cursor at 2");
  ASSERT_EQ_CHAR(buffer_get_char(&b, 1), 'b', "delete: last char is b");
  rope_free(b.rope);
}

static void test_delete_at_start(void)
{
  Buffer b;
  buffer_init(&b);
  buffer_delete_char(&b); /* should be a no-op */
  ASSERT_EQ_INT(buffer_length(&b), 0, "delete at start: no-op");
  ASSERT_EQ_INT(b.cursor, 0, "delete at start: cursor still 0");
  rope_free(b.rope);
}

/* -------------------------------------------------------------------------
 * cursor_line / cursor_col
 * ---------------------------------------------------------------------- */

static void test_cursor_line_single_line(void)
{
  Buffer b;
  buffer_init(&b);
  buffer_insert_char(&b, 'a');
  buffer_insert_char(&b, 'b');
  buffer_insert_char(&b, 'c');
  ASSERT_EQ_INT(cursor_line(&b), 0, "cursor line: single line");
  ASSERT_EQ_INT(cursor_col(&b), 3, "cursor col: 3 chars in");
  rope_free(b.rope);
}

static void test_cursor_line_multiline(void)
{
  Buffer b;
  buffer_init(&b);
  buffer_insert_char(&b, 'a');
  buffer_insert_char(&b, '\n');
  buffer_insert_char(&b, 'b');
  ASSERT_EQ_INT(cursor_line(&b), 1, "cursor line: second line");
  ASSERT_EQ_INT(cursor_col(&b), 1, "cursor col: 1 char into second line");
  rope_free(b.rope);
}

static void test_cursor_col_resets_after_newline(void)
{
  Buffer b;
  buffer_init(&b);
  buffer_insert_char(&b, 'a');
  buffer_insert_char(&b, 'b');
  buffer_insert_char(&b, '\n');
  ASSERT_EQ_INT(cursor_col(&b), 0, "cursor col: resets to 0 after newline");
  rope_free(b.rope);
}

/* -------------------------------------------------------------------------
 * buffer_move_cursor
 * ---------------------------------------------------------------------- */

static void test_move_cursor_left_right(void)
{
  Buffer b;
  buffer_init(&b);
  buffer_insert_char(&b, 'a');
  buffer_insert_char(&b, 'b');
  buffer_insert_char(&b, 'c');
  buffer_move_cursor(&b, -1, 0);
  ASSERT_EQ_INT(b.cursor, 2, "move left: cursor at 2");
  buffer_move_cursor(&b, 1, 0);
  ASSERT_EQ_INT(b.cursor, 3, "move right: cursor at 3");
  rope_free(b.rope);
}

static void test_move_cursor_clamps_left(void)
{
  Buffer b;
  buffer_init(&b);
  buffer_insert_char(&b, 'a');
  buffer_move_cursor(&b, -5, 0);
  ASSERT_EQ_INT(b.cursor, 0, "move left clamp: cursor at 0");
  rope_free(b.rope);
}

static void test_move_cursor_clamps_right(void)
{
  Buffer b;
  buffer_init(&b);
  buffer_insert_char(&b, 'a');
  buffer_move_cursor(&b, 99, 0);
  ASSERT_EQ_INT(b.cursor, 1, "move right clamp: cursor at length");
  rope_free(b.rope);
}

static void test_move_cursor_up_down(void)
{
  Buffer b;
  buffer_init(&b);
  /* "abc\ndef" */
  buffer_insert_char(&b, 'a');
  buffer_insert_char(&b, 'b');
  buffer_insert_char(&b, 'c');
  buffer_insert_char(&b, '\n');
  buffer_insert_char(&b, 'd');
  buffer_insert_char(&b, 'e');
  buffer_insert_char(&b, 'f');
  /* cursor is at end of line 1, col 3 */
  ASSERT_EQ_INT(cursor_line(&b), 1, "setup: on line 1");
  buffer_move_cursor(&b, 0, -1);
  ASSERT_EQ_INT(cursor_line(&b), 0, "move up: on line 0");
  ASSERT_EQ_INT(cursor_col(&b), 3, "move up: sticky col preserved");
  rope_free(b.rope);
}

static void test_move_cursor_up_short_line(void)
{
  Buffer b;
  buffer_init(&b);
  /* "ab\ncdef" — move up from col 3 to a 2-char line */
  buffer_insert_char(&b, 'a');
  buffer_insert_char(&b, 'b');
  buffer_insert_char(&b, '\n');
  buffer_insert_char(&b, 'c');
  buffer_insert_char(&b, 'd');
  buffer_insert_char(&b, 'e');
  buffer_insert_char(&b, 'f');
  /* cursor at end of "cdef", col 4 */
  buffer_move_cursor(&b, 0, -1);
  /* line 0 is "ab" (len 2), cursor should clamp to col 2 */
  ASSERT_EQ_INT(cursor_col(&b), 2, "move up short line: col clamped");
  rope_free(b.rope);
}

/* -------------------------------------------------------------------------
 * buffer_scroll
 * ---------------------------------------------------------------------- */

static void test_scroll_down(void)
{
  Buffer b;
  buffer_init(&b);
  /* insert enough lines to force a scroll */
  for (int i = 0; i < term_rows + 5; i++)
  {
    buffer_insert_char(&b, 'a');
    buffer_insert_char(&b, '\n');
  }
  buffer_scroll(&b);
  ASSERT(b.rowoff > 0, "scroll down: rowoff advanced");
  rope_free(b.rope);
}

static void test_scroll_up(void)
{
  Buffer b;
  buffer_init(&b);
  for (int i = 0; i < term_rows + 5; i++)
  {
    buffer_insert_char(&b, 'a');
    buffer_insert_char(&b, '\n');
  }
  buffer_scroll(&b);
  int rowoff_after_down = b.rowoff;
  /* move cursor back to top */
  for (int i = 0; i < term_rows + 5; i++)
    buffer_move_cursor(&b, 0, -1);
  buffer_scroll(&b);
  ASSERT(b.rowoff < rowoff_after_down, "scroll up: rowoff decreased");
  ASSERT_EQ_INT(b.rowoff, 0, "scroll up: rowoff back to 0");
  rope_free(b.rope);
}

/* -------------------------------------------------------------------------
 * buffer_get_char bounds
 * ---------------------------------------------------------------------- */

static void test_get_char_out_of_bounds(void)
{
  Buffer b;
  buffer_init(&b);
  buffer_insert_char(&b, 'x');
  ASSERT_EQ_CHAR(buffer_get_char(&b, -1), '\0', "get_char: negative index returns null");
  ASSERT_EQ_CHAR(buffer_get_char(&b, 99), '\0', "get_char: past end returns null");
  rope_free(b.rope);
}

/* -------------------------------------------------------------------------
 * main
 * ---------------------------------------------------------------------- */

int main(void)
{
  test_init();

  test_insert_char();
  test_insert_newline();

  test_delete_char();
  test_delete_at_start();

  test_cursor_line_single_line();
  test_cursor_line_multiline();
  test_cursor_col_resets_after_newline();

  test_move_cursor_left_right();
  test_move_cursor_clamps_left();
  test_move_cursor_clamps_right();
  test_move_cursor_up_down();
  test_move_cursor_up_short_line();

  test_scroll_down();
  test_scroll_up();

  test_get_char_out_of_bounds();

  printf("\n%d/%d tests passed\n", tests_run - tests_failed, tests_run);
  return tests_failed > 0 ? 1 : 0;
}
