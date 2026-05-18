#include "terminal.h"
#include "ui.h"
#include "buffer.h"
#include "fileio.h"

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Execute a colon command. Returns 1 if editor should quit.
static int run_command(Buffer *buf, const char *cmd, const char *filename)
{
  if (strcmp(cmd, "q") == 0)
    return 1;
  else if (strcmp(cmd, "q!") == 0)
    return 1;
  else if (strcmp(cmd, "w") == 0)
  {
    if (filename)
    {
      fileio_save(buf, filename);
      snprintf(buf->statusmsg, CMDBUF_MAX, "Written: %s", filename);
    }
    else
      snprintf(buf->statusmsg, CMDBUF_MAX, "No filename");
  }
  else if (strcmp(cmd, "wq") == 0 || strcmp(cmd, "x") == 0)
  {
    if (filename)
      fileio_save(buf, filename);
    return 1;
  }
  else
    snprintf(buf->statusmsg, CMDBUF_MAX, "Not a command: %s", cmd);

  return 0;
}

int main(int argc, char *argv[])
{
  terminal_enable_raw_mode();
  terminal_enable_alt_screen();

  atexit(terminal_disable_raw_mode);
  atexit(terminal_disable_alt_screen);

  terminal_get_size(&term_rows, &term_cols);

  Buffer buf;
  buffer_init(&buf);

  const char *filename = argc > 1 ? argv[1] : NULL;
  if (filename)
    fileio_open(&buf, filename);

  buffer_scroll(&buf);
  ui_render(&buf);

  while (1)
  {
    int c = terminal_read_key();
    if (c == -1)
      continue;

    if (buf.mode == MODE_COMMAND)
    {
      if (c == '\x1b') // ESC → cancel
      {
        buf.cmdlen = 0;
        buf.cmdbuf[0] = '\0';
        buf.statusmsg[0] = '\0';
        buf.mode = MODE_NORMAL;
      }
      else if (c == '\r') // Enter → execute
      {
        buf.cmdbuf[buf.cmdlen] = '\0';
        buf.mode = MODE_NORMAL;
        if (run_command(&buf, buf.cmdbuf, filename))
          break;
        buf.cmdlen = 0;
        buf.cmdbuf[0] = '\0';
      }
      else if ((c == 127 || c == 8) && buf.cmdlen > 0) // Backspace
      {
        buf.cmdbuf[--buf.cmdlen] = '\0';
        if (buf.cmdlen == 0)
        {
          buf.mode = MODE_NORMAL; // cancel if emptied
          buf.statusmsg[0] = '\0';
        }
      }
      else if (c >= 32 && c <= 126 && buf.cmdlen < CMDBUF_MAX - 1)
      {
        buf.cmdbuf[buf.cmdlen++] = (char) c;
        buf.cmdbuf[buf.cmdlen] = '\0';
      }
    }
    else if (buf.mode == MODE_NORMAL)
    {
      buf.statusmsg[0] = '\0'; // clear status on any normal keypress
      if (c == ':')
      {
        buf.mode = MODE_COMMAND;
        buf.cmdlen = 0;
        buf.cmdbuf[0] = '\0';
      }
      else if (c == 'i')
        buf.mode = MODE_INSERT;
      else if (c == 'a')
      {
        int len = buffer_length(&buf);
        if (buf.cursor < len)
          buffer_move_cursor(&buf, 1, 0);
        buf.mode = MODE_INSERT;
      }
      else if (c == 'o')
      {
        int len = buffer_length(&buf);
        while (buf.cursor < len && buffer_get_char(&buf, buf.cursor) != '\n')
          buffer_move_cursor(&buf, 1, 0);
        buffer_insert_char(&buf, '\n');
        buf.mode = MODE_INSERT;
      }
      else if (c == 'h' || c == KEY_LEFT)
        buffer_move_cursor(&buf, -1, 0);
      else if (c == 'l' || c == KEY_RIGHT)
        buffer_move_cursor(&buf, 1, 0);
      else if (c == 'k' || c == KEY_UP)
        buffer_move_cursor(&buf, 0, -1);
      else if (c == 'j' || c == KEY_DOWN)
        buffer_move_cursor(&buf, 0, 1);
      else if (c == 'x')
      {
        int len = buffer_length(&buf);
        if (buf.cursor < len)
        {
          buffer_move_cursor(&buf, 1, 0);
          buffer_delete_char(&buf);
        }
      }
      else if (c == '0')
      {
        while (buf.cursor > 0 && buffer_get_char(&buf, buf.cursor - 1) != '\n')
          buffer_move_cursor(&buf, -1, 0);
      }
      else if (c == '$')
      {
        int len = buffer_length(&buf);
        while (buf.cursor < len && buffer_get_char(&buf, buf.cursor) != '\n')
          buffer_move_cursor(&buf, 1, 0);
      }
    }
    else // MODE_INSERT
    {
      if (c == '\x1b')
        buf.mode = MODE_NORMAL;
      else if (c >= 32 && c <= 126)
        buffer_insert_char(&buf, c);
      else if (c == 127)
        buffer_delete_char(&buf);
      else if (c == '\r')
        buffer_insert_char(&buf, '\n');
      else if (c == KEY_UP)
        buffer_move_cursor(&buf, 0, -1);
      else if (c == KEY_DOWN)
        buffer_move_cursor(&buf, 0, 1);
      else if (c == KEY_LEFT)
        buffer_move_cursor(&buf, -1, 0);
      else if (c == KEY_RIGHT)
        buffer_move_cursor(&buf, 1, 0);
    }

    buffer_scroll(&buf);
    ui_render(&buf);
  }

  return 0;
}
