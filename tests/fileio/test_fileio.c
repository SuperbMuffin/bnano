#include "test_common.h"
#include "fileio.h"
#include "buffer.h"
#include "rope.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

static char *write_temp_file(const char *content)
{
  char *path = strdup("/tmp/bnano_test_XXXXXX");
  int fd = mkstemp(path);
  if (fd == -1) { free(path); return NULL; }
  if (content && content[0])
    write(fd, content, strlen(content));
  close(fd);
  return path;
}

static char *buf_to_str(Buffer *b)
{
  return rope_to_string(b->rope);
}

static void test_open_basic(void)
{
  char *path = write_temp_file("hello world");
  Buffer b;
  buffer_init(&b);
  fileio_open(&b, path);
  ASSERT_EQ_INT(buffer_length(&b), 11, "open basic: length");
  char *s = buf_to_str(&b);
  ASSERT_EQ_STR(s, "hello world", "open basic: content");
  free(s);
  buffer_free(&b);
  unlink(path);
  free(path);
}

static void test_open_empty_file(void)
{
  char *path = write_temp_file("");
  Buffer b;
  buffer_init(&b);
  fileio_open(&b, path);
  ASSERT_EQ_INT(buffer_length(&b), 0, "open empty: length 0");
  buffer_free(&b);
  unlink(path);
  free(path);
}

static void test_open_multiline(void)
{
  char *path = write_temp_file("line one\nline two\nline three\n");
  Buffer b;
  buffer_init(&b);
  fileio_open(&b, path);
  ASSERT_EQ_INT(buffer_length(&b), 29, "open multiline: length");
  char *s = buf_to_str(&b);
  ASSERT_EQ_STR(s, "line one\nline two\nline three\n", "open multiline: content");
  free(s);
  buffer_free(&b);
  unlink(path);
  free(path);
}

static void test_open_nonexistent(void)
{
  Buffer b;
  buffer_init(&b);
  fileio_open(&b, "/tmp/bnano_does_not_exist_xyz");
  ASSERT_EQ_INT(buffer_length(&b), 0, "open nonexistent: buffer stays empty");
  ASSERT_EQ_INT(b.cursor, 0, "open nonexistent: cursor stays 0");
  buffer_free(&b);
}

static void test_open_preserves_filename(void)
{
  char *path = write_temp_file("abc");
  Buffer b;
  buffer_init(&b);
  buffer_set_filename(&b, path);
  fileio_open(&b, path);
  ASSERT(b.filename != NULL, "open preserves filename: not null");
  ASSERT_EQ_STR(b.filename, path, "open preserves filename: matches");
  buffer_free(&b);
  unlink(path);
  free(path);
}

static void test_open_resets_cursor(void)
{
  char *path = write_temp_file("hello");
  Buffer b;
  buffer_init(&b);
  buffer_insert_char(&b, 'x');
  buffer_insert_char(&b, 'x');
  fileio_open(&b, path);
  ASSERT_EQ_INT(b.cursor, 0, "open resets cursor to 0");
  buffer_free(&b);
  unlink(path);
  free(path);
}

static void test_open_large_file(void)
{
  char *path = write_temp_file(NULL);
  FILE *f = fopen(path, "w");
  for (int i = 0; i < 10000; i++)
    fputc('a', f);
  fputc('Z', f);
  fclose(f);
  Buffer b;
  buffer_init(&b);
  fileio_open(&b, path);
  ASSERT_EQ_INT(buffer_length(&b), 10001, "open large: length");
  ASSERT_EQ_CHAR(rope_index(b.rope, 0),     'a', "open large: first char");
  ASSERT_EQ_CHAR(rope_index(b.rope, 9999),  'a', "open large: last a");
  ASSERT_EQ_CHAR(rope_index(b.rope, 10000), 'Z', "open large: sentinel");
  buffer_free(&b);
  unlink(path);
  free(path);
}

