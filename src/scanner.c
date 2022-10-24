#include <stdbool.h>
#include <string.h>

#include "scanner.h"

void scanner_init(struct scanner *sc, const char *source)
{
  sc->start = source;
  sc->current = source;
  sc->line = 1;
}

static bool is_digit(char c)
{
  return '0' <= c && c <= '9';
}

static bool is_valid_identifier(char c)
{
  return 0x21 <= c && c <= 0x7E  // Printable.
    && c != '(' && c != ')' && c != '\'' && c != '.';  // Distinguishable.
}

static struct token make_token(struct scanner *sc, enum token_type type)
{
  struct token token;
  token.type = type;
  token.start = sc->start;
  token.length = sc->current - sc->start;
  token.line = sc->line;
  return token;
}

static struct token error_token(struct scanner *sc, const char *msg)
{
  struct token token;
  token.type = TOKEN_ERROR;
  token.start = msg;
  token.length = strlen(msg);
  token.line = sc->line;
  return token;
}

static bool is_at_end(struct scanner *sc)
{
  return *(sc->current) == '\0';
}

static char advance(struct scanner *sc)
{
  sc->current++;
  return sc->current[-1];
}

static char peek(struct scanner *sc)
{
  return *(sc->current);
}

static char peek_next(struct scanner *sc)
{
  if (is_at_end(sc))
    return '\0';
  return sc->current[1];
}

static void skip_whitespace_comments(struct scanner *sc)
{
  for (;;) {
    char c = peek(sc);
    switch (c) {
    case '\n':
      sc->line++; // Fallthrough.
    case ' ':
    case '\t':
    case '\r':
      advance(sc);
      break;
    case ';':
      while (peek(sc) != '\n' && !is_at_end(sc))
        advance(sc);
      break;
    default:
      return;
    }
  }
}

static struct token number(struct scanner *sc)
{
  while (is_digit(peek(sc)))
    advance(sc);

  if (peek(sc) == '.' && is_digit(peek_next(sc)))
    do
      advance(sc);
    while (is_digit(peek(sc)));

  return make_token(sc, TOKEN_NUMBER);
}

static enum token_type check_keyword(struct scanner *sc, int start, int len,
    const char *rest, enum token_type type)
{
  if (sc->current - sc->start == start + len
      && memcmp(sc->start + start, rest, len) == 0)
    return type;

  return TOKEN_IDENTIFIER;
}

static enum token_type identifier_type(struct scanner *sc)
{
  while (is_valid_identifier(peek(sc)))
    advance(sc);

  switch (sc->start[0]) {
  case 'c':
    if (sc->current - sc->start > 1) {
      switch (sc->start[1]) {
      case 'a': return check_keyword(sc, 2, 1, "r", TOKEN_CAR);
      case 'd': return check_keyword(sc, 2, 1, "r", TOKEN_CDR);
      case 'o': return check_keyword(sc, 2, 2, "ns", TOKEN_CONS);
      }
    }
    break;
  case 'd': return check_keyword(sc, 1, 5, "efine", TOKEN_DEFINE);
  case 'l': return check_keyword(sc, 1, 5, "ambda", TOKEN_LAMBDA);
  case 'q': return check_keyword(sc, 1, 4, "uote", TOKEN_QUOTE);
  }

  return TOKEN_IDENTIFIER;
}

static struct token identifier(struct scanner *sc)
{
  return make_token(sc, identifier_type(sc));
}

struct token scanner_next(struct scanner *sc)
{
  skip_whitespace_comments(sc);
  sc->start = sc->current;

  if (is_at_end(sc))
    return make_token(sc, TOKEN_EOF);

  char c = advance(sc);

  if (is_digit(c))
    return number(sc);

  if (is_valid_identifier(c))
    return identifier(sc);

  switch (c) {
  case '(':
    return make_token(sc, TOKEN_LEFT_PAREN);
  case ')':
    return make_token(sc, TOKEN_RIGHT_PAREN);
  case '\'':
    return make_token(sc, TOKEN_QUOTE);
  case '.':
    return make_token(sc, TOKEN_DOT);
  }

  // TODO: Emit the unexpected character value.
  return error_token(sc, "Unexpected character");
}
