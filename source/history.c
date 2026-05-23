#include "history.h"
#include "rope.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_HISTORY 128

typedef struct
{
  Rope *rope;
  int cursor;
  int cursor_cx;
  int cursor_cy;
} Snapshot;

static Snapshot undo_stack[MAX_HISTORY];
static int undo_top = 0; // number of entries on undo stack

static Snapshot redo_stack[MAX_HISTORY];
static int redo_top = 0; // number of entries on redo stack

static HistoryAction last_action = ACTION_NONE;

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

static Snapshot snapshot_create(Buffer *b)
{
  Snapshot s;
  // Flatten and re-create so we own an independent copy of the rope
  char *text = rope_to_string(b->rope);
  s.rope = rope_create(text ? text : "");
  free(text);
  s.cursor = b->cursor;
  s.cursor_cx = b->cursor_cx;
  s.cursor_cy = b->cursor_cy;
  return s;
}

static void snapshot_free(Snapshot *s)
{
  rope_free(s->rope);
  s->rope = NULL;
}

static void snapshot_restore(Buffer *b, Snapshot *s)
{
  rope_free(b->rope);
  char *text = rope_to_string(s->rope);
  b->rope = rope_create(text ? text : "");
  free(text);
  b->cursor = s->cursor;
  b->cursor_cx = s->cursor_cx;
  b->cursor_cy = s->cursor_cy;
  b->dirty = 1;
}

static void push_undo(Buffer *b)
{
  if (undo_top >= MAX_HISTORY)
  {
    // Drop the oldest entry (index 0) by shifting everything down
    snapshot_free(&undo_stack[0]);
    memmove(&undo_stack[0], &undo_stack[1], (size_t) (MAX_HISTORY - 1) * sizeof(Snapshot));
    undo_top--;
  }
  undo_stack[undo_top++] = snapshot_create(b);
}

static void clear_redo(void)
{
  for (int i = 0; i < redo_top; i++)
    snapshot_free(&redo_stack[i]);
  redo_top = 0;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void history_init(void)
{
  undo_top = 0;
  redo_top = 0;
  last_action = ACTION_NONE;
}

void history_free(void)
{
  for (int i = 0; i < undo_top; i++)
    snapshot_free(&undo_stack[i]);
  for (int i = 0; i < redo_top; i++)
    snapshot_free(&redo_stack[i]);
  undo_top = 0;
  redo_top = 0;
}

void history_on_insert_mode(Buffer *b)
{
  // Each entry into insert mode is a fresh undo boundary
  push_undo(b);
  clear_redo();
  last_action = ACTION_NONE;
}

void history_on_action(Buffer *b, HistoryAction action)
{
  if (action == ACTION_NONE)
    return;

  // If the action type flipped (insert→delete or delete→insert), push a new snapshot
  if (last_action != ACTION_NONE && action != last_action)
  {
    push_undo(b);
    clear_redo();
  }

  last_action = action;
}

void history_push(Buffer *b)
{
  push_undo(b);
  clear_redo();
  last_action = ACTION_NONE;
}

void history_undo(Buffer *b)
{
  if (undo_top == 0)
  {
    snprintf(b->statusmsg, STATUSMSG_MAX, "Already at oldest change");
    return;
  }

  // Push current state onto redo stack, evicting oldest if full
  if (redo_top >= MAX_HISTORY)
  {
    snapshot_free(&redo_stack[0]);
    memmove(&redo_stack[0], &redo_stack[1], (size_t) (MAX_HISTORY - 1) * sizeof(Snapshot));
    redo_top--;
  }
  redo_stack[redo_top++] = snapshot_create(b);

  Snapshot *s = &undo_stack[--undo_top];
  snapshot_restore(b, s);
  snapshot_free(s);

  snprintf(b->statusmsg, STATUSMSG_MAX, "Undid change #%d", undo_top + 1);
  last_action = ACTION_NONE;
}

void history_redo(Buffer *b)
{
  if (redo_top == 0)
  {
    snprintf(b->statusmsg, STATUSMSG_MAX, "Already at newest change");
    return;
  }

  // Push current state onto undo stack, evicting oldest if full
  if (undo_top >= MAX_HISTORY)
  {
    snapshot_free(&undo_stack[0]);
    memmove(&undo_stack[0], &undo_stack[1], (size_t) (MAX_HISTORY - 1) * sizeof(Snapshot));
    undo_top--;
  }
  undo_stack[undo_top++] = snapshot_create(b);

  Snapshot *s = &redo_stack[--redo_top];
  snapshot_restore(b, s);
  snapshot_free(s);

  snprintf(b->statusmsg, STATUSMSG_MAX, "Redid change #%d", undo_top);
  last_action = ACTION_NONE;
}
