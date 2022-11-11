#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chunk.h"
#include "compiler.h"
#include "object.h"
#include "scanner.h"
#include "value.h"

struct parser {
  // Fetches tokens from the parsed string.
  struct scanner *scanner;
  
  // The last consumed token.
  struct token prev;

  // The currently looked at but yet unconsumed token.
  struct token curr;

  // Is the parser currently in an error state?
  bool panic_mode;

  // Was there a parse error?
  bool had_error;
};

enum function_type {
  // Currently compiling a lambda.
  TYPE_LAMBDA,

  // Currently compiling the global scope.
  TYPE_SCRIPT,
};

struct local {
  // Token holding the local variable name.
  struct token name;

  // Depth of the scope the local is defined in.
  int depth;

  // Is the local captured by any nested function declaration?
  bool is_captured;
};

struct upvalue {
  // Index of the variable the upvalue closes over in the surrounding compiler
  // locals array (is_local == true) or upvalues array (is_local == false).
  uint8_t index;

  // Does the upvalue close over a local variable in the surrounding scope or
  // over another upvalue?
  bool is_local;
};

struct compiler {
  // The overall state of the program shared by all compilers.
  struct wisp_state *w;

  // The immediately surrounding compiler (or NULL if in global scope).
  struct compiler *enclosing;

  // Parser object shared between all compilers.
  struct parser *parser;

  // Currently compiled lambda (or NULL if in global scope).
  struct obj_lambda *lambda;

  // Type of the currently compiled lambda.
  enum function_type type;

  // Local variables defined in the currently compiled lambda.
  struct local locals[UINT8_COUNT];

  // Upvalues defined in the currently compiled lambda.
  struct upvalue upvalues[UINT8_COUNT];

  // Number of local variables defined in the currently compiled lambda.
  int local_count;

  // How many scopes away from the global scope (= 0).
  int scope_depth;
};

static void parser_init(struct parser *p, struct scanner *sc)
{
  p->scanner = sc;
  p->panic_mode = false;
  p->had_error = false;
}

static void compiler_init(struct compiler *c, struct wisp_state *w,
    struct compiler *enclosing, struct parser *p, enum function_type type)
{
  c->w = w;
  c->enclosing = enclosing;
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
    fprintf(stderr, " at '%.*s'", tok.len, tok.start);

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
  chunk_write(&c->lambda->chunk, byte, c->parser->prev.line);
}

static void emit_bytes(struct compiler *c, uint8_t byte1, uint8_t byte2)
{
  emit_byte(c, byte1);
  emit_byte(c, byte2);
}

static uint8_t make_constant(struct compiler *c, Value v)
{
  int constant = chunk_add_constant(&c->lambda->chunk, v);

  if (constant > UINT8_MAX) {
    error(c->parser, "Too many constants in one chunk");
    return 0;
  }

  return (uint8_t) constant;
}

static void emit_constant(struct compiler *c, Value v)
{
  emit_bytes(c, OP_CONSTANT, make_constant(c, v));
}

static void synchronize(struct parser *p)
{
  p->panic_mode = false;

  while (p->curr.type != TOKEN_EOF) {
    if (p->prev.type == TOKEN_RIGHT_PAREN || IS_PRIMITIVE(p->curr.type))
      return;

    advance(p);
  }
}

static uint8_t identifier_constant(struct compiler *c, struct token *name)
{
  struct obj_string *identifier = str_pool_intern(&c->w->str_pool, name->start,
      name->len);
  return make_constant(c, OBJ_VAL(identifier));
}

static bool identifiers_equal(struct token *a, struct token *b)
{
  return a->len == b->len && memcmp(a->start, b->start, a->len) == 0;
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

  // Check whether a variable with this name already exists in the scope.
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

  return identifier_constant(c, &c->parser->prev);
}

static void sexp(struct compiler *, bool);

