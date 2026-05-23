// test_config.c — test suite for config.c
//
// Adding a new test:
//   1. Write a static void test_xxx(void) function using the ASSERT/CFG macros.
//   2. Call run_test("xxx", test_xxx) in main().
//   That's it — no build changes needed.
//
// Writing config content:
//   Use WRITE_CONFIG(str) to write an INI string to a temp file and load it.
//   The macro handles setup/teardown automatically within the test function.
//
// Available assertions (from test_common.h):
//   ASSERT(cond, msg)
//   ASSERT_EQ_INT(a, b, msg)
//   ASSERT_EQ_STR(a, b, msg)
//
// Config-specific shorthand:
//   CFG_INT(field, expected, msg)   — checks g_config.field == expected
//   CFG_TRUE(field, msg)            — checks g_config.field is truthy
//   CFG_FALSE(field, msg)           — checks g_config.field is falsy

#include "config.h"
#include "test_common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Write content to a temp file, point $HOME at its parent dir so config_load()
// finds it at $HOME/.config/bnano/bnano.conf, then load it.
// Returns the path to the temp dir (caller must free + remove).
static char *load_config_str(const char *content)
{
  // Build a temp dir tree: /tmp/bnano_test_XXXXXX/.config/bnano/
  char tmpl[] = "/tmp/bnano_test_XXXXXX";
  char *tmpdir = mkdtemp(tmpl);
  if (!tmpdir)
    return NULL;

  char confdir[512], confpath[528];
  snprintf(confdir, sizeof(confdir), "%s/.config/bnano", tmpdir);
  snprintf(confpath, sizeof(confpath), "%s/bnano.conf", confdir);

  // mkdir -p the config dir
  char cmd[600];
  snprintf(cmd, sizeof(cmd), "mkdir -p '%s'", confdir);
  system(cmd);

  FILE *f = fopen(confpath, "w");
  if (!f)
    return NULL;
  fputs(content, f);
  fclose(f);

  setenv("HOME", tmpdir, 1);
  config_load();

  return strdup(tmpdir);
}

static void cleanup_config_dir(char *tmpdir)
{
  if (!tmpdir)
    return;
  char cmd[512];
  snprintf(cmd, sizeof(cmd), "rm -rf '%s'", tmpdir);
  system(cmd);
  free(tmpdir);
}

// Convenience macro: write a config string, run assertions, clean up.
#define WRITE_CONFIG(str)               \
  char *_tmpdir = load_config_str(str); \
  ASSERT(_tmpdir != NULL, "failed to write temp config")

#define CLEANUP_CONFIG() cleanup_config_dir(_tmpdir)

// Shorthand assertion macros for g_config fields.
#define CFG_INT(field, expected, msg) ASSERT_EQ_INT(g_config.field, expected, msg)
#define CFG_TRUE(field, msg) ASSERT(g_config.field, msg)
#define CFG_FALSE(field, msg) ASSERT(!g_config.field, msg)

// ---------------------------------------------------------------------------
// Defaults
// ---------------------------------------------------------------------------

static void test_defaults(void)
{
  config_defaults();
  CFG_INT(tab_width, 4, "default tab_width is 4");
  CFG_TRUE(smart_backspace, "default smart_backspace is on");
  CFG_INT(line_numbers, LINE_NUMBERS_NONE, "default line_numbers is none");
}

static void test_missing_file_uses_defaults(void)
{
  // Point HOME somewhere with no config file.
  char tmpl[] = "/tmp/bnano_empty_XXXXXX";
  char *tmpdir = mkdtemp(tmpl);
  ASSERT(tmpdir != NULL, "mkdtemp failed");
  setenv("HOME", tmpdir, 1);

  config_load();

  CFG_INT(tab_width, 4, "missing file: tab_width default");
  CFG_TRUE(smart_backspace, "missing file: smart_backspace default");
  CFG_INT(line_numbers, LINE_NUMBERS_NONE, "missing file: line_numbers default");

  char cmd[256];
  snprintf(cmd, sizeof(cmd), "rm -rf '%s'", tmpdir);
  system(cmd);
}

// ---------------------------------------------------------------------------
// [editor] section
// ---------------------------------------------------------------------------

static void test_tab_width(void)
{
  WRITE_CONFIG("[editor]\ntab_width = 2\n");
  CFG_INT(tab_width, 2, "tab_width parsed as 2");
  CLEANUP_CONFIG();
}

