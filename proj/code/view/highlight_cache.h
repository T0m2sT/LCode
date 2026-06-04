#pragma once

#include <lcom/lcf.h>
#include "view/syntax.h"

/* File goal: Per-line syntax color cache.
 * Tokenizes each line once via syntax_highlight_line() and caches the result,
 * tracking multi-line block-comment state so edits recolor only what changed. */

/* Returns the cached color array for a row. Returns NULL for empty lines or on allocation failure. */
const uint32_t *highlight_cache_get_line(int row);

/* Sets the active language, invalidates all cached colors, and rebuilds the
 * block-comment state for the whole file. */
void highlight_cache_set_language(SyntaxLanguage lang);

/* Updates cache to reflect file change */
void highlight_cache_sync(int new_count, int cursor_row);

/* Drops the cached colors for a single row */
void highlight_cache_free_row(int row);

/* Re-calculates colors from row downward because of block-comment state changes. Updates cache for touched rows. */
void highlight_cache_rebuild_from(int row);

/* Clears cache */
void highlight_cache_cleanup(void);
