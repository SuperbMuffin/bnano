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

  // Reset all editing state (allocates a fresh empty rope), preserving filename.
  buffer_reset(b);

  // Read the entire file in one shot, then replace the rope with it.
  // This avoids N chunk insertions (each triggering a rebalance) on open.
  if (fseek(f, 0, SEEK_END) != 0)
  {
    fclose(f);
    return;
  }
  long file_size = ftell(f);
  if (file_size < 0)
  {
    fclose(f);
    return;
  }
  rewind(f);

  char *text = malloc((size_t) file_size + 1);
  if (text == NULL)
  {
    fclose(f);
    return; // buffer_reset already gave us a valid empty rope
  }

  size_t n = fread(text, 1, (size_t) file_size, f);
  text[n] = '\0';
  fclose(f);

  // Replace the empty rope buffer_reset allocated with the file contents.
  rope_free(b->rope);
  b->rope = rope_create(text);
  free(text);
}

// Returns 1 on success, 0 on failure.
int fileio_save(Buffer *b, const char *path)
{
  FILE *f = fopen(path, "w");
  if (f == NULL)
    return 0;

  char *text = rope_to_string(b->rope);
  if (text == NULL)
  {
    fclose(f);
    return 0;
  }

  size_t len = (size_t) buffer_length(b);
  size_t written = fwrite(text, 1, len, f);
  free(text);

  if (fclose(f) != 0 || written != len)
    return 0;

  return 1;
}
