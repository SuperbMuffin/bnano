#define _POSIX_C_SOURCE 200809L

#include "terminal.h"

#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

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

static void terminal_close_pipe(void)
{
  close(resize_pipe[0]);
  close(resize_pipe[1]);
}

static void handle_sigwinch(int sig)
{
  (void) sig;
  terminal_get_size(&term_rows, &term_cols);
  write(resize_pipe[1], "r", 1);
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
  atexit(terminal_close_pipe);

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
    struct pollfd esc_fd;
    esc_fd.fd = STDIN_FILENO;
    esc_fd.events = POLLIN;

    if (poll(&esc_fd, 1, 25) <= 0 || !(esc_fd.revents & POLLIN))
    {
      return '\x1b';
    }

    char seq[3];
    if (read(STDIN_FILENO, &seq[0], 1) != 1)
      return '\x1b';

    if (seq[0] == '[')
    {
      if (poll(&esc_fd, 1, 25) <= 0 || !(esc_fd.revents & POLLIN))
      {
        return '\x1b';
      }

      if (read(STDIN_FILENO, &seq[1], 1) != 1)
        return '\x1b';

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

void terminal_write(const char *s)
{
  write(STDOUT_FILENO, s, strlen(s));
}
