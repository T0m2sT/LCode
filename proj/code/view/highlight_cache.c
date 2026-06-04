#include "view/highlight_cache.h"
#include "view/syntax.h"
#include "model/editor.h"
#include <string.h>
#include <stdlib.h>

static SyntaxLanguage current_lang = SYNTAX_LANG_NONE;
static bool *block_comment_open = NULL;
static int block_comment_open_cap = 0;

static uint32_t **line_colors_cache = NULL;
static int *line_colors_cache_cap = NULL;
static int line_colors_cache_count = 0;
static int line_colors_last_row_count = 0;


// ---------------
// Block comment
// ---------------

static int block_comment_open_grow(int needed) {
  //buffer size calc
  if (block_comment_open_cap >= needed) return 0;
  int new_cap = block_comment_open_cap ? block_comment_open_cap * 2 : 64;
  while (new_cap < needed) new_cap *= 2;

  //buffer allocation
  bool *p = malloc(new_cap * sizeof(bool));
  if (!p) return -1;
  memcpy(p, block_comment_open, block_comment_open_cap * sizeof(bool));

  memset(p + block_comment_open_cap, 0, (new_cap - block_comment_open_cap) * sizeof(bool));

  free(block_comment_open);
  block_comment_open = p;
  block_comment_open_cap = new_cap;
  return 0;
}

static void block_comment_open_cleanup(void) {
  free(block_comment_open);
  block_comment_open = NULL;
  block_comment_open_cap = 0;
}

static bool block_comment_open_at(int row) {
  if (row < 0 || row >= block_comment_open_cap) return false;
  return block_comment_open[row];
}

static void rebuild_block_comment_open_full(void) {
  int total = editor_get_row_count();
  if (block_comment_open_grow(total + 1) != 0) return;

  //before first line can't be commented
  bool in_block_comment = false;

  for (int r = 0; r < total; r++) {
    //check state before reading line
    block_comment_open[r] = in_block_comment;
    bool out_bc;
    syntax_highlight_line(editor_get_line(r), editor_get_line_len(r), in_block_comment, current_lang, NULL, &out_bc);
    in_block_comment = out_bc;
  }
}


// ---------------
// Color cache
// ---------------

static int line_colors_cache_grow(int needed) {
  if (line_colors_cache_count >= needed) return 0;

  int new_count = line_colors_cache_count ? line_colors_cache_count * 2 : 64;
  while (new_count < needed) new_count *= 2;

  //Colors
  uint32_t **p = malloc(new_count * sizeof(uint32_t *));
  if (!p) return -1;

  //amount of colors per line
  int *q = malloc(new_count * sizeof(int));
  if (!q) { free(p); return -1; }

  //Copy existing to new
  if (line_colors_cache_count > 0) {
    memcpy(p, line_colors_cache, line_colors_cache_count * sizeof(uint32_t *));
    memcpy(q, line_colors_cache_cap, line_colors_cache_count * sizeof(int));
  }

  //zero-out mem
  memset(p + line_colors_cache_count, 0, (new_count - line_colors_cache_count) * sizeof(uint32_t *));
  memset(q + line_colors_cache_count, 0, (new_count - line_colors_cache_count) * sizeof(int));

  free(line_colors_cache);
  free(line_colors_cache_cap);
  line_colors_cache = p;
  line_colors_cache_cap = q;
  line_colors_cache_count = new_count;
  return 0;
}

void highlight_cache_free_row(int row) {
  if (row < 0 || row >= line_colors_cache_count) return;
  free(line_colors_cache[row]);
  line_colors_cache[row] = NULL;
  line_colors_cache_cap[row] = 0;
}

// bulk clear
static void line_colors_cache_invalidate_all(void) {
  for (int r = 0; r < line_colors_cache_count; r++) {
    highlight_cache_free_row(r);
  }
  line_colors_last_row_count = 0;
}

/* Ensures row's cache buffer holds at least len colors and returns it. NULL on error or empty lines */
static uint32_t *ensure_row_cap(int row, int len) {
  if (len <= 0) return NULL;

  if (line_colors_cache_grow(row + 1) != 0) return NULL;
  if (line_colors_cache_cap[row] < len) {
    uint32_t *p = malloc(len * sizeof(uint32_t));
    if (!p) return NULL;
    
    free(line_colors_cache[row]);
    line_colors_cache[row] = p;
    line_colors_cache_cap[row] = len;
  }
  return line_colors_cache[row];
}


