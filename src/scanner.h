#ifndef WISP_SCANNER_H
#define WISP_SCANNER_H

enum token_type {
  // Single-character tokens.
  TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN, TOKEN_DOT,

  // Primitives.
  PRIMITIVE_START,

    TOKEN_DEFINE, TOKEN_LAMBDA, TOKEN_QUOTE,
    TOKEN_CONS, TOKEN_CAR, TOKEN_CDR,

  PRIMITIVE_END,

  // Literals.
  TOKEN_IDENTIFIER, TOKEN_NUMBER,

  // Special tokens.
  TOKEN_ERROR, TOKEN_EOF,
};

struct token {
  enum token_type type;
  const char *start;
  int length;
  int line;
};

struct scanner {
  const char *start;
  const char *current;
  int line;
};

void scanner_init(struct scanner *, const char *);

struct token scanner_next(struct scanner *);

#endif
