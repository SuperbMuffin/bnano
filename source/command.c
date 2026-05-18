#include "command.h"
#include "buffer.h"
#include "fileio.h"

#include <string.h>
#include <stdio.h>

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
  return 1;
}

static int cmd_file(Buffer *buf, const char *args)
{
  if (!args[0])
  {
    snprintf(buf->statusmsg, STATUSMSG_MAX, buf->filename[0] ? "\"%s\"" : "No filename",
             buf->filename);
    return 0;
  }
  buffer_set_filename(buf, args);
  snprintf(buf->statusmsg, STATUSMSG_MAX, "Renamed to: %s", args);
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
}
