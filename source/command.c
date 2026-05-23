#include "command.h"
#include "buffer.h"
#include "fileio.h"
#include "rope.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
  const char *name;
  CommandFn fn;
} Command;

// ---------------------------------------------------------------------------
// Registry
// ---------------------------------------------------------------------------

#define MAX_COMMANDS 64

static Command registry[MAX_COMMANDS];
static int registry_len = 0;

void command_register(const char *name, CommandFn fn)
{
  if (registry_len >= MAX_COMMANDS)
    return;
  registry[registry_len].name = name;
  registry[registry_len].fn = fn;
  registry_len++;
}

// Forward declaration needed by command_run's numeric fallback
static int cmd_goto(Buffer *buf, const char *args);

// Split "w foo.c" into name="w", args="foo.c"
int command_run(Buffer *buf, const char *input)
{
  char name[64];
  const char *args = "";

  const char *space = strchr(input, ' ');
  if (space)
  {
    size_t nlen = (size_t) (space - input);
    if (nlen >= sizeof(name))
      nlen = sizeof(name) - 1;
    memcpy(name, input, nlen);
    name[nlen] = '\0';
    args = space + 1;
  }
  else
  {
    strncpy(name, input, sizeof(name) - 1);
    name[sizeof(name) - 1] = '\0';
  }

  for (int i = 0; i < registry_len; i++)
  {
    if (strcmp(registry[i].name, name) == 0)
      return registry[i].fn(buf, args);
  }

  // :N — bare number jumps to line N
  int all_digits = 1;
  for (int i = 0; name[i]; i++)
    if (!isdigit((unsigned char) name[i]))
    {
      all_digits = 0;
      break;
    }
  if (all_digits && name[0])
  {
    const char *num_args = name; // reuse name as the line number string
    return cmd_goto(buf, num_args);
  }

  snprintf(buf->statusmsg, STATUSMSG_MAX, "Not a command: %s", name);
  return 0;
}

// ---------------------------------------------------------------------------
// Default commands
// ---------------------------------------------------------------------------

static int cmd_quit(Buffer *buf, const char *args)
{
  (void) args;
  if (buf->dirty)
  {
    snprintf(buf->statusmsg, STATUSMSG_MAX, "Unsaved changes, use :q! to force quit");
    return 0;
  }
  return 1;
}

static int cmd_quit_force(Buffer *buf, const char *args)
{
  (void) buf;
  (void) args;
  return 1;
}

static int cmd_write(Buffer *buf, const char *args)
{
  const char *target = (args && args[0]) ? args : buf->filename;
  if (!target || !target[0])
  {
    snprintf(buf->statusmsg, STATUSMSG_MAX, "No filename");
    return 0;
  }
  fileio_save(buf, target);
  buf->dirty = 0;
  snprintf(buf->statusmsg, STATUSMSG_MAX, "Written: %s", target);
  return 0;
}

static int cmd_write_quit(Buffer *buf, const char *args)
{
  cmd_write(buf, args);
  return cmd_quit(buf, args);
}

static int cmd_file(Buffer *buf, const char *args)
{
  if (!args[0])
  {
    if (buf->filename && buf->filename[0])
      snprintf(buf->statusmsg, STATUSMSG_MAX, "\"%s\"", buf->filename);
    else
      snprintf(buf->statusmsg, STATUSMSG_MAX, "No filename");
    return 0;
  }
  buffer_set_filename(buf, args);
  snprintf(buf->statusmsg, STATUSMSG_MAX, "Renamed to: %s", args);
  return 0;
}

static int cmd_goto(Buffer *buf, const char *args)
{
  if (!args || !args[0])
  {
    snprintf(buf->statusmsg, STATUSMSG_MAX, "Usage: :goto <line>");
    return 0;
  }

  int target = atoi(args);
  if (target < 1)
    target = 1;

  // Count total lines so we can clamp
  int len = buffer_length(buf);
  char *text = rope_to_string(buf->rope);
  if (text == NULL)
    return 0;

  int total_lines = 1;
  for (int i = 0; i < len; i++)
    if (text[i] == '\n')
      total_lines++;
  free(text);

  if (target > total_lines)
    target = total_lines;

  // Move cursor: dy = target_line - current_line (1-based → 0-based)
  int current = cursor_line(buf); // 0-based visual line
  int dest = target - 1;          // 0-based
  buffer_move_cursor(buf, 0, dest - current);
  // Jump to start of that line
  while (buf->cursor > 0 && buffer_get_char(buf, buf->cursor - 1) != '\n')
    buffer_move_cursor(buf, -1, 0);

  snprintf(buf->statusmsg, STATUSMSG_MAX, "Line %d", target);
  return 0;
}

void command_register_defaults(void)
{
  command_register("q", cmd_quit);
  command_register("q!", cmd_quit_force);
  command_register("w", cmd_write);
  command_register("wq", cmd_write_quit);
  command_register("x", cmd_write_quit);
  command_register("file", cmd_file);
  command_register("f", cmd_file);
  command_register("goto", cmd_goto);
  command_register("g", cmd_goto);
}
