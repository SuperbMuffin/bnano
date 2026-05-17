#ifndef ROPE_H
#define ROPE_H

typedef struct Rope
{
  char *str;          // For leaf nodes, this holds the string
  int weight;         // For internal nodes, this holds the total length of the left subtree
  struct Rope *left;  // Left child
  struct Rope *right; // Right child
} Rope;

int rope_length(Rope *r);
Rope *rope_create(const char *str);
char rope_index(Rope *r, int index);
Rope *rope_delete(Rope *r, int start, int end);
Rope *rope_insert(Rope *r, int index, const char *str);
void rope_free(Rope *r);

#endif // ROPE_H
