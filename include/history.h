#ifndef HISTORY_H
#define HISTORY_H

#include "buffer.h"

typedef enum
{
  ACTION_NONE,
  ACTION_INSERT,
  ACTION_DELETE,
} HistoryAction;

void history_init(void);
void history_free(void);

// Call on entry to insert mode — always pushes a snapshot
void history_on_insert_mode(Buffer *b);

// Call on each keypress in insert mode — pushes if action type changed
void history_on_action(Buffer *b, HistoryAction action);

// Force a snapshot boundary (e.g. on newline)
void history_push(Buffer *b);

// Undo/redo — call from normal mode key handling
void history_undo(Buffer *b);
void history_redo(Buffer *b);

#endif // HISTORY_H
