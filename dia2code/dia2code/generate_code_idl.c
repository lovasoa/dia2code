/***************************************************************************
         generate_code_idl.c  -  Function that generates CORBA IDL
                             -------------------
    begin                : Sun Dec 30 2001
    copyright            : (C) 2000-2001 by Chris McGee
    email                : sirnewton_01@yahoo.ca
 ***************************************************************************/
/**** Rational Rose stereotypes added by okellogg@users.sourceforge.net ****/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "dia2code.h"

#ifndef NEW
#define NEW(c) ((c*)malloc(sizeof(c)))
#endif

static void
check_umlattr (umlattribute *u, char *typename)
{
    if (u->visibility == '1')
        fprintf (stderr, "%s/%s: ignoring non-visibility\n",
                         typename, u->name);
    if (u->isstatic)
        fprintf (stderr, "%s/%s: ignoring staticness\n",
                         typename, u->name);
}

/* Package processing auxiliary structure:
   IDL declarations are UML packages (which map to IDL modules)
   or UML classes (which map to various other IDL constructs.)
   For each top level declaration, a separate file is generated.
   Nested packages are generated into the same file as nested modules.
   `struct declaration' is a buffer that gathers together classes and
   packages ordered by their dependencies, going from least dependendcies
   to most.  We cannot handle circular dependencies yet.  */

struct declaration;

struct module {  /* IDL module = UML package */
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
        /* In `this_class', only the `key' and `parents' are currently used.
           `next' is not used since we use our own sequencing (see `prev'
           and `next' below.)  */
    } u;
    struct declaration *prev, *next;  /* other declarations in this scope */
};

typedef struct declaration declaration;

static declaration *decls = NULL;

static FILE * outfile;
static int indentlevel = 0;

/********* includefile related stuff *******************/
static namelist includes = NULL;

static int
have_include (char *name)
{
    namelist inc = includes;
    while (inc) {
        if (!strcmp (inc->name, name))
            return 1;
        inc = inc->next;
    }
    return 0;
}

static void
add_include (char *name)
{
    namelist inc = includes;

    if (have_include (name))
        return;
    if (inc == NULL) {
        includes = NEW (namenode);
        inc = includes;
    } else {
        while (inc->next)
            inc = inc->next;
        inc->next = NEW (namenode);
        inc = inc->next;
    }
    inc->name = name;
    inc->next = NULL;
}

static void
push_include (umlclassnode *node)
{
    if (node->key->package != NULL) {
        umlpackagelist pkglist = make_package_list (node->key->package);
        add_include (pkglist->key->name);
    } else {
        add_include (node->key->name);
    }
}

static void
determine_includes (declaration *d, batch *b)
{
    if (d->decl_kind == dk_module) {
        declaration *inner = d->u.this_module->contents;
        while (inner != NULL) {
            determine_includes (inner, b);
            inner = inner->next;
        }
    } else {
        umlclasslist cl = list_classes (d->u.this_class, b);
        while (cl != NULL) {
            push_include (cl);
            cl = cl->next;
        }
    }
}
/********* end of includefile related stuff *************/

static char *
spc ()
{
   static char spaces[132];
   int n_spaces = 2 * indentlevel;
   if (n_spaces >= sizeof(spaces)) {
       fprintf (stderr, "spc(): spaces buffer overflow\n");
       exit (1);
   }
   memset (spaces, ' ', n_spaces);
   spaces[n_spaces] = '\0';
   return spaces;
}

static module *
create_nested_modules_from_pkglist (umlpackagelist pkglist, module *m)
{   /* Expects pklist and m to be non-NULL and m->contents to be NULL. */
    while (pkglist->next != NULL) {
        declaration *d = NEW (declaration);
        d->decl_kind = dk_module;
        d->prev = d->next = NULL;
        d->u.this_module = NEW (module);
        m->contents = d;
        pkglist = pkglist->next;
        m = d->u.this_module;
        m->pkg = pkglist->key;
        m->contents = NULL;
    }
    return m;
}

