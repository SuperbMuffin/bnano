#ifndef COMMAND_H
#define COMMAND_H

#include "buffer.h"

// Return 0 to stay, 1 to quit
typedef int (*CommandFn)(Buffer *buf, const char *args);

typedef struct
{
  const char *name;
  CommandFn fn;
} Command;

void command_register(const char *name, CommandFn fn);
int command_run(Buffer *buf, const char *input);
void command_register_defaults(void);

#endif // COMMAND_H
