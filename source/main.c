#include "buffer.h"
#include "command.h"
#include "fileio.h"
#include "history.h"
#include "terminal.h"
#include "ui.h"

#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
  terminal_enable_raw_mode();
  terminal_enable_alt_screen();

  atexit(terminal_disable_raw_mode);
  atexit(terminal_disable_alt_screen);

  terminal_get_size(&term_rows, &term_cols);

  Buffer buf;
  buffer_init(&buf);
  history_init();
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
      {
        history_on_insert_mode(&buf);
        buf.mode = MODE_INSERT;
      }
      else if (c == 'a')
      {
        int len = buffer_length(&buf);
        if (buf.cursor < len)
          buffer_move_cursor(&buf, 1, 0);
        history_on_insert_mode(&buf);
        buf.mode = MODE_INSERT;
      }
      else if (c == 'o')
      {
        int len = buffer_length(&buf);
        while (buf.cursor < len && buffer_get_char(&buf, buf.cursor) != '\n')
          buffer_move_cursor(&buf, 1, 0);
        history_on_insert_mode(&buf);
        buffer_insert_char(&buf, '\n');
        buf.mode = MODE_INSERT;
      }
      else if (c == 'u')
        history_undo(&buf);
      else if (c == 18) // ctrl-r
        history_redo(&buf);
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
      {
        history_on_action(&buf, ACTION_INSERT);
        buffer_insert_char(&buf, c);
      }
      else if (c == 127)
      {
        history_on_action(&buf, ACTION_DELETE);
        buffer_delete_char(&buf);
      }
      else if (c == '\t')
      {
        // Expand tab to spaces, aligning to the next tab stop
        history_on_action(&buf, ACTION_INSERT);
        int spaces = TAB_WIDTH - (cursor_col(&buf) % TAB_WIDTH);
        for (int i = 0; i < spaces; i++)
          buffer_insert_char(&buf, ' ');
      }
      else if (c == '\r')
      {
        history_push(&buf);
        buffer_insert_char(&buf, '\n');
      }
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
