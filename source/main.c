#include "terminal.h"
#include "ui.h"
#include "buffer.h"
#include "fileio.h"
#include "command.h"

#include <unistd.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
  terminal_enable_raw_mode();
  terminal_enable_alt_screen();

  atexit(terminal_disable_raw_mode);
  atexit(terminal_disable_alt_screen);

  terminal_get_size(&term_rows, &term_cols);

  Buffer buf;
  buffer_init(&buf);
  command_register_defaults();

  if (argc > 1)
  {
    buffer_set_filename(&buf, argv[1]);
    fileio_open(&buf, buf.filename);
  }

  buffer_scroll(&buf);
  ui_render(&buf);

  while (1)
  {
    int c = terminal_read_key();
    if (c == -1)
      continue;

    if (c == KEY_RESIZE)
    {
      buffer_scroll(&buf);
      ui_render(&buf);
      continue;
    }

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
        if (command_run(&buf, buf.cmdbuf))
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
      if (c == ':' || c == ';')
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
