#include "view/editor/syntax.h"
#include <string.h>
#include <stdbool.h>

static const char *keywords[] = {
  "if", "else", "for", "while", "do", "switch", "case", "break",
  "continue", "return", "goto", "default",
  "sizeof", "typedef", "struct", "union", "enum",
  "static", "extern", "const", "volatile", "register", "auto",
  "inline", "void",
  NULL
};

static const char *types[] = {
  "int", "char", "float", "double", "long", "short", "unsigned", "signed",
  "bool", "uint8_t", "uint16_t", "uint32_t", "uint64_t",
  "int8_t", "int16_t", "int32_t", "int64_t",
  "size_t", "ssize_t", "ptrdiff_t", "uintptr_t", "intptr_t",
  "true", "false", "NULL",
  NULL
};

static bool is_alpha(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static bool is_digit(char c) { return c >= '0' && c <= '9'; }
static bool is_alnum(char c) { return is_alpha(c) || is_digit(c); }

static bool match_word(const char **list, const char *s, int len) {
  for (int i = 0; list[i]; i++) {
    int klen = (int)strlen(list[i]);
    if (klen == len && strncmp(list[i], s, len) == 0) return true;
  }
  return false;
}

/* Returns true if the word s[0..len) is ALL_CAPS (at least one letter, all uppercase/digits/underscore) */
static bool is_macro(const char *s, int len) {
  bool has_letter = false;
  for (int i = 0; i < len; i++) {
    if (s[i] >= 'a' && s[i] <= 'z') return false;
    if (s[i] >= 'A' && s[i] <= 'Z') has_letter = true;
    else if (!is_digit(s[i]) && s[i] != '_') return false;
  }
  return has_letter;
}

/* Returns true if c is an operator character.
 * Excludes '/' (comment-checked first), '.' and '-' (member access checked first). */
static bool is_operator(char c) {
  return c == '+' || c == '*' || c == '%' ||
         c == '=' || c == '!' || c == '<' || c == '>' || c == '&' ||
         c == '|' || c == '^' || c == '~' || c == '?';
}

SyntaxLanguage syntax_detect_language(const char *filename) {
  if (!filename) return SYNTAX_LANG_NONE;
  const char *dot = NULL;
  for (const char *p = filename; *p; p++)
    if (*p == '.') dot = p;
  if (!dot) return SYNTAX_LANG_NONE;
  if (strcmp(dot, ".c") == 0 || strcmp(dot, ".h") == 0) return SYNTAX_LANG_C;
  return SYNTAX_LANG_NONE;
}

/* Skips whitespace at position i, returns new i */
static int skip_ws(const char *line, int i, int len) {
  while (i < len && (line[i] == ' ' || line[i] == '\t')) i++;
  return i;
}

void syntax_highlight_line(const char *line, int len,
                           bool in_block_comment,
                           SyntaxLanguage lang,
                           uint32_t *colors,
                           bool *out_block_comment) {
  if (lang != SYNTAX_LANG_C) {
    if (colors)
      for (int i = 0; i < len; i++) colors[i] = SYNTAX_COLOR_DEFAULT;
    *out_block_comment = in_block_comment;
    return;
  }

  bool block_comment = in_block_comment;
  int i = 0;

  bool is_preproc = false;
  if (!block_comment) {
    int j = skip_ws(line, 0, len);
    if (j < len && line[j] == '#') is_preproc = true;
  }

  while (i < len) {
    /* Inside block comment */
    if (block_comment) {
      if (colors) colors[i] = SYNTAX_COLOR_COMMENT;
      if (line[i] == '*' && i + 1 < len && line[i + 1] == '/') {
        if (colors) colors[i + 1] = SYNTAX_COLOR_COMMENT;
        i += 2;
        block_comment = false;
      } else {
        i++;
      }
      continue;
    }

    /* Line comment */
    if (line[i] == '/' && i + 1 < len && line[i + 1] == '/') {
      if (colors)
        while (i < len) colors[i++] = SYNTAX_COLOR_COMMENT;
      break;
    }

    /* Block comment open */
    if (line[i] == '/' && i + 1 < len && line[i + 1] == '*') {
      if (colors) { colors[i] = SYNTAX_COLOR_COMMENT; colors[i + 1] = SYNTAX_COLOR_COMMENT; }
      i += 2;
      block_comment = true;
      continue;
    }

    /* String literal — escape sequences highlighted differently */
    if (line[i] == '"') {
      if (colors) colors[i] = SYNTAX_COLOR_STRING;
      i++;
      while (i < len) {
        if (line[i] == '\\' && i + 1 < len) {
          if (colors) { colors[i] = SYNTAX_COLOR_ESCAPE; colors[i + 1] = SYNTAX_COLOR_ESCAPE; }
          i += 2;
        } else if (line[i] == '"') {
          if (colors) colors[i] = SYNTAX_COLOR_STRING;
          i++;
          break;
        } else {
          if (colors) colors[i] = SYNTAX_COLOR_STRING;
          i++;
        }
      }
      continue;
    }

    /* Char literal — escape sequences highlighted differently */
    if (line[i] == '\'') {
      if (colors) colors[i] = SYNTAX_COLOR_STRING;
      i++;
      while (i < len) {
        if (line[i] == '\\' && i + 1 < len) {
          if (colors) { colors[i] = SYNTAX_COLOR_ESCAPE; colors[i + 1] = SYNTAX_COLOR_ESCAPE; }
          i += 2;
        } else if (line[i] == '\'') {
          if (colors) colors[i] = SYNTAX_COLOR_STRING;
          i++;
          break;
        } else {
          if (colors) colors[i] = SYNTAX_COLOR_STRING;
          i++;
        }
      }
      continue;
    }

    /* Number */
    if (is_digit(line[i])) {
      while (i < len && (is_alnum(line[i]) || line[i] == '.' || line[i] == '_')) {
        if (colors) colors[i] = SYNTAX_COLOR_NUMBER;
        i++;
      }
      continue;
    }

    /* Member access: .name or ->name — color the member identifier */
    if (line[i] == '.' && i + 1 < len && is_alpha(line[i + 1])) {
      if (colors) colors[i] = SYNTAX_COLOR_OPERATOR;
      i++;
      int start = i;
      while (i < len && is_alnum(line[i])) i++;
      if (colors)
        for (int k = start; k < i; k++) colors[k] = SYNTAX_COLOR_MEMBER;
      continue;
    }
    if (line[i] == '-' && i + 1 < len && line[i + 1] == '>' &&
        i + 2 < len && is_alpha(line[i + 2])) {
      if (colors) { colors[i] = SYNTAX_COLOR_OPERATOR; colors[i + 1] = SYNTAX_COLOR_OPERATOR; }
      i += 2;
      int start = i;
      while (i < len && is_alnum(line[i])) i++;
      if (colors)
        for (int k = start; k < i; k++) colors[k] = SYNTAX_COLOR_MEMBER;
      continue;
    }

    /* '-' not part of '->' — treat as operator */
    if (line[i] == '-') {
      if (colors) colors[i] = SYNTAX_COLOR_OPERATOR;
      i++;
      continue;
    }

    /* Operator */
    if (is_operator(line[i])) {
      if (colors) colors[i] = SYNTAX_COLOR_OPERATOR;
      i++;
      continue;
    }

    /* Preprocessor directive */
    if (is_preproc) {
      if (colors) colors[i] = SYNTAX_COLOR_PREPROC;
      i++;
      continue;
    }

    /* Identifier — function call, keyword, type, macro, or default */
    if (is_alpha(line[i])) {
      int start = i;
      while (i < len && is_alnum(line[i])) i++;
      int word_len = i - start;

      /* Look ahead past whitespace for '(' to detect function calls */
      int j = skip_ws(line, i, len);
      bool is_call = (j < len && line[j] == '(');

      if (colors) {
        uint32_t color;
        if (match_word(keywords, line + start, word_len))
          color = SYNTAX_COLOR_KEYWORD;
        else if (match_word(types, line + start, word_len))
          color = SYNTAX_COLOR_TYPE;
        else if (is_call)
          color = SYNTAX_COLOR_FUNCTION;
        else if (is_macro(line + start, word_len))
          color = SYNTAX_COLOR_MACRO;
        else
          color = SYNTAX_COLOR_DEFAULT;
        for (int k = start; k < i; k++) colors[k] = color;
      }
      continue;
    }

    /* Everything else */
    if (colors) colors[i] = SYNTAX_COLOR_DEFAULT;
    i++;
  }

  *out_block_comment = block_comment;
}
