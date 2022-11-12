#include <stdio.h>
#include <stdlib.h>

#include "compiler.h"
#include "object.h"
#include "scanner.h"
#include "state.h"

#define EXIT_USAGE_ERROR 64
#define EXIT_DATA_ERROR 65
#define EXIT_SOFTWARE_ERROR 70
#define EXIT_IO_ERROR 74

static char *read_file(const char *path)
{
  FILE *f = fopen(path, "r");
  if (f == NULL) {
    fprintf(stderr, "Cannot open %s\n", path);
    return NULL;
  }

  if (fseek(f, 0L, SEEK_END) == -1) {
    fclose(f);
    fprintf(stderr, "Cannot seek to the end of %s\n", path);
    return NULL;
  }

  long fs = ftell(f);
  if (fs == -1) {
    fclose(f);
    fprintf(stderr, "Cannot tell the size of %s\n", path);
    return NULL;
  }

  rewind(f);
  size_t file_size = (size_t) fs;

  char *buffer = malloc((file_size + 1) * sizeof(char));
  if (buffer == NULL) {
    fclose(f);
    fprintf(stderr, "Not enough memory to read %s\n", path);
    return NULL;
  }

  size_t bytes_read = fread(buffer, sizeof(char), file_size, f);
  if (bytes_read < file_size) {
    fprintf(stderr, "Could not read %s\n", path);
    free(buffer);
    fclose(f);
    return NULL;
  }

  buffer[bytes_read] = '\0';
  fclose(f);
  return buffer;
}

static void run_repl()
{
  char line[1024];
  struct wisp_state w;
  wisp_state_init(&w);

  // TODO: Read arbitrarily long lines.
  for (;;) {
    printf("> ");
    if (!fgets(line, sizeof(line), stdin)) {
      printf("\n");
      break;
    }

    struct obj_lambda *lambda = compile(&w, line);
    (void) lambda;
    // TODO: interpret(lambda);
  }

  wisp_state_free(&w);
}

static int run_file(const char *path)
{
  char *source = read_file(path);
  if (source == NULL)
    return EXIT_IO_ERROR;

  struct wisp_state w;
  wisp_state_init(&w);

  struct obj_lambda *lambda = compile(&w, source);
  if (lambda == NULL)
    return EXIT_DATA_ERROR;

  // TODO: interpret(lambda);
  // TODO: interpret error => EXIT_SOFTWARE_ERROR

  wisp_state_free(&w);
  free(source);

  return EXIT_SUCCESS;
}

int main(int argc, const char **argv)
{
  int exit_code = EXIT_SUCCESS;
  if (argc == 1)
    run_repl();
  else if (argc == 2)
    exit_code = run_file(argv[1]);
  else {
    exit_code = EXIT_USAGE_ERROR;
    fprintf(stderr, "Usage: wisp [path]\n");
  }
  return exit_code;
}
