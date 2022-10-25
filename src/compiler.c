#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "chunk.h"
#include "compiler.h"
#include "scanner.h"
#include "value.h"

struct parser {
  struct scanner *scanner;
  struct token prev;  // The last consumed token.
  struct token curr;  // The currently looked at but yet unconsumed token.
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

static void emit_byte(struct compiler *c, uint8_t byte)
{
  // TODO
}

static void emit_bytes(struct compiler *c, uint8_t byte1, uint8_t byte2)
{
  emit_byte(c, byte1);
  emit_byte(c, byte2);
}

static uint8_t make_constant(struct compiler *c, Value v)
{
  // TODO
  return (uint8_t) 0;
}

static void emit_constant(struct compiler *c, Value v)
{
  emit_bytes(c, OP_CONSTANT, make_constant(c, v));
}

static void identifier(struct compiler *c)
{
  // TODO: Emit identifier
}

static void number(struct compiler *c)
{
  double value = strtod(c->parser->prev.start, NULL);
  emit_constant(c, NUM_VAL(value));
}

// TODO: Needs handling.
static void quote(struct compiler *c)
{
  emit_byte(c, OP_QUOTE);
}

static bool is_primitive(struct parser *p)
{
  return p->curr.type >= PRIMITIVE_START && p->curr.type <= PRIMITIVE_END;
}

static uint8_t identifier_constant(struct compiler *c)
{
  // TODO: Add c->parser.prev as an atom constant
  return (uint8_t) 0;
}

static uint8_t read_identifier(struct compiler *c, const char *msg)
{
  consume(c->parser, TOKEN_IDENTIFIER, msg);
  return identifier_constant(c);
}

static void define_variable(struct compiler *c, uint8_t global)
{
  emit_bytes(c, OP_DEFINE_GLOBAL, global);
}

static void sexp(struct compiler *);

static void define(struct compiler *c)
{
  uint8_t global = read_identifier(c, "Expect identifier after 'define'"); // a
  sexp(c);  // b
  define_variable(c, global);  // (define a b)
}

static void lambda(struct compiler *c)
{
  // TODO: (lambda identifier|list expr)
}

static void cons(struct compiler *c)
{
  sexp(c);  // a
  sexp(c);  // b
  emit_byte(c, OP_CONS);  // (cons a b)
}

static void car(struct compiler *c)
{
  sexp(c);  // a
  emit_byte(c, OP_CAR);  // (car a)
}

static void cdr(struct compiler *c)
{
  sexp(c);  // a
  emit_byte(c, OP_CDR);  // (cdr a)
}

static void primitive(struct compiler *c)
{
  if (match(c->parser, TOKEN_DEFINE))
    define(c);
  else if (match(c->parser, TOKEN_LAMBDA))
    lambda(c);
  else if (match(c->parser, TOKEN_CONS))
    cons(c);
  else if (match(c->parser, TOKEN_CAR))
    car(c);
  else if (match(c->parser, TOKEN_CDR))
    cdr(c);
  else
    // Should never happen as long as all primitive tokens are between
    // 'PRIMITIVE_START' and 'PRIMITIVE_END'.
    error_at_current(c->parser, "Unknown primitive");
}

static void call(struct compiler *c)
{
  // Compile the function being called.
  sexp(c);

  uint8_t opcode = OP_CALL;
  uint8_t arg_count = 0;

  // Compile the function arguments.
  while (!check(c->parser, TOKEN_RIGHT_PAREN)
      && !check(c->parser, TOKEN_DOT)) {
    sexp(c);

    if (arg_count == 255)
      error(c->parser, "Can't have more than 255 arguments");

    arg_count++;
  }

  // Compile the optional dotted argument, which, for a function call,
  // must be a quoted list (or an identifier associated with one).
  // TODO: Compile error for different S-expressions?
  if (match(c->parser, TOKEN_DOT)) {
    opcode = OP_DOT_CALL;
    sexp(c);

    if (arg_count == 255)
      error(c->parser,
          "Can't have more than 255 arguments, including the dotted one");

    arg_count++;
  }

  emit_bytes(c, opcode, arg_count);
}

static void list(struct compiler *c)
{
  if (match(c->parser, TOKEN_RIGHT_PAREN))
    emit_constant(c, NIL_VAL);
  else if (is_primitive(c->parser))
    primitive(c);
  else
    call(c);

  consume(c->parser, TOKEN_RIGHT_PAREN, "Expect ')' at the end of a list");
}

static void sexp(struct compiler *c)
{
  if (match(c->parser, TOKEN_IDENTIFIER))
    identifier(c);
  else if (match(c->parser, TOKEN_NUMBER))
    number(c);
  else if (match(c->parser, TOKEN_QUOTE))
    quote(c);
  else if (match(c->parser, TOKEN_LEFT_PAREN))
    list(c);
  else
    error_at_current(c->parser, "Unexpected token");
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
