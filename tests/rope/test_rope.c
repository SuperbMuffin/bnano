#include "test_common.h"
#include "rope.h"

#include <stdlib.h>
#include <string.h>

static char *rope_to_str(Rope *r)
{
  int len = rope_length(r);
  char *buf = malloc(len + 1);
  for (int i = 0; i < len; i++)
    buf[i] = rope_index(r, i);
  buf[len] = '\0';
  return buf;
}

static void test_create_empty(void)
{
  Rope *r = rope_create("");
  ASSERT_EQ_INT(rope_length(r), 0, "empty rope length");
  rope_free(r);
}

static void test_create_basic(void)
{
  Rope *r = rope_create("hello");
  ASSERT_EQ_INT(rope_length(r), 5, "basic rope length");
  rope_free(r);
}

static void test_index_leaf(void)
{
  Rope *r = rope_create("abcde");
  ASSERT_EQ_CHAR(rope_index(r, 0), 'a', "index 0");
  ASSERT_EQ_CHAR(rope_index(r, 4), 'e', "index 4");
  ASSERT_EQ_CHAR(rope_index(r, 2), 'c', "index 2");
  rope_free(r);
}

static void test_index_after_insert(void)
{
  Rope *r = rope_create("ac");
  r = rope_insert(r, 1, "b");
  ASSERT_EQ_CHAR(rope_index(r, 0), 'a', "index 0 after insert");
  ASSERT_EQ_CHAR(rope_index(r, 1), 'b', "index 1 after insert");
  ASSERT_EQ_CHAR(rope_index(r, 2), 'c', "index 2 after insert");
  rope_free(r);
}

static void test_insert_at_start(void)
{
  Rope *r = rope_create("world");
  r = rope_insert(r, 0, "hello ");
  char *s = rope_to_str(r);
  ASSERT_EQ_STR(s, "hello world", "insert at start");
  free(s);
  rope_free(r);
}

static void test_insert_at_end(void)
{
  Rope *r = rope_create("hello");
  r = rope_insert(r, 5, " world");
  char *s = rope_to_str(r);
  ASSERT_EQ_STR(s, "hello world", "insert at end");
  free(s);
  rope_free(r);
}

static void test_insert_in_middle(void)
{
  Rope *r = rope_create("helloworld");
  r = rope_insert(r, 5, " ");
  char *s = rope_to_str(r);
  ASSERT_EQ_STR(s, "hello world", "insert in middle");
  free(s);
  rope_free(r);
}

static void test_insert_empty_string(void)
{
  Rope *r = rope_create("hello");
  r = rope_insert(r, 2, "");
  char *s = rope_to_str(r);
  ASSERT_EQ_STR(s, "hello", "insert empty string unchanged");
  ASSERT_EQ_INT(rope_length(r), 5, "insert empty string length unchanged");
  free(s);
  rope_free(r);
}

static void test_insert_into_empty(void)
{
  Rope *r = rope_create("");
  r = rope_insert(r, 0, "hello");
  char *s = rope_to_str(r);
  ASSERT_EQ_STR(s, "hello", "insert into empty rope");
  free(s);
  rope_free(r);
}

static void test_multiple_inserts(void)
{
  Rope *r = rope_create("");
  r = rope_insert(r, 0, "c");
  r = rope_insert(r, 0, "b");
  r = rope_insert(r, 0, "a");
  char *s = rope_to_str(r);
  ASSERT_EQ_STR(s, "abc", "multiple inserts at start");
  free(s);
  rope_free(r);
}

static void test_delete_middle(void)
{
  Rope *r = rope_create("hello world");
  r = rope_delete(r, 5, 6);
  char *s = rope_to_str(r);
  ASSERT_EQ_STR(s, "helloworld", "delete middle");
  free(s);
  rope_free(r);
}

static void test_delete_start(void)
{
  Rope *r = rope_create("hello world");
  r = rope_delete(r, 0, 6);
  char *s = rope_to_str(r);
  ASSERT_EQ_STR(s, "world", "delete start");
  free(s);
  rope_free(r);
}

static void test_delete_end(void)
{
  Rope *r = rope_create("hello world");
  r = rope_delete(r, 5, 11);
  char *s = rope_to_str(r);
  ASSERT_EQ_STR(s, "hello", "delete end");
  free(s);
  rope_free(r);
}

