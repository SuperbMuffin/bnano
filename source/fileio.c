#include "fileio.h"
#include "rope.h"
#include "buffer.h"

#include <stdio.h>
#include <stdlib.h>

void fileio_open(Buffer *b, const char *path)
{
  FILE *f = fopen(path, "r");
  if (f == NULL)
    return;

  char *saved = b->filename;
  b->filename = NULL;
  rope_free(b->rope);
  buffer_init(b);
  b->filename = saved;

  b->rope = rope_create("");
  char chunk[4097];
  size_t n;
  int pos = 0;
  while ((n = fread(chunk, 1, sizeof(chunk) - 1, f)) > 0)
  {
    chunk[n] = '\0';
    b->rope = rope_insert(b->rope, pos, chunk);
    pos += (int) n;
  }
  fclose(f);
}

void fileio_save(Buffer *b, const char *path)
{
  FILE *f = fopen(path, "w");
  if (f == NULL)
    return;

  char *text = rope_to_string(b->rope);
  if (text != NULL)
  {
    fwrite(text, 1, (size_t) buffer_length(b), f);
    free(text);
  }

  fclose(f);
}
