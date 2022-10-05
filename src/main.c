#include <stdio.h>
#include <stdlib.h>

#include "scanner.h"

#define PROGRAM_NAME "lisp"

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

  fseek(f, 0L, SEEK_END);
  size_t file_size = ftell(f);
  rewind(f);

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
  // TODO: Read arbitrarily long lines.
  char line[1024];
  for (;;) {
    printf("> ");
    if (!fgets(line, sizeof(line), stdin)) {
      printf("\n");
      break;
    }
    // TODO: interpret(line);
  }
}

static int run_file(const char *path)
{
  char *source = read_file(path);
  if (source == NULL)
    return EXIT_IO_ERROR;

  printf("%s\n", source);

  struct scanner sc;
  scanner_init(&sc, source);
  // TODO: interpret(source);
  // TODO: free(source);
  // TODO: compile error => EXIT_DATA_ERROR
  // TODO: interpret error => EXIT_SOFTWARE_ERROR

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
    fprintf(stderr, "Usage: " PROGRAM_NAME " [path]\n");
  }
  return exit_code;
}
