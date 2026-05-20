#include "rope.h"

#include <stdlib.h>
#include <string.h>

static char *rope_strdup(const char *str)
{
  size_t len = strlen(str);
  char *copy = malloc(len + 1);
  if (copy == NULL)
    return NULL;
  memcpy(copy, str, len + 1);
  return copy;
}

static char *rope_strndup(const char *str, size_t n)
{
  char *copy = malloc(n + 1);
  if (copy == NULL)
    return NULL;
  memcpy(copy, str, n);
  copy[n] = '\0';
  return copy;
}

#define MAX_DEPTH 64

static size_t fib_table[MAX_DEPTH + 1];
static int fib_ready = 0;

static void build_fib_table(void)
{
  if (fib_ready)
    return;
  fib_table[0] = 0;
  fib_table[1] = 1;
  for (int i = 2; i <= MAX_DEPTH; i++)
    fib_table[i] = fib_table[i - 1] + fib_table[i - 2];
  fib_ready = 1;
}

void rope_free(Rope *r)
{
  if (r == NULL)
    return;
  free(r->str);
  rope_free(r->left);
  rope_free(r->right);
  free(r);
}

Rope *rope_create(const char *str)
{
  Rope *node = malloc(sizeof(Rope));
  if (node == NULL)
    return NULL;
  node->str = rope_strdup(str);
  if (node->str == NULL)
  {
    free(node);
    return NULL;
  }
  node->weight = (int) strlen(str);
  node->left = NULL;
  node->right = NULL;
  return node;
}

int rope_length(Rope *r)
{
  if (r == NULL)
    return 0;
  if (r->str != NULL)
    return r->weight;
  return r->weight + rope_length(r->right);
}

static Rope *rope_concat(Rope *r1, Rope *r2)
{
  if (r1 == NULL)
    return r2;
  if (r2 == NULL)
    return r1;
  // drop empty leaves so they can't corrupt weights after rebalance
  if (r1->str != NULL && r1->weight == 0)
  {
    rope_free(r1);
    return r2;
  }
  if (r2->str != NULL && r2->weight == 0)
  {
    rope_free(r2);
    return r1;
  }
  Rope *node = malloc(sizeof(Rope));
  if (node == NULL)
    return NULL;
  node->str = NULL;
  node->weight = rope_length(r1);
  node->left = r1;
  node->right = r2;
  return node;
}

static void rope_split(Rope *r, int index, Rope **left, Rope **right)
{
  if (r == NULL)
  {
    *left = *right = NULL;
    return;
  }

  if (r->str != NULL)
  {
    if (index <= 0)
    {
      *left = NULL;
      *right = r;
    }
    else if (index >= r->weight)
    {
      *left = r;
      *right = NULL;
    }
    else
    {
      char *right_str = rope_strndup(r->str + index, (size_t) (r->weight - index));
      r->str[index] = '\0';
      r->weight = index;
      *left = r;
      *right = rope_create(right_str);
      free(right_str);
    }
    return;
  }

  if (index < r->weight)
  {
    Rope *old_right = r->right;
    Rope *new_left, *new_right;
    rope_split(r->left, index, &new_left, &new_right);
    free(r);
    *left = new_left;
    *right = rope_concat(new_right, old_right);
  }
  else
  {
    Rope *old_left = r->left;
    Rope *new_left, *new_right;
    rope_split(r->right, index - r->weight, &new_left, &new_right);
    free(r);
    *left = rope_concat(old_left, new_left);
    *right = new_right;
  }
}

static void rope_flatten(Rope *r, char *buf, int *pos)
{
  if (r == NULL)
    return;
  if (r->str != NULL)
  {
    memcpy(buf + *pos, r->str, (size_t) r->weight);
    *pos += r->weight;
    return;
  }
  rope_flatten(r->left, buf, pos);
  rope_flatten(r->right, buf, pos);
}

char *rope_to_string(Rope *r)
{
  int len = rope_length(r);
  char *buf = malloc((size_t) len + 1);
  if (buf == NULL)
    return NULL;
  int pos = 0;
  rope_flatten(r, buf, &pos);
  buf[len] = '\0';
  return buf;
}

