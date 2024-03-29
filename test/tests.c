#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/common.h"
#include "../src/memory.h"
#include "../src/scanner.h"
#include "../src/state.h"
#include "../src/strpool.h"
#include "../src/value.h"

static int count_fail = 0;
static int count_pass = 0;

#define TEST(cond, msg, ...) \
  do { \
    if (cond) { \
      printf("\033[32;1mPASS\033[0m " msg "\n", __VA_ARGS__); \
      count_pass++; \
    } else { \
      printf("\033[31;1mFAIL\033[0m " msg "\n", __VA_ARGS__); \
      count_fail++; \
    } \
  } while (false)

// A variation of the TEST macro that does not expect any string formatting.
// Necessary because __VA_ARGS__ must be non-empty.
#define TEST1(cond, msg) \
  do { \
    if (cond) { \
      printf("\033[32;1mPASS\033[0m " msg "\n"); \
      count_pass++; \
    } else { \
      printf("\033[31;1mFAIL\033[0m " msg "\n"); \
      count_fail++; \
    } \
  } while (false)

static void test_scanner_simple(void)
{
  const char *sources[] = {
    "(",
    ")",
    "'",
    ".",
    "\0",
    "define",
    "lambda",
    "quote",
    "car",
    "cdr",
    "cons",
    NULL,
  };
  enum token_type types[] = {
    TOKEN_LEFT_PAREN,
    TOKEN_RIGHT_PAREN,
    TOKEN_QUOTE,
    TOKEN_DOT,
    TOKEN_EOF,
    TOKEN_DEFINE,
    TOKEN_LAMBDA,
    TOKEN_QUOTE,
    TOKEN_CAR,
    TOKEN_CDR,
    TOKEN_CONS,
  };
  
  for (int i = 0; sources[i] != NULL; ++i) {
    struct scanner sc;
    scanner_init(&sc, sources[i]);

    struct token tok = scanner_next(&sc);
    TEST(tok.type == types[i], "simple token '%s', type", sources[i]);

    tok = scanner_next(&sc);
    TEST(tok.type == TOKEN_EOF, "simple token '%s', source end", sources[i]);
  }
}

static void test_scanner_identifier(void)
{
  {
    const char *sources[] = {
      "asdf", "snake_case", "a12", "+/", "kebab-case",
      "a->b", "<>", "  camelCase\t", "->>", "@", "&b",
      "definenot", "unquote", "Lambda", "caar", "cdadr",
      NULL,
    };
    int lengths[] = {
      4, 10, 3, 2, 10,
      4, 2, 9, 3, 1, 2,
      9, 7, 6, 4, 5,
    };

    for (int i = 0; sources[i] != NULL; ++i) {
      struct scanner sc;
      scanner_init(&sc, sources[i]);

      struct token tok = scanner_next(&sc);
      TEST(tok.type == TOKEN_IDENTIFIER, "identifier '%s', type", sources[i]);
      TEST(tok.len == lengths[i], "identifier '%s', length %d",
        sources[i], lengths[i]);

      tok = scanner_next(&sc);
      TEST(tok.type == TOKEN_EOF, "identifier '%s', source end", sources[i]);
    }
  }

  {
    const char *source = "space case";
    enum token_type types[] = {TOKEN_IDENTIFIER, TOKEN_IDENTIFIER};

    struct scanner sc;
    scanner_init(&sc, source);

    for (int i = 0; i < 2; ++i) {
      struct token tok = scanner_next(&sc);
      TEST(tok.type == types[i], "invalid identifier '%s':%d",
        source, i);
    }

    struct token tok = scanner_next(&sc);
    TEST(tok.type == TOKEN_EOF, "invalid identifier '%s', source end", source);
  }

  {
    const char *source = "with(parenthesis";
    enum token_type types[] = {
      TOKEN_IDENTIFIER, TOKEN_LEFT_PAREN, TOKEN_IDENTIFIER
    };

    struct scanner sc;
    scanner_init(&sc, source);

    for (int i = 0; i < 3; ++i) {
      struct token tok = scanner_next(&sc);
      TEST(tok.type == types[i], "invalid identifier '%s':%d",
        source, i);
    }

    struct token tok = scanner_next(&sc);
    TEST(tok.type == TOKEN_EOF, "invalid identifier '%s', source end", source);
  }

  {
    const char *source = "with.dot";
    enum token_type types[] = {
      TOKEN_IDENTIFIER, TOKEN_DOT, TOKEN_IDENTIFIER
    };

    struct scanner sc;
    scanner_init(&sc, source);

    for (int i = 0; i < 3; ++i) {
      struct token tok = scanner_next(&sc);
      TEST(tok.type == types[i], "invalid identifier '%s':%d",
        source, i);
    }

    struct token tok = scanner_next(&sc);
    TEST(tok.type == TOKEN_EOF, "invalid identifier '%s', source end", source);
  }

  {
    const char *source = "multi\nline";
    enum token_type types[] = {TOKEN_IDENTIFIER, TOKEN_IDENTIFIER};

    struct scanner sc;
    scanner_init(&sc, source);

    for (int i = 0; i < 2; ++i) {
      struct token tok = scanner_next(&sc);
      TEST(tok.type == types[i], "invalid identifier '%s':%d",
        source, i);
    }

    struct token tok = scanner_next(&sc);
    TEST(tok.type == TOKEN_EOF, "invalid identifier '%s', source end", source);
  }
}

