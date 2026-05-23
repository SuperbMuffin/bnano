#define _POSIX_C_SOURCE 200809L

#include "ui.h"
#include "buffer.h"
#include "config.h"
#include "rope.h"
#include "terminal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// ---------------------------------------------------------------------------
// Escape sequences
// ---------------------------------------------------------------------------

// Cursor control
#define ESC_HIDE_CURSOR "\x1b[?25l"
#define ESC_SHOW_CURSOR "\x1b[?25h"
#define ESC_CURSOR_HOME "\x1b[H"
#define ESC_CLEAR_LINE "\x1b[K"

// Reset all attributes
#define ESC_RESET "\x1b[0m"

// True color macros  (foreground / background)
// Usage: TC_FG(r,g,b)  TC_BG(r,g,b)  — expand to string literals via snprintf
#define TC_FG(r, g, b) "\x1b[38;2;" #r ";" #g ";" #b "m"
#define TC_BG(r, g, b) "\x1b[48;2;" #r ";" #g ";" #b "m"

// Named colors
#define COLOR_FG_WHITE TC_FG(220, 220, 220)
#define COLOR_FG_BLACK TC_FG(20, 20, 20)
#define COLOR_FG_GUTTER TC_FG(80, 80, 80)
#define COLOR_FG_GUTTER_CUR TC_FG(180, 180, 180)
#define COLOR_BG_DARK TC_BG(24, 24, 24)
#define COLOR_BG_BLUE TC_BG(64, 120, 242)
#define COLOR_BG_GREEN TC_BG(60, 168, 90)

// Composite styles
#define STYLE_RESET ESC_RESET
#define STYLE_BAR ESC_RESET COLOR_BG_DARK COLOR_FG_WHITE
#define STYLE_PILL_NORMAL ESC_RESET COLOR_BG_BLUE COLOR_FG_WHITE
#define STYLE_PILL_INSERT ESC_RESET COLOR_BG_GREEN COLOR_FG_BLACK

// ---------------------------------------------------------------------------
// Append buffer — write-accumulator to avoid many small write() calls
// ---------------------------------------------------------------------------

struct abuf
{
  char *b;
  int len;
};

#define ABUF_INIT {NULL, 0}

static void ab_append(struct abuf *ab, const char *s, int len)
{
  char *new = realloc(ab->b, ab->len + len);
  if (new == NULL)
    return;
  memcpy(&new[ab->len], s, len);
  ab->b = new;
  ab->len += len;
}

#define ab_lit(ab, s) ab_append((ab), (s), sizeof(s) - 1)

static void ab_free(struct abuf *ab)
{
  free(ab->b);
}

// ---------------------------------------------------------------------------
// Gutter helpers
// ---------------------------------------------------------------------------

// Count total logical lines (newline-delimited) in text.
static int count_lines(const char *text, int len)
{
  int lines = 1;
  for (int i = 0; i < len; i++)
    if (text[i] == '\n')
      lines++;
  return lines;
}

// Number of decimal digits needed to represent n (minimum 1).
static int digit_width(int n)
{
  if (n < 10)
    return 1;
  if (n < 100)
    return 2;
  if (n < 1000)
    return 3;
  if (n < 10000)
    return 4;
  return 5;
}

// Gutter width = digit_width(total_lines) + 2 (space + separator "│ ")
// Returns 0 when line numbers are disabled.
static int gutter_width(int total_lines)
{
  if (g_config.line_numbers == LINE_NUMBERS_NONE)
    return 0;
  return digit_width(total_lines) + 2; // " NNN "
}

static void render_gutter(struct abuf *ab, int visual_line, int cur_visual_line, int total_lines,
                          int num_width)
{
  // visual_line and cur_visual_line are 0-based visual (screen) lines.
  // We convert to 1-based logical lines for display.
  int abs_line = visual_line + 1;
  int rel_dist = visual_line - cur_visual_line;
  if (rel_dist < 0)
    rel_dist = -rel_dist;

  char gutter[32];
  int glen;

  int is_current = (visual_line == cur_visual_line);

  if (is_current)
    ab_append(ab, COLOR_FG_GUTTER_CUR, sizeof(COLOR_FG_GUTTER_CUR) - 1);
  else
    ab_append(ab, COLOR_FG_GUTTER, sizeof(COLOR_FG_GUTTER) - 1);

  switch (g_config.line_numbers)
  {
    case LINE_NUMBERS_ABSOLUTE:
      glen = snprintf(gutter, sizeof(gutter), " %*d ", num_width, abs_line);
      break;
    case LINE_NUMBERS_RELATIVE:
      if (is_current)
        glen = snprintf(gutter, sizeof(gutter), " %*d ", num_width, abs_line);
      else
        glen = snprintf(gutter, sizeof(gutter), " %*d ", num_width, rel_dist);
      break;
    case LINE_NUMBERS_BOTH:
      if (is_current)
        glen = snprintf(gutter, sizeof(gutter), " %*d ", num_width, abs_line);
      else
        glen = snprintf(gutter, sizeof(gutter), " %*d ", num_width, rel_dist);
      break;
    default:
      return;
  }

  (void) total_lines;
  ab_append(ab, gutter, glen);
  ab_append(ab, ESC_RESET, sizeof(ESC_RESET) - 1);
}

