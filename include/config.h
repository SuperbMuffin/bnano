#ifndef CONFIG_H
#define CONFIG_H

// Runtime configuration — loaded from ~/.config/bnano/bnano.conf
// All fields have defaults and the file is optional.

typedef enum
{
  LINE_NUMBERS_NONE,     // ui.line_numbers: none
  LINE_NUMBERS_ABSOLUTE, // ui.line_numbers: absolute
  LINE_NUMBERS_RELATIVE, // ui.line_numbers: relative
  LINE_NUMBERS_BOTH,     // ui.line_numbers: both  (absolute on cursor, relative elsewhere)
} LineNumberMode;

typedef struct
{
  // [editor]
  int tab_width;       // editor.tab_width       (default: 4)
  int smart_backspace; // editor.smart_backspace  (default: 1)

  // [ui]
  LineNumberMode line_numbers; // ui.line_numbers  (default: none)
} Config;

// Global config instance — accessible from any module that includes config.h
extern Config g_config;

// Load config from ~/.config/bnano/bnano.conf.
// Silently succeeds if the file doesn't exist; bad lines are skipped.
void config_load(void);

// Reset all fields to their compiled-in defaults.
void config_defaults(void);

#endif // CONFIG_H
