#pragma once

#include <lcom/lcf.h>
#include "model/editor.h"

/**
 * @file syntax.h
 * @brief Syntax tokenizer and color palette for the Color theme - In our case, One Dark Pro
 */

/** @defgroup syntax_colors Syntax color palette (One Dark Pro)
 * @{
 */
#define SYNTAX_COLOR_DEFAULT    0xABB2BF
#define SYNTAX_COLOR_KEYWORD    0xC678DD
#define SYNTAX_COLOR_TYPE       0x56B6C2
#define SYNTAX_COLOR_PREPROC    0xE06C75
#define SYNTAX_COLOR_STRING     0x98C379
#define SYNTAX_COLOR_COMMENT    0x5C6370
#define SYNTAX_COLOR_NUMBER     0xD19A66
#define SYNTAX_COLOR_FUNCTION   0x61AFEF
#define SYNTAX_COLOR_MACRO      0xD19A66
#define SYNTAX_COLOR_ESCAPE     0x56B6C2
#define SYNTAX_COLOR_MEMBER     0xE06C75
#define SYNTAX_COLOR_OPERATOR   0xABB2BF
/** @} */

typedef enum {
  SYNTAX_LANG_NONE,
  SYNTAX_LANG_C
} SyntaxLanguage;

/**
 * @brief Detects the syntax language from filename (extension) 
 * @param filename Filename to inspect.
 * @return Detected SyntaxLanguage. SYNTAX_LANG_NONE if none recognized - In LCode's case, any other than C
 */
SyntaxLanguage syntax_detect_language(const char *filename);

/**
 * @brief Parses a line and fills a color array with one uint32_t per character
 * @param line Pointer to the line text (not null-terminated)
 * @param len Number of characters in the line
 * @param in_block_comment true if the line starts inside an open block comment
 * @param lang Language to use for tokenization
 * @param colors Output buffer of at least len uint32_t values. Or NULL to
 * compute block-comment state only without writing colors
 * @param out_block_comment Set to true if the line ends inside an open block comment
 */
void syntax_highlight_line(const char *line, int len, bool in_block_comment, SyntaxLanguage lang, uint32_t *colors, bool *out_block_comment);