static void test_save_basic(void)
{
  char *path = write_temp_file("");
  Buffer b;
  buffer_init(&b);
  buffer_insert_char(&b, 'h');
  buffer_insert_char(&b, 'i');
  fileio_save(&b, path);
  FILE *f = fopen(path, "r");
  char buf[64];
  size_t n = fread(buf, 1, sizeof(buf) - 1, f);
  buf[n] = '\0';
  fclose(f);
  ASSERT_EQ_STR(buf, "hi", "save basic: content on disk");
  buffer_free(&b);
  unlink(path);
  free(path);
}

static void test_save_empty_buffer(void)
{
  char *path = write_temp_file("existing content");
  Buffer b;
  buffer_init(&b);
  fileio_save(&b, path);
  FILE *f = fopen(path, "r");
  char buf[64];
  size_t n = fread(buf, 1, sizeof(buf) - 1, f);
  fclose(f);
  ASSERT_EQ_INT((int) n, 0, "save empty buffer: file is empty");
  buffer_free(&b);
  unlink(path);
  free(path);
}

static void test_save_multiline(void)
{
  char *path = write_temp_file("");
  Buffer b;
  buffer_init(&b);
  const char *text = "first\nsecond\nthird\n";
  for (const char *p = text; *p; p++)
    buffer_insert_char(&b, *p);
  fileio_save(&b, path);
  FILE *f = fopen(path, "r");
  char buf[64];
  size_t n = fread(buf, 1, sizeof(buf) - 1, f);
  buf[n] = '\0';
  fclose(f);
  ASSERT_EQ_STR(buf, text, "save multiline: content on disk");
  buffer_free(&b);
  unlink(path);
  free(path);
}

static void test_save_overwrites_existing(void)
{
  char *path = write_temp_file("old content that is longer");
  Buffer b;
  buffer_init(&b);
  buffer_insert_char(&b, 'n');
  buffer_insert_char(&b, 'e');
  buffer_insert_char(&b, 'w');
  fileio_save(&b, path);
  FILE *f = fopen(path, "r");
  char buf[64];
  size_t n = fread(buf, 1, sizeof(buf) - 1, f);
  buf[n] = '\0';
  fclose(f);
  ASSERT_EQ_STR(buf, "new", "save overwrites: correct new content");
  ASSERT_EQ_INT((int) n, 3, "save overwrites: no leftover bytes");
  buffer_free(&b);
  unlink(path);
  free(path);
}

static void test_roundtrip(void)
{
  char *path = write_temp_file("hello");
  Buffer b;
  buffer_init(&b);
  buffer_set_filename(&b, path);
  fileio_open(&b, path);
  while (b.cursor < buffer_length(&b))
    buffer_move_cursor(&b, 1, 0);
  const char *extra = " world";
  for (const char *p = extra; *p; p++)
    buffer_insert_char(&b, *p);
  fileio_save(&b, path);
  buffer_free(&b);

  Buffer b2;
  buffer_init(&b2);
  fileio_open(&b2, path);
  char *s = buf_to_str(&b2);
  ASSERT_EQ_STR(s, "hello world", "roundtrip: content after edit+save+reopen");
  free(s);
  buffer_free(&b2);
  unlink(path);
  free(path);
}

int main(void)
{
  run_test("open_basic",              test_open_basic);
  run_test("open_empty_file",         test_open_empty_file);
  run_test("open_multiline",          test_open_multiline);
  run_test("open_nonexistent",        test_open_nonexistent);
  run_test("open_preserves_filename", test_open_preserves_filename);
  run_test("open_resets_cursor",      test_open_resets_cursor);
  run_test("open_large_file",         test_open_large_file);
  run_test("save_basic",              test_save_basic);
  run_test("save_empty_buffer",       test_save_empty_buffer);
  run_test("save_multiline",          test_save_multiline);
  run_test("save_overwrites",         test_save_overwrites_existing);
  run_test("roundtrip",               test_roundtrip);

  print_summary();
  return tc_suite_failed() > 0 ? 1 : 0;
}