static void test_scanner_number(void)
{
  const char *sources[] = {
    "0", "1", "13234", "3.1415", "0.1", "123.21",
    "  134", "\n123432.432 ", "\t\t67\n", NULL};
  int lengths[] = {
    1, 1, 5, 6, 3, 6,
    3, 10, 2,
  };

  for (int i = 0; sources[i] != NULL; ++i) {
    struct scanner sc;
    scanner_init(&sc, sources[i]);

    struct token tok = scanner_next(&sc);
    TEST(tok.type == TOKEN_NUMBER, "number token '%s', type", sources[i]);
    TEST(tok.len == lengths[i], "number token '%s', length %d",
      sources[i], lengths[i]);

    tok = scanner_next(&sc);
    TEST(tok.type == TOKEN_EOF, "number token '%s', source end", sources[i]);
  }
}

static void test_scanner_skip_comment(void)
{
  {
    const char *source = \
      "; This is a comment.\n"
      "123\n";
    struct scanner sc;
    scanner_init(&sc, source);

    struct token tok = scanner_next(&sc);
    TEST1(tok.type == TOKEN_NUMBER, "skip comment 1");
    TEST(tok.len == 3, "skip comment 1, length %d", tok.len);
    TEST(tok.line == 2, "skip comment 1, line %d", tok.line);

    tok = scanner_next(&sc);
    TEST1(tok.type == TOKEN_EOF, "skip comment 1, source end");
  }

  {
    const char *source = \
      "123\n"
      "; This is a comment.\n"
      "4567\n";
    struct scanner sc;
    scanner_init(&sc, source);

    struct token tok = scanner_next(&sc);
    TEST1(tok.type == TOKEN_NUMBER, "skip comment 2");
    TEST(tok.len == 3, "skip comment 2, length %d", tok.len);
    TEST(tok.line == 1, "skip comment 2, line %d", tok.line);

    tok = scanner_next(&sc);
    TEST1(tok.type == TOKEN_NUMBER, "skip comment 2");
    TEST(tok.len == 4, "skip comment 2, length %d", tok.len);
    TEST(tok.line == 3, "skip comment 2, line %d", tok.line);

    tok = scanner_next(&sc);
    TEST1(tok.type == TOKEN_EOF, "skip comment 2, source end");
  }
}

