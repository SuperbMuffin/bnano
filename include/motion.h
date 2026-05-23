#ifndef MOTION_H
#define MOTION_H

#include "buffer.h"

// Word motions
void motion_word_next(Buffer *b); // w — start of next word
void motion_word_back(Buffer *b); // b — start of previous word
void motion_word_end(Buffer *b);  // e — end of current/next word

// File motions
void motion_file_top(Buffer *b); // gg — first line
void motion_file_end(Buffer *b); // G  — last line

#endif // MOTION_H
