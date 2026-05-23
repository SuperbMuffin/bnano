#ifndef CONFIG_H
#define CONFIG_H

// Runtime configuration — loaded from ~/.config/bnano/bnano.conf
// All fields have defaults and the file is optional.

typedef struct
{
  // [editor]
  int tab_width;       // editor.tab_width       (default: 4)
  int smart_backspace; // editor.smart_backspace  (default: 1)

  // [ui]
  int show_line_numbers; // ui.show_line_numbers   (default: 0)
} Config;

// Global config instance — accessible from any module that includes config.h
extern Config g_config;

// Load config from ~/.config/bnano/bnano.conf.
// Silently succeeds if the file doesn't exist; bad lines are skipped.
void config_load(void);

// Reset all fields to their compiled-in defaults.
void config_defaults(void);

#endif // CONFIG_H