static void test_scanner_compound(void)
{
  {
    const char *source = "(123)";
    enum token_type types[] = {
      TOKEN_LEFT_PAREN,
      TOKEN_NUMBER,
      TOKEN_RIGHT_PAREN,
    };
    struct scanner sc;
    scanner_init(&sc, source);

    for (int i = 0; i < 3; ++i) {
      struct token tok = scanner_next(&sc);
      TEST(tok.type == types[i], "compound 1:%d", i + 1);
    }

    struct token tok = scanner_next(&sc);
    TEST1(tok.type == TOKEN_EOF, "compound 1, source end");
  }

  {
    const char *source = "((1 . 2) 3 '(4 . 5) (fun 5.6 7))";
    enum token_type types[] = {
      TOKEN_LEFT_PAREN,
      TOKEN_LEFT_PAREN,
      TOKEN_NUMBER,
      TOKEN_DOT,
      TOKEN_NUMBER,
      TOKEN_RIGHT_PAREN,
      TOKEN_NUMBER,
      TOKEN_QUOTE,
      TOKEN_LEFT_PAREN,
      TOKEN_NUMBER,
      TOKEN_DOT,
      TOKEN_NUMBER,
      TOKEN_RIGHT_PAREN,
      TOKEN_LEFT_PAREN,
      TOKEN_IDENTIFIER,
      TOKEN_NUMBER,
      TOKEN_NUMBER,
      TOKEN_RIGHT_PAREN,
      TOKEN_RIGHT_PAREN,
    };
    struct scanner sc;
    scanner_init(&sc, source);

    for (int i = 0; i < 19; ++i) {
      struct token tok = scanner_next(&sc);
      TEST(tok.type == types[i], "compound 2:%d", i + 1);
    }

    struct token tok = scanner_next(&sc);
    TEST1(tok.type == TOKEN_EOF, "compound 2, source end");
  }
}

static void test_interning_unique(void)
{
  const char *strings[] = {
    "a",
    "cat",
    "",
    "a string",
    "a very very long string",
    "this one is extremely long, so long that it might consist of many words",
    "<>><%$#@&* $%#$% $%",
    "\n453453\tlalala",
    "another one",
    "let's have one more",
    NULL,
  };

  struct wisp_state w;
  wisp_state_init(&w);

  for (int i = 0; strings[i] != NULL; ++i) {
    struct obj_string *interned = str_pool_intern(&w, strings[i],
        strlen(strings[i]));
    const char *c_interned = interned->chars;

    int non_null_positions = 0;
    for (int j = 0; j < 1 << w.str_pool.exp; ++j)
      if (w.str_pool.ht[j] != NULL)
        non_null_positions++;

    TEST(strlen(strings[i]) == interned->len
        && interned->len == strlen(c_interned)
        && memcmp(strings[i], c_interned, strlen(strings[i])) == 0,
        "unique interning %d, equality", i + 1);
    TEST(w.str_pool.count == i + 1, "unique interning %d, pool size %d",
        i + 1, w.str_pool.count);
    TEST(w.str_pool.count == non_null_positions,
        "unique interning %d, non nulls %d", i + 1, non_null_positions);
  }

  wisp_state_free(&w);
}

