#ifndef DECLS_H
#define DECLS_H

#include "dia2code.h"

/* Package processing auxiliary structure:
   "Declarations" are UML packages (which map to IDL module or C++
   namespace or Ada package or ...) or UML classes (which map to various
   other language constructs.)
   For each top level declaration, a separate file is generated.
   Nested packages are generated into the same file as nested modules.
   `struct declaration' is a buffer that gathers together classes and
   packages ordered by their dependencies, going from least dependencies
   to most.  We cannot handle circular dependencies yet.  */

struct declaration;

struct module {  /* UML package = IDL module
                    What we call `module' equally applies to C++ (namespace)
                    and Ada (package.) I don't know about other languages.  */
    umlpackage *pkg;
    struct declaration *contents;
};

typedef struct module module;

typedef enum { dk_module, dk_class } decl_kind_t;

struct declaration {
    decl_kind_t decl_kind;
    union /* switch(decl_kind) */ {
    /* case dk_module: */
        module *this_module;
    /* case dk_class:  */
        umlclassnode *this_class;
        /* In `this_class', `next' is not used since we use our own
           sequencing (see `prev' and `next' below.)  */
    } u;
    struct declaration *prev, *next;  /* other declarations in this scope */
};

typedef struct declaration declaration;

extern declaration *decls;

/* Utilities for building the global `decls' from umlclassnodes and
   their parents.  (`decls' contains everything in ascending order of
   interdependence.)  */

extern module *
       create_nested_modules_from_pkglist (umlpackagelist pkglist, module *m);

extern module *
       find_or_add_module (declaration **dptr, umlpackagelist pkglist);

extern module * find_module (declaration *d, umlpackagelist pkglist);

extern declaration * find_class (umlclassnode *node);

extern declaration * append_decl (declaration *d);

extern void push (umlclassnode *node, batch *b);

extern int use_corba;  /* Set by push() if CORBA stereotypes in use.  */

#endif  /* DECLS_H */