static void test_delete_all(void)
{
  Rope *r = rope_create("hello");
  r = rope_delete(r, 0, 5);
  ASSERT_EQ_INT(rope_length(r), 0, "delete all length");
  rope_free(r);
}

static void test_insert_then_delete(void)
{
  Rope *r = rope_create("hello world");
  r = rope_insert(r, 5, " beautiful");
  r = rope_delete(r, 5, 15);
  char *s = rope_to_str(r);
  ASSERT_EQ_STR(s, "hello world", "insert then delete restores string");
  free(s);
  rope_free(r);
}

static void test_rebalance_preserves_content(void)
{
  Rope *r = rope_create("");
  const char *str = "the quick brown fox jumps over the lazy dog";
  int len = (int) strlen(str);
  for (int i = 0; i < len; i++)
  {
    char ch[2] = {str[i], '\0'};
    r = rope_insert(r, rope_length(r), ch);
  }
  ASSERT_EQ_INT(rope_length(r), len, "rebalance: length preserved");
  char *s = rope_to_str(r);
  ASSERT_EQ_STR(s, str, "rebalance: content preserved");
  free(s);
  rope_free(r);
}

static void test_index_after_rebalance(void)
{
  Rope *r = rope_create("");
  for (int i = 0; i < 100; i++)
  {
    char ch[2] = {'a' + (i % 26), '\0'};
    r = rope_insert(r, rope_length(r), ch);
  }
  ASSERT_EQ_CHAR(rope_index(r, 0),  'a', "index 0 after many inserts");
  ASSERT_EQ_CHAR(rope_index(r, 25), 'z', "index 25 after many inserts");
  ASSERT_EQ_CHAR(rope_index(r, 26), 'a', "index 26 after many inserts");
  rope_free(r);
}

static void test_multi_chunk_load(void)
{
  const char *file = "abcdefghijklmnopqrstuvwxyz";
  int file_len = (int) strlen(file);
  int chunk_size = 4;
  Rope *r = rope_create("");
  int pos = 0;
  while (pos < file_len)
  {
    int n = chunk_size < file_len - pos ? chunk_size : file_len - pos;
    char chunk[5];
    memcpy(chunk, file + pos, n);
    chunk[n] = '\0';
    r = rope_insert(r, pos, chunk);
    pos += n;
  }
  ASSERT_EQ_INT(rope_length(r), file_len, "multi-chunk load length");
  char *s = rope_to_str(r);
  ASSERT_EQ_STR(s, file, "multi-chunk load content");
  free(s);
  rope_free(r);
}

static void test_split_at_zero(void)
{
  Rope *r = rope_create("world");
  r = rope_insert(r, 0, "hello ");
  ASSERT_EQ_CHAR(rope_index(r, 0), 'h', "split at 0: first char");
  ASSERT_EQ_CHAR(rope_index(r, 6), 'w', "split at 0: original start");
  rope_free(r);
}

static void test_split_at_length(void)
{
  Rope *r = rope_create("hello");
  r = rope_insert(r, 5, " world");
  ASSERT_EQ_INT(rope_length(r), 11, "split at length: correct length");
  ASSERT_EQ_CHAR(rope_index(r, 10), 'd', "split at length: last char");
  rope_free(r);
}

int main(void)
{
  run_test("create_empty",          test_create_empty);
  run_test("create_basic",          test_create_basic);
  run_test("index_leaf",            test_index_leaf);
  run_test("index_after_insert",    test_index_after_insert);
  run_test("insert_at_start",       test_insert_at_start);
  run_test("insert_at_end",         test_insert_at_end);
  run_test("insert_in_middle",      test_insert_in_middle);
  run_test("insert_empty_string",   test_insert_empty_string);
  run_test("insert_into_empty",     test_insert_into_empty);
  run_test("multiple_inserts",      test_multiple_inserts);
  run_test("delete_middle",         test_delete_middle);
  run_test("delete_start",          test_delete_start);
  run_test("delete_end",            test_delete_end);
  run_test("delete_all",            test_delete_all);
  run_test("insert_then_delete",    test_insert_then_delete);
  run_test("rebalance_preserves",   test_rebalance_preserves_content);
  run_test("index_after_rebalance", test_index_after_rebalance);
  run_test("multi_chunk_load",      test_multi_chunk_load);
  run_test("split_at_zero",         test_split_at_zero);
  run_test("split_at_length",       test_split_at_length);

  print_summary();
  return tc_suite_failed() > 0 ? 1 : 0;
}
