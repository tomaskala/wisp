# Wisp

A Lisp implementation called Wisp.

Extremely unfinished to the point of being barely usable. Currently, there is 
an almost-working garbage collector with the slight flaw such that it eats the 
compiler before it's done compiling. It's not enough to simply disable the 
garbage collector when compiling, because even the compilation process needs to 
allocate some memory. The correct way to fix this is to mark the compiler as a 
GC root. I need to think about how to do that elegantly without making the 
overall state carry around the compiler reference.

Another annoyance is the need to carry around two hash table definitions; one 
for the string pool and for the global-scope bindings. Ideally, these would get 
merged, or possibly global-scope bindings stored in another data structure.

## Implemented

The following has been implemented.

- [x] REPL
- [x] Script evaluation
- [x] Lambda functions correctly handling dotted lists in the parameters as 
  well as when applied to such lists
- [x] Proper and improper lists
- [x] `cons`, `car` and `cdr` operators
- [x] Global scope definition (`define`)
- [x] Basic quoting
- [x] Function calls
- [x] Recursion
- [x] String interning

## Missing

Apart from the various TODOs scattered around the codebase, the following is a 
non-exhaustive list of unimplemented features.

- [ ] `eq?` and `atom?` operators
- [ ] More complex equality tests (`eqv?` and `equal?`)
- [ ] Let bindings (`let`, `let*` and `letrec`)
- [ ] Conditions
- [ ] Native functions (including arithmetic)
- [ ] Tail call optimisation
- [ ] Standard library
- [ ] Complex quoting
- [ ] Optimisations (such as direct threading of the interpreter loop)
- [ ] Immutable data structures
- [ ] Bytes and/or string types
- [ ] Documentation
