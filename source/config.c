#define _POSIX_C_SOURCE 200809L

#include "config.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Config g_config;

void config_defaults(void)
{
  g_config.tab_width = 4;
  g_config.smart_backspace = 1;
  g_config.line_numbers = LINE_NUMBERS_NONE;
}

// ---------------------------------------------------------------------------
// Parsing helpers
// ---------------------------------------------------------------------------

static char *trim(char *s)
{
  while (isspace((unsigned char) *s))
    s++;
  char *end = s + strlen(s);
  while (end > s && isspace((unsigned char) *(end - 1)))
    end--;
  *end = '\0';
  return s;
}

static void apply(const char *section, const char *key, const char *value)
{
  if (strcmp(section, "editor") == 0)
  {
    if (strcmp(key, "tab_width") == 0)
    {
      int v = atoi(value);
      if (v >= 1 && v <= 16)
        g_config.tab_width = v;
    }
    else if (strcmp(key, "smart_backspace") == 0)
    {
      g_config.smart_backspace = (strcmp(value, "true") == 0 || strcmp(value, "1") == 0) ? 1 : 0;
    }
  }
  else if (strcmp(section, "ui") == 0)
  {
    if (strcmp(key, "line_numbers") == 0)
    {
      if (strcmp(value, "absolute") == 0)
        g_config.line_numbers = LINE_NUMBERS_ABSOLUTE;
      else if (strcmp(value, "relative") == 0)
        g_config.line_numbers = LINE_NUMBERS_RELATIVE;
      else if (strcmp(value, "both") == 0)
        g_config.line_numbers = LINE_NUMBERS_BOTH;
      else
        g_config.line_numbers = LINE_NUMBERS_NONE;
    }
  }
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void config_load(void)
{
  config_defaults();

  const char *home = getenv("HOME");
  if (home == NULL)
    return;

  // Build path: ~/.config/bnano/bnano.conf
  char path[4096];
  snprintf(path, sizeof(path), "%s/.config/bnano/bnano.conf", home);

  FILE *f = fopen(path, "r");
  if (f == NULL)
    return; // file is optional

  char line[256];
  char section[64] = "";

  while (fgets(line, sizeof(line), f) != NULL)
  {
    char *s = trim(line);

    // Skip blank lines and comments
    if (*s == '\0' || *s == '#')
      continue;

    // Section header: [editor]
    if (*s == '[')
    {
      char *end = strchr(s + 1, ']');
      if (end == NULL)
        continue;
      size_t len = (size_t) (end - (s + 1));
      if (len >= sizeof(section))
        len = sizeof(section) - 1;
      memcpy(section, s + 1, len);
      section[len] = '\0';
      continue;
    }

    // key = value  or  key: value
    char *eq = strchr(s, '=');
    if (eq == NULL)
      eq = strchr(s, ':');
    if (eq == NULL)
      continue;

    *eq = '\0';
    char *raw_key = trim(s);
    char *raw_val = trim(eq + 1);

    // Support dotted section.key syntax (overrides current section)
    char dot_section[64] = "";
    char dot_key[64] = "";
    char *dot = strchr(raw_key, '.');
    if (dot != NULL)
    {
      size_t slen = (size_t) (dot - raw_key);
      if (slen >= sizeof(dot_section))
        slen = sizeof(dot_section) - 1;
      memcpy(dot_section, raw_key, slen);
      dot_section[slen] = '\0';

      size_t klen = strlen(dot + 1);
      if (klen >= sizeof(dot_key))
        klen = sizeof(dot_key) - 1;
      memcpy(dot_key, dot + 1, klen);
      dot_key[klen] = '\0';

      apply(dot_section, dot_key, raw_val);
    }
    else if (section[0] != '\0')
    {
      apply(section, raw_key, raw_val);
    }
    // else: key with no section and no dot — skip
  }

  fclose(f);
}