static void define(struct compiler *c)
{
  uint8_t global = read_identifier(c, "Expect identifier after 'define'"); // a
  sexp(c, false);  // b
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
  compiler_init(&inner, c->w, c, c->parser, TYPE_LAMBDA);
  scope_begin(&inner);

  if (match(inner.parser, TOKEN_LEFT_PAREN)) {
    // (lambda (p1 p2 ... pn) expr) or (lambda (p1 p2 ... pn . params) expr)
    while (!check(inner.parser, TOKEN_RIGHT_PAREN)
        && !check(inner.parser, TOKEN_DOT)
        && !check(inner.parser, TOKEN_EOF)) {
      inner.lambda->arity++;

      if (inner.lambda->arity > UINT8_MAX)
        error_at_current(inner.parser,
            "Can't have more than " XSTR(UINT8_MAX) " parameters");

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
  sexp(&inner, false);

  // Emit a return opcode.
  emit_byte(&inner, OP_RETURN);

  // At this point, the lambda is compiled and the 'inner' compiler done.
  struct obj_lambda *lambda = inner.lambda;

  emit_bytes(c, OP_CLOSURE, make_constant(c, OBJ_VAL(lambda)));

  for (int i = 0; i < lambda->upvalue_count; ++i) {
    emit_byte(c, inner.upvalues[i].is_local ? 1 : 0);
    emit_byte(c, inner.upvalues[i].index);
  }
}

static void cons(struct compiler *c)
{
  sexp(c, false);  // a
  sexp(c, false);  // b
  emit_byte(c, OP_CONS);  // (cons a b)
}

static void car(struct compiler *c)
{
  sexp(c, false);  // a
  emit_byte(c, OP_CAR);  // (car a)
}

static void cdr(struct compiler *c)
{
  sexp(c, false);  // a
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
    // '_PRIMITIVE_START' and '_PRIMITIVE_END'.
    error_at_current(c->parser, "Unknown primitive");
}

static void call(struct compiler *c)
{
  // Compile the function being called.
  sexp(c, false);

  uint8_t opcode = OP_CALL;
  uint8_t arg_count = 0;

  // Compile the function arguments.
  while (!check(c->parser, TOKEN_RIGHT_PAREN)
      && !check(c->parser, TOKEN_DOT)
      && !check(c->parser, TOKEN_EOF)) {
    sexp(c, false);

    if (arg_count == UINT8_MAX)
      error(c->parser, "Can't have more than " XSTR(UINT8_MAX) " arguments");

    arg_count++;
  }

  // Compile the optional dotted argument, which, for a function call,
  // must be a quoted list (or an identifier associated with one).
  // TODO: Compile error for different S-expressions?
  if (match(c->parser, TOKEN_DOT)) {
    opcode = OP_DOT_CALL;
    sexp(c, false);
  }

  emit_bytes(c, opcode, arg_count);
}

static int resolve_local(struct compiler *c, struct token *name)
{
  for (int i = c->local_count - 1; i >= 0; --i) {
    struct local *local = &c->locals[i];

    if (identifiers_equal(name, &local->name)) {
      if (local->depth == -1)
        error(c->parser, "Can't read a variable in its own initializer");

      return i;
    }
  }

  return -1;
}

static int add_upvalue(struct compiler *c, uint8_t index, bool is_local)
{
  int upvalue_count = c->lambda->upvalue_count;

  for (int i = 0; i < upvalue_count; ++i) {
    struct upvalue *upvalue = &c->upvalues[i];

    if (upvalue->index == index && upvalue->is_local == is_local)
      // An upvalue closing over the variable already exists, reuse it.
      return i;
  }

  if (upvalue_count == UINT8_COUNT) {
    error(c->parser, "Too many closure variables in function");
    return 0;
  }

  c->upvalues[upvalue_count].is_local = is_local;
  c->upvalues[upvalue_count].index = index;
  return c->lambda->upvalue_count++;
}

static int resolve_upvalue(struct compiler *c, struct token *name)
{
  // The function is called after attempting to resolve the variable in
  // the current function scope, so we start at the enclosing compiler.
  if (c->enclosing == NULL)
    // Reached the outermost function without finding a local variable, so it
    // must be global (or undefined).
    return -1;

  int local = resolve_local(c->enclosing, name);
  if (local != -1) {
    // Found the variable as local in the enclosing compiler.
    c->enclosing->locals[local].is_captured = true;
    return add_upvalue(c, (uint8_t) local, true);
  }

  int upvalue = resolve_upvalue(c->enclosing, name);
  if (upvalue != -1)
    // Found the variable as local in a non-directly enclosing compiler.
    return add_upvalue(c, (uint8_t) upvalue, false);

  return -1;
}

static void atom(struct compiler *c)
{
  // TODO
  (void) c;
}

static void identifier(struct compiler *c)
{
  uint8_t get_op;
  struct token *name = &c->parser->prev;
  int arg = resolve_local(c, name);

  if (arg != -1)
    // Local scope.
    get_op = OP_GET_LOCAL;
  else if ((arg = resolve_upvalue(c, name)) != -1)
    // Outer scope.
    get_op = OP_GET_UPVALUE;
  else {
    // Global scope.
    arg = identifier_constant(c, name);
    get_op = OP_GET_GLOBAL;
  }

  emit_bytes(c, get_op, (uint8_t) arg);
}

static void number(struct compiler *c)
{
  double value = strtod(c->parser->prev.start, NULL);
  emit_constant(c, NUM_VAL(value));
}

static void list(struct compiler *c)
{
  if (check(c->parser, TOKEN_RIGHT_PAREN))
    // '() evaluates to nil.
    emit_constant(c, NIL_VAL);
  else if (match(c->parser, TOKEN_DOT))
    // '( . a) evaluates to a
    sexp(c, true);
  else {
    int elements = 0;

    while (!check(c->parser, TOKEN_RIGHT_PAREN)
        && !check(c->parser, TOKEN_DOT)
        && !check(c->parser, TOKEN_EOF)) {
      sexp(c, true);
      elements++;
    }

    if (match(c->parser, TOKEN_DOT))
      sexp(c, true);
    else
      emit_constant(c, NIL_VAL);

    for (int i = 0; i < elements - 1; ++i)
      emit_byte(c, OP_CONS);
  }

  consume(c->parser, TOKEN_RIGHT_PAREN, "Expect ')' at the end of a list");
}

static void call_or_primitive(struct compiler *c)
{
  if (check(c->parser, TOKEN_RIGHT_PAREN))
    error_at_current(c->parser, "Expect function to call");
  else if (IS_PRIMITIVE(c->parser->curr.type))
    primitive(c);
  else
    call(c);

  consume(c->parser, TOKEN_RIGHT_PAREN, "Expect ')' at the end of a list");
}

static void sexp(struct compiler *c, bool quoted)
{
  if (match(c->parser, TOKEN_IDENTIFIER)) {
    if (quoted)
      atom(c);
    else
      identifier(c);
  } else if (match(c->parser, TOKEN_NUMBER))
    number(c);
  else if (match(c->parser, TOKEN_QUOTE))
    sexp(c, true);
  else if (match(c->parser, TOKEN_LEFT_PAREN)) {
    if (quoted)
      list(c);
    else
      call_or_primitive(c);
  } else
    error_at_current(c->parser, "Unexpected token");

  if (c->parser->panic_mode)
    synchronize(c->parser);
}

void compile(struct wisp_state *w, const char *source)
{
  struct scanner sc;
  scanner_init(&sc, source);

  struct parser p;
  parser_init(&p, &sc);

  struct compiler c;
  compiler_init(&c, w, NULL, &p, TYPE_SCRIPT);

  advance(&p);
  while (!match(&p, TOKEN_EOF))
    sexp(&c, false);
}
