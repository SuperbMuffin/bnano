#include "test_common.h"
#include "command.h"
#include "buffer.h"
#include "fileio.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* terminal stubs */

static int mock_cmd_run_called = 0;
static int mock_cmd_val = 0;

static int mock_cmd(Buffer *buf, const char *args)
{
  (void) buf;
  (void) args;
  mock_cmd_run_called = 1;
  return mock_cmd_val;
}

static void test_register_and_run(void)
{
  Buffer b;
  buffer_init(&b);
  
  mock_cmd_run_called = 0;
  mock_cmd_val = 42;
  
  command_register("test", mock_cmd);
  int result = command_run(&b, "test");
  
  ASSERT_EQ_INT(mock_cmd_run_called, 1, "mock command was called");
  ASSERT_EQ_INT(result, 42, "command returned correct value");
  
  buffer_free(&b);
}

static char last_args[64];
static int mock_arg_cmd(Buffer *buf, const char *args)
{
  (void) buf;
  strncpy(last_args, args, sizeof(last_args) - 1);
  return 0;
}

static void test_command_with_args(void)
{
  Buffer b;
  buffer_init(&b);
  
  command_register("argtest", mock_arg_cmd);
  command_run(&b, "argtest foo bar");
  
  ASSERT_EQ_STR(last_args, "foo bar", "args were passed correctly");
  
  buffer_free(&b);
}

static void test_unknown_command(void)
{
  Buffer b;
  buffer_init(&b);
  
  int result = command_run(&b, "nonexistent");
  ASSERT_EQ_INT(result, 0, "unknown command returns 0");
  ASSERT(strstr(b.statusmsg, "Not a command") != NULL, "status message set for unknown command");
  
  buffer_free(&b);
}

static void test_default_quit(void)
{
  Buffer b;
  buffer_init(&b);
  command_register_defaults();
  
  // Clean buffer
  int result = command_run(&b, "q");
  ASSERT_EQ_INT(result, 1, "quit returns 1 when clean");
  
  // Dirty buffer
  b.dirty = 1;
  result = command_run(&b, "q");
  ASSERT_EQ_INT(result, 0, "quit returns 0 when dirty");
  ASSERT(strstr(b.statusmsg, "Unsaved changes") != NULL, "warning message set");
  
  // Force quit
  result = command_run(&b, "q!");
  ASSERT_EQ_INT(result, 1, "force quit returns 1 even if dirty");
  
  buffer_free(&b);
}

static void test_default_write(void)
{
  Buffer b;
  buffer_init(&b);
  command_register_defaults();
  
  buffer_set_filename(&b, "test_output.txt");
  buffer_insert_char(&b, 'h');
  b.dirty = 1;
  
  int result = command_run(&b, "w");
  ASSERT_EQ_INT(result, 0, "write returns 0");
  ASSERT_EQ_INT(b.dirty, 0, "buffer no longer dirty after write");
  ASSERT(strstr(b.statusmsg, "Written") != NULL, "status message set");
  
  // Check if file exists and has content
  FILE *f = fopen("test_output.txt", "r");
  ASSERT(f != NULL, "output file was created");
  if (f) {
    char ch = fgetc(f);
    ASSERT_EQ_CHAR(ch, 'h', "file content is correct");
    fclose(f);
    remove("test_output.txt");
  }
  
  buffer_free(&b);
}

static void test_default_file_rename(void)
{
  Buffer b;
  buffer_init(&b);
  command_register_defaults();
  
  command_run(&b, "file newname.c");
  ASSERT_EQ_STR(b.filename, "newname.c", "filename updated via :file");
  
  command_run(&b, "f other.c");
  ASSERT_EQ_STR(b.filename, "other.c", "filename updated via :f");
  
  buffer_free(&b);
}

static void test_default_wq(void)
{
  Buffer b;
  buffer_init(&b);
  command_register_defaults();
  
  buffer_set_filename(&b, "test_wq.txt");
  buffer_insert_char(&b, 'x');
  b.dirty = 1;
  
  int result = command_run(&b, "wq");
  ASSERT_EQ_INT(result, 1, "wq returns 1 (quit)");
  ASSERT_EQ_INT(b.dirty, 0, "buffer no longer dirty after wq");
  
  FILE *f = fopen("test_wq.txt", "r");
  ASSERT(f != NULL, "wq created file");
  if (f) {
    ASSERT_EQ_CHAR(fgetc(f), 'x', "content is correct");
    fclose(f);
    remove("test_wq.txt");
  }
  
  buffer_free(&b);
}

int main(void)
{
  run_test("register_and_run",    test_register_and_run);
  run_test("command_with_args",   test_command_with_args);
  run_test("unknown_command",     test_unknown_command);
  run_test("default_quit",        test_default_quit);
  run_test("default_write",       test_default_write);
  run_test("default_file_rename", test_default_file_rename);
  run_test("default_wq",          test_default_wq);
  
  print_summary();
  return tc_suite_failed() > 0 ? 1 : 0;
}