static void test_interning_non_unique(void)
{
  const char *strings[] = {
    "a",
    "cat",
    "",
    "cat",
    "a string",
    "a very very long string",
    "<>><%$#@&* $%#$% $%",
    "a very very long string",
    "this one is extremely long, so long that it might consist of many words",
    "a string",
    "<>><%$#@&* $%#$% $%",
    "\n453453\tlalala",
    "",
    "a",
    "b",
    "c",
    "d",
    "e",
    "a",
    "b",
    "a very very long string",
    "<>><%$#@&* $%#$% $%",
    "c",
    "d",
    "cat",
    "1",
    "2",
    "3",
    "4",
    "5",
    "e",
    "this one is extremely long, so long that it might consist of many words",
    NULL,
  };

  bool is_unique[] = {
    true,
    true,
    true,
    false,
    true,
    true,
    true,
    false,
    true,
    false,
    false,
    true,
    false,
    false,
    true,
    true,
    true,
    true,
    false,
    false,
    false,
    false,
    false,
    false,
    false,
    true,
    true,
    true,
    true,
    true,
    false,
    false,
  };

  int prev_pool_count = 0;
  struct wisp_state w;
  wisp_state_init(&w);

  for (int i = 0; strings[i] != NULL; ++i) {
    struct obj_string *interned = str_pool_intern(&w, strings[i],
        strlen(strings[i]));
    const char *c_interned = interned->chars;

    int non_null_positions = 0;
    for (int j = 0; j < 1 << w.str_pool.exp; ++j)
      if (w.str_pool.ht[j] != NULL)
        non_null_positions++;

    TEST(strlen(strings[i]) == interned->len
        && interned->len == strlen(c_interned)
        && memcmp(strings[i], c_interned, strlen(strings[i])) == 0,
        "non-unique interning %d, equality", i + 1);

    if (is_unique[i])
      TEST(w.str_pool.count == prev_pool_count + 1,
          "non-unique interning %d, pool size is %d, should be %d (unique string)",
          i + 1, w.str_pool.count, prev_pool_count + 1);
    else
      TEST(w.str_pool.count == prev_pool_count,
          "non-unique interning %d, pool size is %d, should be %d (non-unique string)",
          i + 1, w.str_pool.count, prev_pool_count);

    TEST(w.str_pool.count == non_null_positions, "unique interning %d, non nulls %d",
        i + 1, non_null_positions);

    prev_pool_count = w.str_pool.count;
  }

  wisp_state_free(&w);
}

static void test_interning_identity()
{
  const char *strings[] = {
    "a",
    "cat",
    "",
    "a string",
    "a very very long string",
    "this one is extremely long, so long that it might consist of many words",
    "<>><%$#@&* $%#$% $%",
    "\n453453\tlalala",
    "another one",
    "let's have one more",
    NULL,
  };

  struct wisp_state w;
  wisp_state_init(&w);

  for (int i = 0; strings[i] != NULL; ++i) {
    struct obj_string *fst = str_pool_intern(&w, strings[i],
        strlen(strings[i]));
    struct obj_string *snd = str_pool_intern(&w, strings[i],
        strlen(strings[i]));

    TEST(fst->len == snd->len && memcmp(fst->chars, snd->chars, fst->len) == 0,
        "Interning comparison by equality %d", i + 1);
    TEST(fst == snd, "Interning comparison by identity %d", i + 1);
  }

  wisp_state_free(&w);
}

