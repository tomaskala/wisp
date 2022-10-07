#ifndef LISP_COMPILER_H
#define LISP_COMPILER_H

struct compiler {
};

void compiler_init(struct compiler *);

void compiler_run(struct compiler *, const char *);

#endif
