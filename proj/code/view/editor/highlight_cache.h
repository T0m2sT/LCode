#pragma once

#include <lcom/lcf.h>
#include "view/editor/syntax.h"

/**
 * @file highlight_cache.h
 * @brief Per-line syntax color cache.
 *
 * Parses each line once via syntax_highlight_line() and caches the result,
 * tracking multi-line block-comment so edits recolor only what changed.
 */

/**
 * @brief Returns the cached color array for a row
 * @param row Line index (0-based)
 * @return One uint32_t per character. NULL for empty lines or allocation failure
 */
const uint32_t *highlight_cache_get_line(int row);

/**
 * @brief Sets the active language, invalidates all cached colors, and rebuilds
 *        block-comment state for the whole file
 * @param lang The syntax language to use for highlights
 */
void highlight_cache_set_language(SyntaxLanguage lang);

/**
 * @brief Updates the cache with a new row count after an edit
 *
 * Shifts cached color rows to match a single-line insertion or deletion at 
 * cursor_row. Full rebuild on multi-line edits
 * @param new_count New total row count
 * @param cursor_row Row where the insertion or deletion occurred
 */
void highlight_cache_sync(int new_count, int cursor_row);

/**
 * @brief Clears cached colors for a row
 * @param row Line index (0-based)
 */
void highlight_cache_free_row(int row);

/**
 * @brief Re-calculates from row to propagate block-comment state changes,
 *        stopping once the state stabilizes
 * @param row Start line index (0-based)
 */
void highlight_cache_rebuild_from(int row);

/**
 * @brief Clears cache
 */
void highlight_cache_cleanup(void);
