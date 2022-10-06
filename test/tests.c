#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "../src/scanner.h"

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

void test_scan_simple(void)
{
  const char *sources[] = {"(", ")", "'", ".", "\0", NULL};
  enum token_type types[] = {
    TOKEN_LEFT_PAREN,
    TOKEN_RIGHT_PAREN,
    TOKEN_QUOTE,
    TOKEN_DOT,
    TOKEN_EOF,
  };
  
  for (int i = 0; sources[i] != NULL; ++i) {
    struct scanner sc;
    scanner_init(&sc, sources[i]);

    struct token tok = scanner_next(&sc);
    TEST(tok.type == types[i], "simple token '%s'", sources[i]);

    tok = scanner_next(&sc);
    TEST(tok.type == TOKEN_EOF, "simple token '%s', source end", sources[i]);
  }
}

int main(void)
{
  test_scan_simple();

  printf("%d failed, %d passed\n", count_fail, count_pass);
  return count_fail != 0;
}