// ---------------
// Highlight Cache API
// ---------------

void highlight_cache_cleanup(void) {
  line_colors_cache_invalidate_all();
  free(line_colors_cache);
  line_colors_cache = NULL;
  free(line_colors_cache_cap);
  line_colors_cache_cap = NULL;
  line_colors_cache_count = 0;
  block_comment_open_cleanup();
}

void highlight_cache_sync(int new_count, int cursor_row) {
  int old_count = line_colors_last_row_count;
  if (new_count == old_count) return;

  //nothing to shift, cache empty
  if (old_count == 0) {
    line_colors_last_row_count = new_count;
    return;
  }

  //one line inserted
  if (new_count == old_count + 1) {
    int split_row = cursor_row - 1;
    if (split_row >= 0 && line_colors_cache_grow(new_count) == 0) {
      int shift = old_count - split_row - 1;
      if (shift > 0) {
        memmove(&line_colors_cache[split_row + 2], &line_colors_cache[split_row + 1], shift * sizeof(uint32_t *));
        memmove(&line_colors_cache_cap[split_row + 2], &line_colors_cache_cap[split_row + 1], shift * sizeof(int));
      }

      //top half stale
      highlight_cache_free_row(split_row);
      line_colors_cache[split_row + 1] = NULL;
      line_colors_cache_cap[split_row + 1] = 0;
    }
  }

  //one line deleted
  else if (new_count == old_count - 1) {
    int merge_row = cursor_row;

    //clamp to the range we actually allocated to avoid out-of-bounds memmove
    int eff_old = old_count < line_colors_cache_count ? old_count : line_colors_cache_count;
    if (merge_row >= 0 && merge_row < eff_old) {
      highlight_cache_free_row(merge_row);
      highlight_cache_free_row(merge_row + 1);
      int shift = eff_old - merge_row - 2;
      if (shift > 0) {
        memmove(&line_colors_cache[merge_row + 1], &line_colors_cache[merge_row + 2], shift * sizeof(uint32_t *));
        memmove(&line_colors_cache_cap[merge_row + 1], &line_colors_cache_cap[merge_row + 2], shift * sizeof(int));
      }

      //null the vacated last slot
      int vacated = eff_old - 1;
      if (vacated >= 0 && vacated < line_colors_cache_count) {
        line_colors_cache[vacated] = NULL;
        line_colors_cache_cap[vacated] = 0;
      }
    }
  }
  //bulk change (paste, undo, multi-line delete)
  else {
    line_colors_cache_invalidate_all();
  }

  line_colors_last_row_count = new_count;
}

const uint32_t *highlight_cache_get_line(int row) {
  //ensure outer pointer array covers this row
  if (line_colors_cache_grow(row + 1) != 0) return NULL;

  //check colors already computed for this row
  if (line_colors_cache[row]) return line_colors_cache[row];

  int len = editor_get_line_len(row);
  uint32_t *dst = ensure_row_cap(row, len);
  if (!dst) return NULL;

  //parse line and store result;
  bool out_bc;
  syntax_highlight_line(editor_get_line(row), len, block_comment_open_at(row), current_lang, dst, &out_bc);
  return dst;
}

void highlight_cache_set_language(SyntaxLanguage lang) {
  current_lang = lang;
  line_colors_cache_invalidate_all();
  rebuild_block_comment_open_full();
}

void highlight_cache_rebuild_from(int row) {
  int total = editor_get_row_count();
  if (row < 0 || row >= total) return;
  if (block_comment_open_grow(total + 1) != 0) return;

  //check state before reading line
  bool in_block_comment = block_comment_open[row];

  for (int r = row; r < total; r++) {
    int len = editor_get_line_len(r);
    bool out_bc;

    uint32_t *dst = ensure_row_cap(r, len);
    //parses line and stores whether comment block is still open at EOL
    syntax_highlight_line(editor_get_line(r), len, in_block_comment, current_lang, dst, &out_bc);

    if (r + 1 < block_comment_open_cap) {
      // next lines state already correct
      if (block_comment_open[r + 1] == out_bc) break;

      highlight_cache_free_row(r + 1);
      block_comment_open[r + 1] = out_bc;
    }

    in_block_comment = out_bc;
  }
}
