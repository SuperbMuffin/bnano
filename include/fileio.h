#ifndef FILEIO_H
#define FILEIO_H

#include "buffer.h"

void fileio_open(Buffer *b, const char *path);
int fileio_save(Buffer *b, const char *path); // returns 1 on success, 0 on failure

#endif // FILEIO_H