static void test_tab_width_max(void)
{
  WRITE_CONFIG("[editor]\ntab_width = 16\n");
  CFG_INT(tab_width, 16, "tab_width accepts max value 16");
  CLEANUP_CONFIG();
}

static void test_tab_width_too_large_ignored(void)
{
  config_defaults();
  WRITE_CONFIG("[editor]\ntab_width = 99\n");
  CFG_INT(tab_width, 4, "tab_width out of range keeps default");
  CLEANUP_CONFIG();
}

static void test_tab_width_zero_ignored(void)
{
  config_defaults();
  WRITE_CONFIG("[editor]\ntab_width = 0\n");
  CFG_INT(tab_width, 4, "tab_width = 0 keeps default");
  CLEANUP_CONFIG();
}

static void test_smart_backspace_true(void)
{
  WRITE_CONFIG("[editor]\nsmart_backspace = true\n");
  CFG_TRUE(smart_backspace, "smart_backspace = true");
  CLEANUP_CONFIG();
}

static void test_smart_backspace_false(void)
{
  WRITE_CONFIG("[editor]\nsmart_backspace = false\n");
  CFG_FALSE(smart_backspace, "smart_backspace = false");
  CLEANUP_CONFIG();
}

static void test_smart_backspace_1(void)
{
  WRITE_CONFIG("[editor]\nsmart_backspace = 1\n");
  CFG_TRUE(smart_backspace, "smart_backspace = 1");
  CLEANUP_CONFIG();
}

static void test_smart_backspace_0(void)
{
  WRITE_CONFIG("[editor]\nsmart_backspace = 0\n");
  CFG_FALSE(smart_backspace, "smart_backspace = 0");
  CLEANUP_CONFIG();
}

// ---------------------------------------------------------------------------
// [ui] section
// ---------------------------------------------------------------------------

static void test_line_numbers_none(void)
{
  WRITE_CONFIG("[ui]\nline_numbers = none\n");
  CFG_INT(line_numbers, LINE_NUMBERS_NONE, "line_numbers = none");
  CLEANUP_CONFIG();
}

static void test_line_numbers_absolute(void)
{
  WRITE_CONFIG("[ui]\nline_numbers = absolute\n");
  CFG_INT(line_numbers, LINE_NUMBERS_ABSOLUTE, "line_numbers = absolute");
  CLEANUP_CONFIG();
}

static void test_line_numbers_relative(void)
{
  WRITE_CONFIG("[ui]\nline_numbers = relative\n");
  CFG_INT(line_numbers, LINE_NUMBERS_RELATIVE, "line_numbers = relative");
  CLEANUP_CONFIG();
}

static void test_line_numbers_both(void)
{
  WRITE_CONFIG("[ui]\nline_numbers = both\n");
  CFG_INT(line_numbers, LINE_NUMBERS_BOTH, "line_numbers = both");
  CLEANUP_CONFIG();
}

static void test_line_numbers_invalid_ignored(void)
{
  config_defaults();
  WRITE_CONFIG("[ui]\nline_numbers = sideways\n");
  CFG_INT(line_numbers, LINE_NUMBERS_NONE, "invalid line_numbers keeps default");
  CLEANUP_CONFIG();
}

// ---------------------------------------------------------------------------
// Dotted key syntax  (section.key = value, no [section] header needed)
// ---------------------------------------------------------------------------

static void test_dotted_tab_width(void)
{
  WRITE_CONFIG("editor.tab_width = 8\n");
  CFG_INT(tab_width, 8, "dotted syntax: editor.tab_width");
  CLEANUP_CONFIG();
}

static void test_dotted_line_numbers(void)
{
  WRITE_CONFIG("ui.line_numbers = relative\n");
  CFG_INT(line_numbers, LINE_NUMBERS_RELATIVE, "dotted syntax: ui.line_numbers");
  CLEANUP_CONFIG();
}

// ---------------------------------------------------------------------------
// Comments and blank lines
// ---------------------------------------------------------------------------

static void test_comments_ignored(void)
{
  WRITE_CONFIG("# this is a comment\n"
               "[editor]\n"
               "# another comment\n"
               "tab_width = 3\n");
  CFG_INT(tab_width, 3, "comments do not interfere");
  CLEANUP_CONFIG();
}

