#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chunk.h"
#include "compiler.h"
#include "object.h"
#include "scanner.h"
#include "value.h"

#define UINT8_COUNT (UINT8_MAX + 1)

struct parser {
  struct scanner *scanner;
  struct token prev;  // The last consumed token.
  struct token curr;  // The currently looked at but yet unconsumed token.
  bool panic_mode;
  bool had_error;
};

enum function_type {
  TYPE_LAMBDA,
  TYPE_SCRIPT,
};

struct local {
  struct token name;
  int depth;
  bool is_captured;
};

struct upvalue {
  uint8_t index;
  bool is_local;
};

struct compiler {
  struct parser *parser;
  struct obj_lambda *lambda;
  enum function_type type;
  struct local locals[UINT8_COUNT];
  struct upvalue upvalues[UINT8_COUNT];
  size_t local_count;
  int scope_depth;
};

static void parser_init(struct parser *p, struct scanner *sc)
{
  p->scanner = sc;
  // TODO: Initialize prev & curr?
  p->panic_mode = false;
  p->had_error = false;
}

static void compiler_init(struct compiler *c, struct parser *p,
    enum function_type type)
{
  c->parser = p;
  c->lambda = NULL;
  c->type = type;
  c->local_count = 0;
  c->scope_depth = 0;
  c->lambda = new_lambda();
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

static bool identifiers_equal(struct token *a, struct token *b)
{
  return a->length == b->length && memcmp(a->start, b->start, a->length) == 0;
}

static void scope_begin(struct compiler *c)
{
  c->scope_depth++;
}

static void add_local(struct compiler *c, struct token name)
{
  if (c->local_count == UINT8_COUNT) {
    error(c->parser, "Too many local variables in this function");
    return;
  }

  struct local *local = &c->locals[c->local_count++];
  local->name = name;
  local->depth = -1;
  local->is_captured = false;
}

static void declare_variable(struct compiler *c)
{
  if (c->scope_depth == 0)
    // Global scope.
    return;

  struct token *name = &c->parser->prev;

  for (int i = c->local_count - 1; i >= 0; --i) {
    struct local *local = &c->locals[i];

    if (local->depth != -1 && local->depth < c->scope_depth)
      break;

    if (identifiers_equal(name, &local->name))
      error(c->parser, "Already a variable with this name in this scope");
  }

  add_local(c, *name);
}

static void define_variable(struct compiler *c, uint8_t global)
{
  if (c->scope_depth > 0) {
    // Local scope.
    c->locals[c->local_count - 1].depth = c->scope_depth;
    return;
  }

  emit_bytes(c, OP_DEFINE_GLOBAL, global);
}

static uint8_t read_identifier(struct compiler *c, const char *msg)
{
  consume(c->parser, TOKEN_IDENTIFIER, msg);
  declare_variable(c);

  if (c->scope_depth > 0)
    return 0;

  return identifier_constant(c);
}

static void sexp(struct compiler *);

static void define(struct compiler *c)
{
  uint8_t global = read_identifier(c, "Expect identifier after 'define'"); // a
  sexp(c);  // b
  define_variable(c, global);  // (define a b)
}

/*
 * TODO
 * ((lambda (x y) (list x y)) 1 2) -> (1 2)
 * ((lambda (x y) (list x y)) . '(1 2)) -> (1 2)
 * ((lambda (x y) (list x y)) 1 . '(2)) -> (1 2)
 *
 * ((lambda (x y . z) (list x y z)) 1 2 3) -> (1 2 (3))
 * ((lambda (x y . z) (list x y z)) . '(1 2 3)) -> (1 2 (3))
 * ((lambda (x y . z) (list x y z)) 1 2 . '(3)) -> (1 2 (3))
 */
static void lambda(struct compiler *c)
{
  struct compiler inner;
  compiler_init(&inner, c->parser, TYPE_LAMBDA);
  scope_begin(&inner);

  if (match(inner.parser, TOKEN_LEFT_PAREN)) {
    // (lambda (p1 p2 ... pn) expr) or (lambda (p1 p2 ... pn . params) expr)
    while (!check(inner.parser, TOKEN_RIGHT_PAREN)
        && !check(inner.parser, TOKEN_DOT)
        && !check(inner.parser, TOKEN_EOF)) {
      inner.lambda->arity++;

      if (inner.lambda->arity > 255)
        error_at_current(inner.parser, "Can't have more than 255 parameters");

      uint8_t constant = read_identifier(&inner, "Expect parameter name");
      define_variable(&inner, constant);
    }

    if (match(inner.parser, TOKEN_DOT)) {
      // (lambda (p1 p2 ... pn . params) expr)
      uint8_t constant = read_identifier(&inner, "Expect parameter list name");
      define_variable(&inner, constant);
      inner.lambda->has_param_list = true;
    }

    consume(inner.parser, TOKEN_RIGHT_PAREN,
        "Expect ')' at the end of a parameter list");
  } else {
    // (lambda params expr), equivalent to (lambda ( . params) expr)
    uint8_t constant = read_identifier(&inner, "Expect parameter list name");
    define_variable(&inner, constant);
    inner.lambda->has_param_list = true;
  }

  // Compile function body.
  sexp(&inner);

  struct obj_lambda *lambda = inner.lambda;
  emit_bytes(&inner, OP_CLOSURE, make_constant(&inner, OBJ_VAL(lambda)));

  for (int i = 0; i < lambda->upvalue_count; ++i) {
    emit_byte(&inner, inner.upvalues[i].is_local ? 1 : 0);
    emit_byte(&inner, inner.upvalues[i].index);
  }
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
      && !check(c->parser, TOKEN_DOT)
      && !check(c->parser, TOKEN_EOF)) {
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

// TODO: Initialize scanner, parser and compiler outside and make this
// TODO: function a "method" of compiler?
void compile(const char *source)
{
  struct scanner sc;
  scanner_init(&sc, source);

  struct parser p;
  parser_init(&p, &sc);

  struct compiler c;
  compiler_init(&c, &p, TYPE_SCRIPT);

  advance(&p);
  while (!match(&p, TOKEN_EOF))
    sexp(&c);
}