static module *
find_or_add_module (declaration **dptr, umlpackagelist pkglist)
{
    declaration *d = *dptr;
    module *m;

    if (pkglist == NULL)
        return NULL;
    if (d == NULL) {
        *dptr = NEW (declaration);
        d = *dptr;
    } else {
        declaration *dprev = NULL;
        while (d != NULL) {
            if (d->decl_kind == dk_module &&
                !strcmp (d->u.this_module->pkg->name, pkglist->key->name)) {
                m = d->u.this_module;
                if (pkglist->next == NULL)
                    return m;
                if (m->contents == NULL) {
                    return create_nested_modules_from_pkglist (pkglist, m);
                }
                return find_or_add_module (&m->contents, pkglist->next);
            }
            dprev = d;
            d = d->next;
        }
        if (dprev != NULL) {
            dprev->next = NEW (declaration);
            d = dprev->next;
        }
    }
    d->decl_kind = dk_module;
    d->next = NULL;
    d->u.this_module = NEW (module);
    m = d->u.this_module;
    m->pkg = pkglist->key;
    m->contents = NULL;
    return create_nested_modules_from_pkglist (pkglist, m);
}

static module *
find_module (declaration *d, umlpackagelist pkglist)
{
    while (d != NULL) {
        if (d->decl_kind == dk_module) {
            module *m = d->u.this_module;
            if (!strcmp (m->pkg->name, pkglist->key->name)) {
                if (pkglist->next != NULL)
                    return find_module (m->contents, pkglist->next);
                else
                    return m;
            }
        }
        d = d->next;
    }
    return NULL;
}

static declaration *
find_class (umlclassnode *node)
{
    declaration *d;

    if (node->key->package != NULL) {
        umlpackagelist pkglist = make_package_list (node->key->package);
        module *m = find_module (decls, pkglist);
        if (m == NULL || m->contents == NULL)
            return 0;
        d = m->contents;
    } else {
        d = decls;
    }

    while (d != NULL) {
        if (d->decl_kind == dk_class) {
            umlclassnode *cl = d->u.this_class;
            if (!strcmp (cl->key->name, node->key->name))
                return d;
        }
        d = d->next;
    }
    return NULL;
}

static declaration *
append_decl (declaration *d)
{
    while (d->next != NULL) {
        d = d->next;
    }
    d->next = NEW (declaration);
    d->next->prev = d;
    d = d->next;
    return d;
}

static void
push (umlclassnode *node, batch *b)
{
    umlclasslist used_classes, tmpnode;
    module *m;
    declaration *d;

    if (node == NULL || find_class (node) != NULL) {
        return;
    }

    used_classes = list_classes (node, b);
    /* Make sure all classes that this one depends on are already pushed. */
    tmpnode = used_classes;
    while (tmpnode != NULL) {
        push (tmpnode, b);
        tmpnode = tmpnode->next;
    }

    if (node->key->package != NULL) {
        umlpackagelist pkglist = make_package_list (node->key->package);
        m = find_or_add_module (&decls, pkglist);
        if (m->contents == NULL) {
            m->contents = NEW (declaration);
            d = m->contents;
            d->prev = NULL;
        } else {
            /* We can simply append because all classes that we depend on
               are already pushed. */
            d = append_decl (m->contents);
        }
    } else {
        if (decls == NULL) {
            decls = NEW (declaration);
            d = decls;
            d->prev = NULL;
        } else {
            d = append_decl (decls);
            /* We can simply append because all classes that we depend on
               are already pushed. */
        }
    }
    d->decl_kind = dk_class;
    d->next = NULL;
    d->u.this_class = NEW (umlclassnode);
    memcpy (d->u.this_class, node, sizeof(umlclassnode));
}

static void
do_operations (FILE *outfile, char *typename, umloplist umlo)
{
    if (umlo == NULL)
        return;

    fprintf(outfile, "  // Operations\n");

    while (umlo != NULL) {
        umlattrlist tmpa = umlo->key.parameters;

        fprintf (outfile, "%s", spc());

        if (umlo->key.attr.isabstract) {
            fprintf (stderr, "ignoring abstractness\n");
            /* umlo->key.attr.value[0] = '0'; */
        }
        check_umlattr (&umlo->key.attr, typename);

        if (strlen (umlo->key.attr.type) > 0)
            fprintf (outfile, "%s ", umlo->key.attr.type);
        else
            fprintf (outfile, "void ");
        fprintf (outfile, "%s (", umlo->key.attr.name);

        while (tmpa != NULL) {
            fprintf(outfile, "in %s %s", tmpa->key.type, tmpa->key.name);
            if (tmpa->key.value[0] != 0)
                fprintf (stderr, "ignoring default value %s\n",
                                 tmpa->key.value);
            tmpa = tmpa->next;
            if (tmpa != NULL) {
                fprintf(outfile, ", ");
            }
        }

        fprintf (outfile, ")");

        if (umlo->key.attr.value[0] != 0)
            fprintf (stderr, "ignoring default value %s",
                             umlo->key.attr.value);
        fprintf(outfile, ";\n");
        umlo = umlo->next;
    }
}


