#include "test_common.h"
#include "buffer.h"
#include "rope.h"

static void test_init(void)
{
  Buffer b;
  buffer_init(&b);
  ASSERT_EQ_INT(b.cursor, 0, "init: cursor at 0");
  ASSERT_EQ_INT(b.rowoff, 0, "init: rowoff at 0");
  ASSERT_EQ_INT(buffer_length(&b), 0, "init: length 0");
  buffer_free(&b);
}

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
  buffer_free(&b);
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
  buffer_free(&b);
}

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
  buffer_free(&b);
}

static void test_delete_at_start(void)
{
  Buffer b;
  buffer_init(&b);
  buffer_delete_char(&b);
  ASSERT_EQ_INT(buffer_length(&b), 0, "delete at start: no-op");
  ASSERT_EQ_INT(b.cursor, 0, "delete at start: cursor still 0");
  buffer_free(&b);
}

static void test_cursor_line_single_line(void)
{
  Buffer b;
  buffer_init(&b);
  buffer_insert_char(&b, 'a');
  buffer_insert_char(&b, 'b');
  buffer_insert_char(&b, 'c');
  ASSERT_EQ_INT(cursor_line(&b), 0, "cursor line: single line");
  ASSERT_EQ_INT(cursor_col(&b), 3, "cursor col: 3 chars in");
  buffer_free(&b);
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
  buffer_free(&b);
}

static void test_cursor_col_resets_after_newline(void)
{
  Buffer b;
  buffer_init(&b);
  buffer_insert_char(&b, 'a');
  buffer_insert_char(&b, 'b');
  buffer_insert_char(&b, '\n');
  ASSERT_EQ_INT(cursor_col(&b), 0, "cursor col: resets to 0 after newline");
  buffer_free(&b);
}

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
  buffer_free(&b);
}

static void test_move_cursor_clamps_left(void)
{
  Buffer b;
  buffer_init(&b);
  buffer_insert_char(&b, 'a');
  buffer_move_cursor(&b, -5, 0);
  ASSERT_EQ_INT(b.cursor, 0, "move left clamp: cursor at 0");
  buffer_free(&b);
}

static void test_move_cursor_clamps_right(void)
{
  Buffer b;
  buffer_init(&b);
  buffer_insert_char(&b, 'a');
  buffer_move_cursor(&b, 99, 0);
  ASSERT_EQ_INT(b.cursor, 1, "move right clamp: cursor at length");
  buffer_free(&b);
}

static void test_move_cursor_up_down(void)
{
  Buffer b;
  buffer_init(&b);
  buffer_insert_char(&b, 'a');
  buffer_insert_char(&b, 'b');
  buffer_insert_char(&b, 'c');
  buffer_insert_char(&b, '\n');
  buffer_insert_char(&b, 'd');
  buffer_insert_char(&b, 'e');
  buffer_insert_char(&b, 'f');
  ASSERT_EQ_INT(cursor_line(&b), 1, "setup: on line 1");
  buffer_move_cursor(&b, 0, -1);
  ASSERT_EQ_INT(cursor_line(&b), 0, "move up: on line 0");
  ASSERT_EQ_INT(cursor_col(&b), 3, "move up: sticky col preserved");
  buffer_free(&b);
}

static void test_move_cursor_up_short_line(void)
{
  Buffer b;
  buffer_init(&b);
  buffer_insert_char(&b, 'a');
  buffer_insert_char(&b, 'b');
  buffer_insert_char(&b, '\n');
  buffer_insert_char(&b, 'c');
  buffer_insert_char(&b, 'd');
  buffer_insert_char(&b, 'e');
  buffer_insert_char(&b, 'f');
  buffer_move_cursor(&b, 0, -1);
  ASSERT_EQ_INT(cursor_col(&b), 2, "move up short line: col clamped");
  buffer_free(&b);
}

static void test_scroll_down(void)
{
  Buffer b;
  buffer_init(&b);
  for (int i = 0; i < term_rows + 5; i++)
  {
    buffer_insert_char(&b, 'a');
    buffer_insert_char(&b, '\n');
  }
  buffer_scroll(&b);
  ASSERT(b.rowoff > 0, "scroll down: rowoff advanced");
  buffer_free(&b);
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
  for (int i = 0; i < term_rows + 5; i++)
    buffer_move_cursor(&b, 0, -1);
  buffer_scroll(&b);
  ASSERT(b.rowoff < rowoff_after_down, "scroll up: rowoff decreased");
  ASSERT_EQ_INT(b.rowoff, 0, "scroll up: rowoff back to 0");
  buffer_free(&b);
}

static void test_get_char_out_of_bounds(void)
{
  Buffer b;
  buffer_init(&b);
  buffer_insert_char(&b, 'x');
  ASSERT_EQ_CHAR(buffer_get_char(&b, -1), '\0', "get_char: negative index returns null");
  ASSERT_EQ_CHAR(buffer_get_char(&b, 99), '\0', "get_char: past end returns null");
  buffer_free(&b);
}

int main(void)
{
  run_test("init",                       test_init);
  run_test("insert_char",                test_insert_char);
  run_test("insert_newline",             test_insert_newline);
  run_test("delete_char",                test_delete_char);
  run_test("delete_at_start",            test_delete_at_start);
  run_test("cursor_line_single_line",    test_cursor_line_single_line);
  run_test("cursor_line_multiline",      test_cursor_line_multiline);
  run_test("cursor_col_resets_newline",  test_cursor_col_resets_after_newline);
  run_test("move_cursor_left_right",     test_move_cursor_left_right);
  run_test("move_cursor_clamps_left",    test_move_cursor_clamps_left);
  run_test("move_cursor_clamps_right",   test_move_cursor_clamps_right);
  run_test("move_cursor_up_down",        test_move_cursor_up_down);
  run_test("move_cursor_up_short_line",  test_move_cursor_up_short_line);
  run_test("scroll_down",                test_scroll_down);
  run_test("scroll_up",                  test_scroll_up);
  run_test("get_char_out_of_bounds",     test_get_char_out_of_bounds);

  print_summary();
  return tc_suite_failed() > 0 ? 1 : 0;
}