// Leaf-walk helper: copy bytes from leaves overlapping [start, end).
static void rope_flatten_range(Rope *r, int *pos, int start, int end, char *buf, int *out)
{
  if (r == NULL || *pos >= end)
    return;
  if (r->str != NULL)
  {
    int leaf_start = *pos;
    int leaf_end   = *pos + r->weight;
    int lo = leaf_start > start ? leaf_start : start;
    int hi = leaf_end   < end   ? leaf_end   : end;
    if (lo < hi)
    {
      memcpy(buf + *out, r->str + (lo - leaf_start), (size_t)(hi - lo));
      *out += hi - lo;
    }
    *pos += r->weight;
    return;
  }
  rope_flatten_range(r->left,  pos, start, end, buf, out);
  rope_flatten_range(r->right, pos, start, end, buf, out);
}

// Returns a newly allocated string for rope[start..end). Caller must free().
char *rope_slice(Rope *r, int start, int end)
{
  int total = rope_length(r);
  if (start < 0)   start = 0;
  if (end > total) end   = total;
  if (start >= end)
  {
    char *empty = malloc(1);
    if (empty) empty[0] = '\0';
    return empty;
  }
  int len = end - start;
  char *buf = malloc((size_t) len + 1);
  if (buf == NULL)
    return NULL;
  int pos = 0, out = 0;
  rope_flatten_range(r, &pos, start, end, buf, &out);
  buf[out] = '\0';
  return buf;
}

char rope_index(Rope *r, int index)
{
  if (r == NULL || index < 0)
    return '\0';
  if (r->str != NULL)
  {
    if (index >= r->weight)
      return '\0';
    return r->str[index];
  }
  if (index < r->weight)
    return rope_index(r->left, index);
  else
    return rope_index(r->right, index - r->weight);
}

static int count_leaves(Rope *r)
{
  if (r == NULL)
    return 0;
  if (r->str != NULL)
    return 1;
  return count_leaves(r->left) + count_leaves(r->right);
}

static void collect_leaves(Rope *r, Rope **leaves, int *idx)
{
  if (r == NULL)
    return;
  if (r->str != NULL)
  {
    leaves[(*idx)++] = r;
    return;
  }
  collect_leaves(r->left, leaves, idx);
  collect_leaves(r->right, leaves, idx);
  free(r);
}

static Rope *rebuild_balanced(Rope **leaves, int lo, int hi)
{
  if (lo > hi)
    return NULL;
  if (lo == hi)
    return leaves[lo];
  int mid = lo + (hi - lo) / 2;
  return rope_concat(rebuild_balanced(leaves, lo, mid), rebuild_balanced(leaves, mid + 1, hi));
}

static int rope_depth(Rope *r)
{
  if (r == NULL || r->str != NULL)
    return 0;
  int ld = rope_depth(r->left);
  int rd = rope_depth(r->right);
  return 1 + (ld > rd ? ld : rd);
}

Rope *rope_rebalance(Rope *r)
{
  build_fib_table();
  int depth = rope_depth(r);
  int length = rope_length(r);

  if (depth <= MAX_DEPTH && (size_t) length >= fib_table[depth])
    return r;

  int n = count_leaves(r);
  if (n == 0)
    return r;

  Rope **leaves = malloc((size_t) n * sizeof(Rope *));
  if (leaves == NULL)
    return r;

  int idx = 0;
  collect_leaves(r, leaves, &idx);
  Rope *balanced = rebuild_balanced(leaves, 0, n - 1);
  free(leaves);
  return balanced;
}

Rope *rope_insert(Rope *r, int index, const char *str)
{
  Rope *left, *right;
  rope_split(r, index, &left, &right);
  Rope *new_node = rope_create(str);
  Rope *result = rope_concat(rope_concat(left, new_node), right);
  return rope_rebalance(result);
}

Rope *rope_delete(Rope *r, int start, int end)
{
  Rope *left, *temp;
  rope_split(r, start, &left, &temp);
  Rope *middle, *right;
  rope_split(temp, end - start, &middle, &right);
  rope_free(middle);
  Rope *result = rope_concat(left, right);
  return rope_rebalance(result);
}
