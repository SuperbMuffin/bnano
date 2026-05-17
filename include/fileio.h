#ifndef FILEIO_H
#define FILEIO_H

#include "buffer.h"

void fileio_open(Buffer *b, const char *path);
void fileio_save(Buffer *b, const char *path);

#endif // FILEIO_H