static void
close_scope ()
{
    indentlevel--;
    fprintf (outfile, "%s};\n\n", spc ());
}

static void
gen_interface (umlclassnode *node)
{
    char *typename = node->key->name;

    fprintf(outfile, "%sinterface %s", spc(), typename);

    if (node->parents != NULL) {
        umlclasslist parents = node->parents;
        fprintf(outfile, ":");
        while ( parents != NULL ) {
            fprintf(outfile, " %s", parents->key->name);
            parents = parents->next;
            if ( parents != NULL ) {
                fprintf(outfile, ", ");
            }
        }
    }
    fprintf(outfile, " {\n");
    indentlevel++;

    if (node->associations) {
        umlassoclist associations = node->associations;
        fprintf(outfile, "%s// Associations\n", spc());
        while (associations != NULL) {
            fprintf(outfile, "%s%s %s;\n", spc(),
                    associations->key->name, associations->name);
            associations = associations->next;
        }
    }

    if (node->key->attributes != NULL) {
        umlattrlist umla = node->key->attributes;
        fprintf(outfile, "%s// Attributes\n", spc());
        while (umla != NULL) {
            check_umlattr (&umla->key, typename);
            fprintf(outfile, "%sattribute %s %s;\n", spc(),
                             umla->key.type, umla->key.name);
            umla = umla->next;
        }
    }

    do_operations (outfile, typename, node->key->operations);
    close_scope ();
}


static void
gen_decl (declaration *d)
{
    char *name;
    char *stype;
    umlclassnode *node;
    umlattrlist umla;

    if (d == NULL)
        return;

    if (d->decl_kind == dk_module) {
        name = d->u.this_module->pkg->name;
        fprintf (outfile, "%smodule %s {\n", spc(), name);
        indentlevel++;
        d = d->u.this_module->contents;
        while (d != NULL) {
            gen_decl (d);
            d = d->next;
        }
        close_scope ();
        return;
    }

    node = d->u.this_class;
    stype = node->key->stereotype;
    name = node->key->name;
    umla = node->key->attributes;

    if (strlen (stype) == 0) {
        gen_interface (node);
        return;
    }

    if (!strcmp (stype, "CORBAEnum")) {
        fprintf (outfile, "%senum %s {\n", spc(), name);
        indentlevel++;
        while (umla != NULL) {
            char *literal = umla->key.name;
            check_umlattr (&umla->key, name);
            if (strlen (umla->key.type) > 0)
                fprintf (stderr, "%s/%s: ignoring type\n", name, literal);
            fprintf (outfile, "%s%s", spc(), literal);
            if (umla->next)
                fprintf (outfile, ",");
            fprintf (outfile, "\n");
            umla = umla->next;
        }
        close_scope ();

    } else if (!strcmp (stype, "CORBAStruct")) {
        fprintf (outfile, "%sstruct %s {\n", spc(), name);
        indentlevel++;
        while (umla != NULL) {
            char *member = umla->key.name;
            check_umlattr (&umla->key, name);
            fprintf (outfile, "%s%s %s;\n", spc(), umla->key.type, member);
            umla = umla->next;
        }
        close_scope ();

    } else if (!strcmp (stype, "CORBAUnion")) {
        /********** TO BE DONE **********************************/
        fprintf (outfile, "%sunion %s switch () {\n", spc(), name);
        indentlevel++;
        while (umla != NULL) {
            char *member = umla->key.name;
            check_umlattr (&umla->key, name);
            fprintf (outfile, "%s%s %s;\n", spc(), umla->key.type, member);
            umla = umla->next;
        }
        close_scope ();

    } else if (!strcmp (stype, "CORBAValue")) {
        fprintf (outfile, "%s", spc());
        if (node->key->isabstract) {
            fprintf (outfile, "abstract ");
        }
        fprintf (outfile, "valuetype %s", name);
        if (node->parents != NULL) {
            umlclasslist parents = node->parents;
            fprintf(outfile, " :");
            while (parents != NULL) {
                fprintf(outfile, " %s", parents->key->name);
                parents = parents->next;
                if (parents != NULL) {
                    fprintf (outfile, ", ");
                }
            }
        }
        fprintf (outfile, " {\n");
        indentlevel++;
        if (node->associations) {
            umlassoclist associations = node->associations;
            fprintf(outfile, "%s// Associations\n", spc());
            while (associations != NULL) {
                fprintf(outfile, "%spublic %s %s;\n", spc(),
                        associations->key->name, associations->name);
                associations = associations->next;
            }
        }
        if (umla) {
            fprintf(outfile, "%s// State members\n", spc());
            while (umla != NULL) {
                char *member = umla->key.name;
                if (umla->key.isstatic)
                    fprintf (stderr, "%s/%s: ignoring staticness\n",
                                     name, member);
                fprintf (outfile, "%s", spc());
                if (umla->key.visibility == '1')
                    fprintf (outfile, "private");
                else
                    fprintf (outfile, "public");
                fprintf (outfile, " %s %s;\n", umla->key.type, member);
                umla = umla->next;
            }
        }
        do_operations (outfile, name, node->key->operations);
        close_scope ();
    } else {
        fprintf (outfile, "%s// %s\n", spc(), stype);
        gen_interface (node);
    }
}


