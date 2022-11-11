#ifndef WISP_SCANNER_H
#define WISP_SCANNER_H

#define IS_PRIMITIVE(type) \
  (_PRIMITIVE_START <= (type) && (type) <= _PRIMITIVE_END)

enum token_type {
  // Single-character tokens.
  TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN, TOKEN_DOT,

  // Primitives.
  _PRIMITIVE_START,

    TOKEN_DEFINE, TOKEN_LAMBDA, TOKEN_QUOTE,
    TOKEN_CONS, TOKEN_CAR, TOKEN_CDR,

  _PRIMITIVE_END,

  // Literals.
  TOKEN_IDENTIFIER, TOKEN_NUMBER,

  // Special tokens.
  TOKEN_ERROR, TOKEN_EOF,
};

struct token {
  // Type of the token.
  enum token_type type;

  // Start of the token.
  const char *start;

  // Length of the token.
  int len;

  // Number of the line the token appears in.
  int line;
};

struct scanner {
  // Start of the currently scanned token.
  const char *start;

  // Current position in the buffer.
  const char *current;

  // Currently scanned line.
  int line;
};

void scanner_init(struct scanner *, const char *);

struct token scanner_next(struct scanner *);

#endif