static void test_blank_lines_ignored(void)
{
  WRITE_CONFIG("\n"
               "[editor]\n"
               "\n"
               "tab_width = 6\n"
               "\n");
  CFG_INT(tab_width, 6, "blank lines do not interfere");
  CLEANUP_CONFIG();
}

// ---------------------------------------------------------------------------
// Multiple sections / keys in one file
// ---------------------------------------------------------------------------

static void test_full_config(void)
{
  WRITE_CONFIG("[editor]\n"
               "tab_width = 2\n"
               "smart_backspace = false\n"
               "[ui]\n"
               "line_numbers = both\n");
  CFG_INT(tab_width, 2, "full config: tab_width");
  CFG_FALSE(smart_backspace, "full config: smart_backspace off");
  CFG_INT(line_numbers, LINE_NUMBERS_BOTH, "full config: line_numbers both");
  CLEANUP_CONFIG();
}

static void test_later_value_wins(void)
{
  // Two assignments to the same key — last one wins.
  WRITE_CONFIG("[editor]\n"
               "tab_width = 2\n"
               "tab_width = 8\n");
  CFG_INT(tab_width, 8, "later value wins");
  CLEANUP_CONFIG();
}

// ---------------------------------------------------------------------------
// Colon separator
// ---------------------------------------------------------------------------

static void test_colon_separator(void)
{
  WRITE_CONFIG("[editor]\ntab_width: 3\n");
  CFG_INT(tab_width, 3, "colon separator parsed correctly");
  CLEANUP_CONFIG();
}

// ---------------------------------------------------------------------------
// Whitespace tolerance
// ---------------------------------------------------------------------------

static void test_whitespace_around_equals(void)
{
  WRITE_CONFIG("[editor]\ntab_width   =   5\n");
  CFG_INT(tab_width, 5, "whitespace around = is tolerated");
  CLEANUP_CONFIG();
}

static void test_unknown_key_ignored(void)
{
  config_defaults();
  WRITE_CONFIG("[editor]\nnonexistent_key = 99\ntab_width = 3\n");
  CFG_INT(tab_width, 3, "unknown key skipped, known key still parsed");
  CLEANUP_CONFIG();
}

static void test_unknown_section_ignored(void)
{
  config_defaults();
  WRITE_CONFIG("[unknown_section]\ntab_width = 7\n");
  CFG_INT(tab_width, 4, "unknown section is ignored, default preserved");
  CLEANUP_CONFIG();
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(void)
{
  // defaults
  run_test("defaults", test_defaults);
  run_test("missing_file_uses_defaults", test_missing_file_uses_defaults);

  // [editor]
  run_test("tab_width", test_tab_width);
  run_test("tab_width_max", test_tab_width_max);
  run_test("tab_width_too_large_ignored", test_tab_width_too_large_ignored);
  run_test("tab_width_zero_ignored", test_tab_width_zero_ignored);
  run_test("smart_backspace_true", test_smart_backspace_true);
  run_test("smart_backspace_false", test_smart_backspace_false);
  run_test("smart_backspace_1", test_smart_backspace_1);
  run_test("smart_backspace_0", test_smart_backspace_0);

  // [ui]
  run_test("line_numbers_none", test_line_numbers_none);
  run_test("line_numbers_absolute", test_line_numbers_absolute);
  run_test("line_numbers_relative", test_line_numbers_relative);
  run_test("line_numbers_both", test_line_numbers_both);
  run_test("line_numbers_invalid_ignored", test_line_numbers_invalid_ignored);

  // dotted syntax
  run_test("dotted_tab_width", test_dotted_tab_width);
  run_test("dotted_line_numbers", test_dotted_line_numbers);

  // formatting / robustness
  run_test("comments_ignored", test_comments_ignored);
  run_test("blank_lines_ignored", test_blank_lines_ignored);
  run_test("colon_separator", test_colon_separator);
  run_test("whitespace_around_equals", test_whitespace_around_equals);
  run_test("unknown_key_ignored", test_unknown_key_ignored);
  run_test("unknown_section_ignored", test_unknown_section_ignored);

  // multi-key / ordering
  run_test("full_config", test_full_config);
  run_test("later_value_wins", test_later_value_wins);

  print_summary();
  return tc_suite_failed() > 0 ? 1 : 0;
}