static void test_interning_uninterning()
{
  const char *strings[] = {
    "a",
    "cat",
    "",
    "a string",
    "a very very long string",
    "this one is extremely long, so long that it might consist of many words",
    "<>><%$#@&* $%#$% $%",
    "\n453453\tlalala",
    "another one",
    "let's have one more",
    NULL,
  };

  struct obj_string *interned_strings[11] = {NULL};

  struct wisp_state w;
  wisp_state_init(&w);

  // We fix the table (exp) capacity and array up front, because we need to
  // ensure that the structure after the first interning is exactly the same
  // as after uninterning and reinterning all strings. The capacity below is
  // sufficient not to require adjusting the table size.
  // If we let the (exp) capacity and the underlying array initialized to the
  // default values, the table would work correctly, but during the first
  // insertion, its capacity would have to grow. This causes the exp to change,
  // which in turn changes the way hash values are computed. In effect,
  // strings interned when exp can change will be placed in different positions
  // from when the same strings are interned with exp fixed.
  w.str_pool.exp = 5;
  w.str_pool.ht = calloc(1 << w.str_pool.exp, sizeof(struct obj_string *));

  for (int i = 0; strings[i] != NULL; ++i) {
    struct obj_string *interned = str_pool_intern(&w, strings[i],
        strlen(strings[i]));
    const char *c_interned = interned->chars;
    interned_strings[i] = interned;

    int non_null_positions = 0;
    for (int j = 0; j < 1 << w.str_pool.exp; ++j)
      if (w.str_pool.ht[j] != NULL)
        non_null_positions++;

    TEST(strlen(strings[i]) == interned->len
        && interned->len == strlen(c_interned)
        && memcmp(strings[i], c_interned, strlen(strings[i])) == 0,
        "unique interning %d, equality", i + 1);
    TEST(w.str_pool.count == i + 1, "unique interning %d, pool size %d",
        i + 1, w.str_pool.count);
    TEST(w.str_pool.count == non_null_positions,
        "unique interning %d, non nulls %d", i + 1, non_null_positions);
  }

  int count = w.str_pool.count;

  // To be kept in sync with strpool.h:str_pool:gravestone;
  char *gravestone = "<deleted>";
  size_t gravestone_len = strlen(gravestone);

  for (int i = 0; interned_strings[i] != NULL; ++i) {
    str_pool_unintern(&w, interned_strings[i]);
    TEST(w.str_pool.count == count,
        "uninterning %d, string pool size: %d, old count: %d",
        i + 1, w.str_pool.count, count);

    int n_gravestones = 0;
    for (int j = 0; j < 1 << w.str_pool.exp; ++j) {
      if (w.str_pool.ht[j] != NULL
          && w.str_pool.ht[j]->len == gravestone_len
          && memcmp(w.str_pool.ht[j]->chars, gravestone, gravestone_len) == 0)
        n_gravestones++;
    }

    TEST(n_gravestones == i + 1,
        "uninterning %d, expected %d gravestones, %d are present",
        i + 1, i + 1, n_gravestones);
  }

  int n_gravestones = 0;
  for (int j = 0; j < 1 << w.str_pool.exp; ++j) {
    if (w.str_pool.ht[j] != NULL
        && w.str_pool.ht[j]->len == gravestone_len
        && memcmp(w.str_pool.ht[j]->chars, gravestone, gravestone_len) == 0)
      n_gravestones++;
  }
  TEST(n_gravestones == w.str_pool.count,
      "uninterned all strings, expecting %d gravestones, %d are present",
      w.str_pool.count, n_gravestones);

  int expected_n_gravestones = w.str_pool.count;
  int old_exp = w.str_pool.exp;

  for (int i = 0; strings[i] != NULL; ++i) {
    expected_n_gravestones--;

    struct obj_string *interned = str_pool_intern(&w, strings[i],
        strlen(strings[i]));
    const char *c_interned = interned->chars;

    int non_null_positions = 0;
    for (int j = 0; j < 1 << w.str_pool.exp; ++j)
      if (w.str_pool.ht[j] != NULL)
        non_null_positions++;

    int n_gravestones = 0;
    for (int j = 0; j < 1 << w.str_pool.exp; ++j) {
      if (w.str_pool.ht[j] != NULL
          && w.str_pool.ht[j]->len == gravestone_len
          && memcmp(w.str_pool.ht[j]->chars, gravestone, gravestone_len) == 0)
        n_gravestones++;
    }

    TEST(strlen(strings[i]) == interned->len
        && interned->len == strlen(c_interned)
        && memcmp(strings[i], c_interned, strlen(strings[i])) == 0,
        "unique interning %d, equality", i + 1);
    TEST(w.str_pool.count == count,
        "reinterning %d, expected size %d, got %d",
        i + 1, count, w.str_pool.count);
    TEST(n_gravestones == expected_n_gravestones,
        "reinterning %d, expected %d gravestones, %d are present",
        i + 1, expected_n_gravestones, n_gravestones);
    TEST(w.str_pool.count == non_null_positions,
        "reinterning %d, non nulls %d", i + 1, non_null_positions);
  }

  TEST(old_exp == w.str_pool.exp,
      "after reinterning, exp(capacity) should be %d and is %d",
      old_exp, w.str_pool.exp);

  wisp_state_free(&w);
}

int main(void)
{
  // Scanner tests.
  test_scanner_simple();
  test_scanner_identifier();
  test_scanner_number();
  test_scanner_skip_comment();
  test_scanner_compound();

  // String pool tests.
  test_interning_unique();
  test_interning_non_unique();
  test_interning_identity();
  test_interning_uninterning();

  printf("%d failed, %d passed\n", count_fail, count_pass);
  return count_fail != 0;
}