static void
put_hfence (char *name)
{
    char *tmpname = strtoupper (name);
    fprintf (outfile, "#ifndef %s_IDL\n", tmpname);
    fprintf (outfile, "#define %s_IDL\n\n", tmpname);
    free (tmpname);
}

/* open_outfile() returns 1 if the file exists and is not rewritten
 * due to a clobber prohibition. Returns 0 if the file is rewritten.
 */
static int
open_outfile (char *basename, batch *b)
{
    static char outfilename[256];
    FILE *dummyfile;
    int tmpdirlgth, tmpfilelgth;

    if (b->outdir == NULL) {
        b->outdir = ".";
    }

    tmpdirlgth = strlen (b->outdir);
    tmpfilelgth = strlen (basename);

    /* This prevents buffer overflows */
    if (tmpfilelgth + tmpdirlgth > sizeof(*outfilename) - 2) {
        fprintf (stderr, "Sorry, name of file too long ...\n"
                         "Try a smaller dir name\n");
        exit (1);
    }

    sprintf (outfilename, "%s/%s.idl", b->outdir, basename);
    dummyfile = fopen (outfilename, "r");
    if (dummyfile && !b->clobber) {
        fclose (dummyfile);
        return 1;
    }
    outfile = fopen (outfilename, "w");
    if (outfile == NULL) {
        fprintf (stderr, "Can't open file %s for writing\n", outfilename);
        exit (1);
    }
    return 0;
}


void generate_code_idl (batch *b) {
    declaration *d;
    umlclasslist tmplist;
    char *name;
    FILE *licensefile = NULL;

    tmplist = b->classlist;

    /* open license file */
    if ( b->license != NULL ) {
        licensefile = fopen(b->license, "r");
        if(!licensefile) {
            fprintf(stderr, "Can't open the license file.\n");
            exit(2);
        }
    }

    while (tmplist != NULL) {
        name = tmplist->key->name;
        if (! (is_present (b->classes, name) ^ b->mask)) {
            push (tmplist, b);
        }
        tmplist = tmplist->next;
    }

    /* Generate a file for each outer declaration.
       Rose does it more elegantly. They use components for the files,
       and realizations for deciding which decl goes into which file.  */
    d = decls;
    while (d != NULL) {
        if (d->decl_kind == dk_module) {
            name = d->u.this_module->pkg->name;
        } else {         /* dk_class */
            name = d->u.this_class->key->name;
        }
        /* name = strtolower(name); */
        if (open_outfile (name, b)) {
            d = d->next;
            continue;
        }

        /* add license to the header */
        if (b->license != NULL) {
            char lc;
            rewind (licensefile);
            while ((lc = fgetc(licensefile)) != EOF)
                fprintf(outfile,"%c",lc);
        }

        put_hfence (name);

        includes = NULL;
        determine_includes (d, b);
        if (includes) {
            namelist incfile = includes;
            while (incfile != NULL) {
                if (strcmp (incfile->name, name)) {
                    fprintf(outfile, "#include \"%s.idl\"\n", incfile->name);
                }
                incfile = incfile->next;
            }
            fprintf(outfile, "\n");
        }

        gen_decl (d);

        fprintf (outfile, "#endif\n");   /* from hfence */
        fclose (outfile);

        d = d->next;
    }
}
