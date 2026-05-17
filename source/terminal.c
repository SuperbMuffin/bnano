#define _POSIX_C_SOURCE 200809L

#include "terminal.h"
#include "buffer.h"

#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <poll.h>
#include <string.h>

int term_rows;
int term_cols;

int resize_pipe[2];
static struct termios orig_termios;

void terminal_get_size(int *rows, int *cols)
{
  struct winsize ws;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
  *rows = ws.ws_row;
  *cols = ws.ws_col;
}

static void handle_sigwinch(int sig)
{
  (void) sig; // Unused parameter
  terminal_get_size(&term_rows, &term_cols);
  write(resize_pipe[1], "r", 1); // Notify main loop of resize
}

void terminal_disable_raw_mode(void)
{
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void terminal_enable_raw_mode(void)
{
  tcgetattr(STDIN_FILENO, &orig_termios);

  if (pipe(resize_pipe) == -1)
  {
    perror("pipe");
    exit(1);
  }

  int flags = fcntl(resize_pipe[0], F_GETFL, 0);
  fcntl(resize_pipe[0], F_SETFL, flags | O_NONBLOCK);

  struct sigaction sa;
  sa.sa_handler = handle_sigwinch;
  sa.sa_flags = SA_RESTART;
  sigemptyset(&sa.sa_mask);
  sigaction(SIGWINCH, &sa, NULL);

  struct termios raw = orig_termios;
  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  raw.c_oflag &= ~(OPOST);
  raw.c_cflag |= (CS8);
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1;

  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int terminal_read_key(void)
{
  struct pollfd fds[2];
  fds[0].fd = STDIN_FILENO;
  fds[0].events = POLLIN;
  fds[1].fd = resize_pipe[0];
  fds[1].events = POLLIN;

  if (poll(fds, 2, -1) == -1)
    return -1;

  if (fds[1].revents & POLLIN)
  {
    char discard;
    while (read(resize_pipe[0], &discard, 1) > 0)
      ;
    return KEY_RESIZE;
  }

  if (!(fds[0].revents & POLLIN))
    return -1;

  char c;
  ssize_t nread = read(STDIN_FILENO, &c, 1);
  if (nread <= 0)
    return -1;

  if (c == '\x1b')
  {
    char seq[3];
    if (read(STDIN_FILENO, &seq[0], 1) != 1)
      return '\x1b';
    if (read(STDIN_FILENO, &seq[1], 1) != 1)
      return '\x1b';

    if (seq[0] == '[')
    {
      switch (seq[1])
      {
        case 'A':
          return KEY_UP;
        case 'B':
          return KEY_DOWN;
        case 'C':
          return KEY_RIGHT;
        case 'D':
          return KEY_LEFT;
      }
    }
    return '\x1b';
  }
  return (unsigned char) c;
}

void terminal_clear_screen(void)
{
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);
}

void terminal_enable_alt_screen(void)
{
  write(STDOUT_FILENO, "\x1b[?1049h", 8);
}

void terminal_disable_alt_screen(void)
{
  write(STDOUT_FILENO, "\x1b[?1049l", 8);
}

struct abuf
{
  char *b;
  int len;
};

#define ABUF_INIT {NULL, 0}

void abAppend(struct abuf *ab, const char *s, int len)
{
  char *new = realloc(ab->b, ab->len + len);
  if (new == NULL)
    return;
  memcpy(&new[ab->len], s, len);
  ab->b = new;
  ab->len += len;
}

void abFree(struct abuf *ab)
{
  free(ab->b);
}

void terminal_refresh_screen(Buffer *buffer)
{
  struct abuf ab = ABUF_INIT;

  abAppend(&ab, "\x1b[?25l", 6);
  abAppend(&ab, "\x1b[H", 3);

  int len = buffer_length(buffer);
  int start_index = buffer_visual_line_start(buffer, buffer->rowoff);
  int x = 0, y = 0;

  for (int i = start_index; i < len && y < term_rows - 1; i++)
  {
    char c = buffer_get_char(buffer, i);
    if (c == '\n')
    {
      abAppend(&ab, "\x1b[K", 3);
      abAppend(&ab, "\r\n", 2);
      x = 0;
      y++;
    }
    else
    {
      abAppend(&ab, &c, 1);
      x++;
      if (x >= term_cols)
      {
        abAppend(&ab, "\r\n", 2);
        x = 0;
        y++;
      }
    }
  }

  abAppend(&ab, "\x1b[J", 3);

  int cy = cursor_line(buffer) - buffer->rowoff;
  int cx = cursor_col(buffer);
  char buf[32];
  int blen = snprintf(buf, sizeof(buf), "\x1b[%d;%dH", cy + 1, cx + 1);
  abAppend(&ab, buf, blen);
  abAppend(&ab, "\x1b[?25h", 6);

  write(STDOUT_FILENO, ab.b, ab.len);
  abFree(&ab);
}
