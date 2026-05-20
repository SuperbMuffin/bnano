#define _POSIX_C_SOURCE 200809L

#include "ui.h"
#include "buffer.h"
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
// Content area
// ---------------------------------------------------------------------------

static void render_content(struct abuf *ab, Buffer *buffer)
{
  int len         = buffer_length(buffer);
  int start_index = buffer_visual_line_start(buffer, buffer->rowoff);
  int x = 0, y = 0;

  // Flatten only the visible portion of the rope — one O(n) pass instead of
  // O(n log n) individual rope_index calls.
  char *slice = rope_slice(buffer->rope, start_index, len);
  int   slice_len = slice ? (int)(len - start_index) : 0;

  for (int i = 0; i < slice_len && y < term_rows - 1; i++)
  {
    char c = slice[i];
    if (c == '\n')
    {
      ab_lit(ab, ESC_CLEAR_LINE "\r\n");
      x = 0;
      y++;
    }
    else
    {
      ab_append(ab, &c, 1);
      x++;
      if (x >= term_cols)
      {
        ab_lit(ab, "\r\n");
        x = 0;
        y++;
      }
    }
  }

  free(slice);

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

  // Reposition cursor in content area
  int cy = cursor_line(buffer) - buffer->rowoff;
  int cx = cursor_col(buffer);
  char buf[32];
  int blen = snprintf(buf, sizeof(buf), "\x1b[%d;%dH", cy + 1, cx + 1);
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
