#include <stdbool.h>
#include <stdio.h>

#include "compiler.h"
#include "scanner.h"

struct parser {
  struct scanner *scanner;
  struct token prev;
  struct token curr;
  bool panic_mode;
  bool had_error;
};

struct compiler {
  struct parser *parser;
};

static void parser_init(struct parser *p, struct scanner *sc)
{
  p->scanner = sc;
  // TODO: Initialize prev & curr?
  p->panic_mode = false;
  p->had_error = false;
}

static void error_at(struct parser *p, struct token tok, const char *msg)
{
  if (p->panic_mode)
    return;

  p->panic_mode = true;
  fprintf(stderr, "[line %d] Error", tok.line);

  if (tok.type == TOKEN_EOF)
    fprintf(stderr, " at end");
  else if (tok.type != TOKEN_ERROR)
    fprintf(stderr, " at '%.*s'", tok.length, tok.start);

  fprintf(stderr, ": %s\n", msg);
  p->had_error = true;
}

static void error(struct parser *p, const char *msg)
{
  error_at(p, p->prev, msg);
}

static void error_at_current(struct parser *p, const char *msg)
{
  error_at(p, p->curr, msg);
}

static void advance(struct parser *p)
{
  p->prev = p->curr;

  for (;;) {
    p->curr = scanner_next(p->scanner);

    if (p->curr.type != TOKEN_ERROR)
      break;

    // Since the current token's type is TOKEN_ERROR, its start
    // field is a null-terminated constant error message.
    error_at_current(p, p->curr.start);
  }
}

static void consume(struct parser *p, enum token_type type, const char *msg)
{
  if (p->curr.type == type)
    advance(p);
  else
    error_at_current(p, msg);
}

static bool check(struct parser *p, enum token_type type)
{
  return p->curr.type == type;
}

static bool match(struct parser *p, enum token_type type)
{
  if (!check(p, type))
    return false;

  advance(p);
  return true;
}

static void sexp(struct compiler *c)
{
  consume(c->parser, TOKEN_LEFT_PAREN, "Expect '('");
  // TODO
  consume(c->parser, TOKEN_RIGHT_PAREN, "Expect ')'");
}

static void compiler_init(struct compiler *c, struct parser *p)
{
  c->parser = p;
}

// TODO: Initialize scanner, parser and compiler outside and make this
// TODO: function a "method" of compiler?
void compile(const char *source)
{
  struct scanner sc;
  scanner_init(&sc, source);

  struct parser p;
  parser_init(&p, &sc);

  struct compiler c;
  compiler_init(&c, &p);

  advance(&p);
  while (!match(&p, TOKEN_EOF))
    sexp(&c);
}
