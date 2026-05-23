#include "fileio.h"
#include "buffer.h"
#include "rope.h"

#include <stdio.h>
#include <stdlib.h>

void fileio_open(Buffer *b, const char *path)
{
  FILE *f = fopen(path, "r");
  if (f == NULL)
    return;

  // Reset buffer state without touching filename (already set by caller).
  // We free the old rope here and assign the new one below — no need to
  // round-trip through buffer_init which would allocate a throwaway rope.
  rope_free(b->rope);
  b->cursor = 0;
  b->saved_col = 0;
  b->rowoff = 0;
  b->cursor_cx = 0;
  b->cursor_cy = 0;
  b->mode = MODE_NORMAL;
  b->cmdlen = 0;
  b->cmdbuf[0] = '\0';
  b->statusmsg[0] = '\0';
  b->dirty = 0;

  // Read the entire file in one shot, then create the rope from it.
  // This avoids N chunk insertions (each triggering a rebalance) on open.
  fseek(f, 0, SEEK_END);
  long file_size = ftell(f);
  rewind(f);

  char *text = malloc((size_t) file_size + 1);
  if (text == NULL)
  {
    fclose(f);
    b->rope = rope_create("");
    return;
  }

  size_t n = fread(text, 1, (size_t) file_size, f);
  text[n] = '\0';
  fclose(f);

  b->rope = rope_create(text);
  free(text);
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