// ---------------------------------------------------------------------------
// Content area
// ---------------------------------------------------------------------------

static void render_content(struct abuf *ab, Buffer *buffer)
{
  int len = buffer_length(buffer);

  char *text = rope_to_string(buffer->rope);
  if (text == NULL)
    return;

  int total_lines = count_lines(text, len);
  int num_width = digit_width(total_lines);
  int gw = gutter_width(total_lines);
  int content_cols = term_cols - gw;
  if (content_cols < 1)
    content_cols = 1;

  int cur_visual_line = cursor_line(buffer); // absolute visual line of cursor

  int start_index = buffer_visual_line_start_str(text, len, buffer->rowoff);
  int x = 0, y = 0;
  int at_line_start = 1; // track whether we're at the start of a visual line

  for (int i = start_index; i < len && y < term_rows - 1; i++)
  {
    if (at_line_start)
    {
      if (gw > 0)
        render_gutter(ab, buffer->rowoff + y, cur_visual_line, total_lines, num_width);
      at_line_start = 0;
    }

    char c = text[i];
    if (c == '\n')
    {
      ab_lit(ab, ESC_CLEAR_LINE "\r\n");
      x = 0;
      y++;
      at_line_start = 1;
    }
    else
    {
      ab_append(ab, &c, 1);
      x++;
      if (x >= content_cols)
      {
        ab_lit(ab, "\r\n");
        x = 0;
        y++;
        at_line_start = 1;
      }
    }
  }

  // Handle gutter for the last line if it didn't end with \n
  if (at_line_start && y < term_rows - 1 && gw > 0)
    render_gutter(ab, buffer->rowoff + y, cur_visual_line, total_lines, num_width);

  free(text);

  while (y < term_rows - 1)
  {
    ab_lit(ab, ESC_CLEAR_LINE "\r\n");
    y++;
  }
}

// ---------------------------------------------------------------------------
// Status bar
// ---------------------------------------------------------------------------

static void render_statusbar(struct abuf *ab, Buffer *buffer)
{
  if (buffer->mode == MODE_COMMAND)
  {
    ab_lit(ab, STYLE_RESET);
    char cmdline[CMDBUF_MAX + 2];
    int clen = snprintf(cmdline, sizeof(cmdline), ":%s", buffer->cmdbuf);
    ab_append(ab, cmdline, clen);
    for (int i = clen; i < term_cols; i++)
      ab_append(ab, " ", 1);
    // Move cursor to end of typed command
    char cur[32];
    int curlen = snprintf(cur, sizeof(cur), "\x1b[%d;%dH", term_rows, clen + 1);
    ab_append(ab, cur, curlen);
    ab_lit(ab, ESC_SHOW_CURSOR);
    return;
  }

  const char *pill_style;
  const char *mode_str;

  if (buffer->mode == MODE_INSERT)
  {
    pill_style = STYLE_PILL_INSERT;
    mode_str = " INSERT ";
  }
  else
  {
    pill_style = STYLE_PILL_NORMAL;
    mode_str = " NORMAL ";
  }

  // Pill
  ab_lit(ab, STYLE_RESET);
  ab_append(ab, pill_style, strlen(pill_style));
  ab_append(ab, mode_str, strlen(mode_str));

  // Rest of bar
  ab_lit(ab, STYLE_BAR);

  int bar_len = 0;
  if (buffer->statusmsg[0])
  {
    bar_len = (int) strlen(buffer->statusmsg);
    ab_lit(ab, "  ");
    ab_append(ab, buffer->statusmsg, bar_len);
    bar_len += 2;
  }

  int used = (int) strlen(mode_str) + bar_len;
  for (int i = used; i < term_cols; i++)
    ab_append(ab, " ", 1);
  ab_lit(ab, STYLE_RESET);

  // Reposition cursor in content area, offset by gutter width.
  // We need total line count to compute the same gutter_width as render_content.
  int cy = cursor_line(buffer) - buffer->rowoff;
  int cx = cursor_col(buffer);
  int blen_rope = buffer_length(buffer);
  char *full_text = rope_to_string(buffer->rope);
  int total_lines = full_text ? count_lines(full_text, blen_rope) : 1;
  free(full_text);
  int gw = gutter_width(total_lines);
  char buf[32];
  int blen = snprintf(buf, sizeof(buf), "\x1b[%d;%dH", cy + 1, cx + gw + 1);
  ab_append(ab, buf, blen);
  ab_lit(ab, ESC_SHOW_CURSOR);
}

// ---------------------------------------------------------------------------
// Public entry point
// ---------------------------------------------------------------------------

void ui_render(Buffer *buffer)
{
  struct abuf ab = ABUF_INIT;

  ab_lit(&ab, ESC_HIDE_CURSOR);
  ab_lit(&ab, ESC_CURSOR_HOME);

  render_content(&ab, buffer);
  render_statusbar(&ab, buffer);

  write(STDOUT_FILENO, ab.b, ab.len);
  ab_free(&ab);
}
