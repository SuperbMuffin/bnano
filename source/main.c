#include "terminal.h"
#include "buffer.h"
#include "fileio.h"

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

  const char *filename = argc > 1 ? argv[1] : NULL;
  if (filename)
    fileio_open(&buf, filename);

  buffer_scroll(&buf);
  terminal_refresh_screen(&buf);

  while (1)
  {
    int c = terminal_read_key();
    if (c == -1) continue;

    if (c == 'q')
      break;
    else if (c == 19) // Ctrl-S
    {
      if (filename)
        fileio_save(&buf, filename);
    }
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
    
    // In all cases (keypress or resize), update state and redraw
    buffer_scroll(&buf);
    terminal_refresh_screen(&buf);
  }


  return 0;
}
