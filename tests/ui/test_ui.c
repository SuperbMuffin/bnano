#include "test_common.h"
#include "ui.h"
#include "buffer.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* -------------------------------------------------------------------------
 * Capture helpers
 *
 * capture_render() redirects STDOUT_FILENO into a pipe, calls ui_render,
 * then reads everything back into a heap buffer.  Caller must free().
 * ------------------------------------------------------------------------- */

static char *capture_render(Buffer *b)
{
  int pipefd[2];
  if (pipe(pipefd) == -1)
    return NULL;

  /* redirect stdout to write-end of pipe */
  int saved_stdout = dup(STDOUT_FILENO);
  dup2(pipefd[1], STDOUT_FILENO);
  close(pipefd[1]);

  ui_render(b);
  fflush(stdout);

  /* restore stdout */
  dup2(saved_stdout, STDOUT_FILENO);
  close(saved_stdout);

  /* drain the pipe */
  size_t cap = 4096, len = 0;
  char  *buf = malloc(cap);
  if (!buf) { close(pipefd[0]); return NULL; }

  ssize_t n;
  while ((n = read(pipefd[0], buf + len, cap - len - 1)) > 0)
  {
    len += (size_t) n;
    if (len + 1 >= cap)
    {
      cap *= 2;
      char *tmp = realloc(buf, cap);
      if (!tmp) { free(buf); close(pipefd[0]); return NULL; }
      buf = tmp;
    }
  }
  buf[len] = '\0';
  close(pipefd[0]);
  return buf;
}

/* -------------------------------------------------------------------------
 * Tests
 * ------------------------------------------------------------------------- */

/* render produces output at all */
static void test_render_produces_output(void)
{
  Buffer b;
  buffer_init(&b);

  char *out = capture_render(&b);
  ASSERT(out != NULL, "capture returned non-NULL");
  ASSERT(strlen(out) > 0, "render produced non-empty output");

  free(out);
  buffer_free(&b);
}

/* content appears somewhere in the rendered output */
static void test_render_shows_content(void)
{
  Buffer b;
  buffer_init(&b);
  const char *text = "hello";
  for (const char *p = text; *p; p++)
    buffer_insert_char(&b, *p);

  char *out = capture_render(&b);
  ASSERT(out != NULL, "capture returned non-NULL");
  ASSERT(strstr(out, "hello") != NULL, "content visible in render output");

  free(out);
  buffer_free(&b);
}

/* NORMAL mode pill text appears in status bar */
static void test_render_normal_mode_pill(void)
{
  Buffer b;
  buffer_init(&b);
  b.mode = MODE_NORMAL;

  char *out = capture_render(&b);
  ASSERT(out != NULL, "capture returned non-NULL");
  ASSERT(strstr(out, "NORMAL") != NULL, "NORMAL pill present in output");

  free(out);
  buffer_free(&b);
}

/* INSERT mode pill text appears in status bar */
static void test_render_insert_mode_pill(void)
{
  Buffer b;
  buffer_init(&b);
  b.mode = MODE_INSERT;

  char *out = capture_render(&b);
  ASSERT(out != NULL, "capture returned non-NULL");
  ASSERT(strstr(out, "INSERT") != NULL, "INSERT pill present in output");

  free(out);
  buffer_free(&b);
}

/* status message appears in the bar */
static void test_render_status_message(void)
{
  Buffer b;
  buffer_init(&b);
  snprintf(b.statusmsg, sizeof(b.statusmsg), "Written: foo.c");

  char *out = capture_render(&b);
  ASSERT(out != NULL, "capture returned non-NULL");
  ASSERT(strstr(out, "Written: foo.c") != NULL, "status message visible in output");

  free(out);
  buffer_free(&b);
}

/* command mode shows the typed command instead of the mode pill */
static void test_render_command_mode(void)
{
  Buffer b;
  buffer_init(&b);
  b.mode = MODE_COMMAND;
  strncpy(b.cmdbuf, "wq", sizeof(b.cmdbuf) - 1);
  b.cmdlen = 2;

  char *out = capture_render(&b);
  ASSERT(out != NULL, "capture returned non-NULL");
  ASSERT(strstr(out, "wq") != NULL,       "command text visible in output");
  ASSERT(strstr(out, "NORMAL") == NULL,   "NORMAL pill absent in command mode");
  ASSERT(strstr(out, "INSERT") == NULL,   "INSERT pill absent in command mode");

  free(out);
  buffer_free(&b);
}

/* multiline content: both lines visible */
static void test_render_multiline(void)
{
  Buffer b;
  buffer_init(&b);
  const char *line1 = "first";
  const char *line2 = "second";
  for (const char *p = line1; *p; p++) buffer_insert_char(&b, *p);
  buffer_insert_char(&b, '\n');
  for (const char *p = line2; *p; p++) buffer_insert_char(&b, *p);

  char *out = capture_render(&b);
  ASSERT(out != NULL, "capture returned non-NULL");
  ASSERT(strstr(out, "first")  != NULL, "first line visible");
  ASSERT(strstr(out, "second") != NULL, "second line visible");

  free(out);
  buffer_free(&b);
}

/* scrolled buffer: only visible lines rendered */
static void test_render_scroll_hides_top(void)
{
  Buffer b;
  buffer_init(&b);

  /* write enough lines that rowoff=1 hides the first */
  for (int i = 0; i < 5; i++)
  {
    buffer_insert_char(&b, '0' + i);
    buffer_insert_char(&b, '\n');
  }
  b.rowoff = 1; /* scroll past line 0 */

  char *out = capture_render(&b);
  ASSERT(out != NULL, "capture returned non-NULL");
  ASSERT(strstr(out, "1") != NULL, "line 1 visible after scroll");

  free(out);
  buffer_free(&b);
}

/* cursor-hide and cursor-show sequences are emitted */
static void test_render_cursor_sequences(void)
{
  Buffer b;
  buffer_init(&b);

  char *out = capture_render(&b);
  ASSERT(out != NULL, "capture returned non-NULL");
  /* hide cursor: ESC[?25l */
  ASSERT(strstr(out, "\x1b[?25l") != NULL, "hide-cursor sequence present");
  /* show cursor: ESC[?25h */
  ASSERT(strstr(out, "\x1b[?25h") != NULL, "show-cursor sequence present");

  free(out);
  buffer_free(&b);
}

/* -------------------------------------------------------------------------
 * Main
 * ------------------------------------------------------------------------- */

int main(void)
{
  run_test("render_produces_output",   test_render_produces_output);
  run_test("render_shows_content",     test_render_shows_content);
  run_test("render_normal_mode_pill",  test_render_normal_mode_pill);
  run_test("render_insert_mode_pill",  test_render_insert_mode_pill);
  run_test("render_status_message",    test_render_status_message);
  run_test("render_command_mode",      test_render_command_mode);
  run_test("render_multiline",         test_render_multiline);
  run_test("render_scroll_hides_top",  test_render_scroll_hides_top);
  run_test("render_cursor_sequences",  test_render_cursor_sequences);

  print_summary();
  return tc_suite_failed() > 0 ? 1 : 0;
}
