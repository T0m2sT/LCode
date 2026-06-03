#pragma once

#include <lcom/lcf.h>
#include "model/editor.h"

/* Token color palette — One Dark Pro */
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

typedef enum {
  SYNTAX_LANG_NONE,
  SYNTAX_LANG_C
} SyntaxLanguage;

SyntaxLanguage syntax_detect_language(const char *filename);

/* Fills oolors with one uint32_t color per character of line */
/* in_block_comment is true if the line starts inside a comment block */
/* out_block_comment is set to true if the line ends still inside a block comment */
void syntax_highlight_line(const char *line, int len,
                           bool in_block_comment,
                           SyntaxLanguage lang,
                           uint32_t *colors,
                           bool *out_block_comment);
