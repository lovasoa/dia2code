/***************************************************************************
         generate_code_idl.c  -  Function that generates CORBA IDL
                             -------------------
    begin                : Sun Dec 30 2001
    copyright            : (C) 2000-2001 by Chris McGee
                           (C) 2002 by Oliver Kellogg
    email                : sirnewton_01@yahoo.ca
                           okellogg@users.sourceforge.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <stdarg.h>

#include "dia2code.h"
#include "decls.h"
#include "includes.h"

static void
check_umlattr (umlattribute *u, char *typename)
{
    /* Check settings that don't make sense for IDL generation.  */
    if (u->visibility == '1')
        fprintf (stderr, "%s/%s: ignoring non-visibility\n", typename, u->name);
    if (u->isstatic)
        fprintf (stderr, "%s/%s: ignoring staticness\n", typename, u->name);
}


static void
do_operations (char *typename, umloplist umlo)
{
    if (umlo == NULL)
        return;

    print ("// Operations\n");

    while (umlo != NULL) {
        umlattrlist tmpa = umlo->key.parameters;

        print ("");

        if (umlo->key.attr.isabstract) {
            fprintf (stderr, "ignoring abstractness\n");
            /* umlo->key.attr.value[0] = '0'; */
        }
        check_umlattr (&umlo->key.attr, typename);

        if (strlen (umlo->key.attr.type) > 0)
            emit ("%s ", umlo->key.attr.type);
        else
            emit ("void ");
        emit ("%s (", umlo->key.attr.name);

        while (tmpa != NULL) {
            emit ("in %s %s", tmpa->key.type, tmpa->key.name);
            if (tmpa->key.value[0] != 0)
                fprintf (stderr, "ignoring default value %s\n", tmpa->key.value);
            tmpa = tmpa->next;
            if (tmpa != NULL) {
                emit (", ");
            }
        }

        emit (")");

        if (umlo->key.attr.value[0] != 0)
            fprintf (stderr, "ignoring default value %s", umlo->key.attr.value);
        emit (";\n");
        umlo = umlo->next;
    }
}


static void
close_scope ()
{
    indentlevel--;
    print ("};\n\n");
}

static void
gen_interface (umlclassnode *node)
{
    char *typename = node->key->name;

    print ("interface %s", typename);

    if (node->parents != NULL) {
        umlclasslist parents = node->parents;
        emit (":");
        while (parents != NULL) {
            emit (" %s", parents->key->name);
            parents = parents->next;
            if (parents != NULL) {
                emit (", ");
            }
        }
    }
    emit (" {\n");
    indentlevel++;

    if (node->associations) {
        umlassoclist associations = node->associations;
        print ("// Associations\n");
        while (associations != NULL) {
            /* Explicit usage of associations->composite is not
               supported by IDL. Perhaps we could give some
               intelligent warning here.  */
            print ("readonly attribute%s %s;\n",
                    associations->key->name, associations->name);
            associations = associations->next;
        }
    }

    if (node->key->attributes != NULL) {
        umlattrlist umla = node->key->attributes;
        print ("// Attributes\n");
        while (umla != NULL) {
            check_umlattr (&umla->key, typename);
            print ("attribute %s %s;\n", umla->key.type, umla->key.name);
            umla = umla->next;
        }
    }

    do_operations (typename, node->key->operations);
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
        print ("module %s {\n", name);
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

    if (eq (stype, "CORBANative")) {
        print ("native %s;\n\n", name);

    } else if (is_const_stereo (stype)) {
        if (umla == NULL) {
            fprintf (stderr, "Error: first attribute not set at const %s\n", name);
            exit (1);
        }
        if (strlen (umla->key.name) > 0)
            fprintf (stderr, "Warning: ignoring attribute name at const %s\n", name);

        print ("const %s %s = %s;\n\n", umla->key.type, name, umla->key.value);

    } else if (is_enum_stereo (stype)) {
        print ("enum %s {\n", name);
        indentlevel++;
        while (umla != NULL) {
            char *literal = umla->key.name;
            check_umlattr (&umla->key, name);
            if (strlen (umla->key.type) > 0)
                fprintf (stderr, "%s/%s: ignoring type\n", name, literal);
            print ("%s", literal);
            if (umla->next)
                emit (",");
            emit ("\n");
            umla = umla->next;
        }
        close_scope ();

    } else if (is_struct_stereo (stype) ||
               eq (stype, "CORBAException")) {
        int corba_ofst = strncmp (stype, "CORBA", 5) == 0 ? 5 : 0;
        char *keyword = strtolower (stype + corba_ofst);
        print ("%s %s {\n", keyword, name);
        indentlevel++;
        while (umla != NULL) {
            char *member = umla->key.name;
            check_umlattr (&umla->key, name);
            print ("%s %s;\n", umla->key.type, member);
            umla = umla->next;
        }
        close_scope ();

    } else if (eq (stype, "CORBAUnion")) {
        umlattrnode *sw = umla;
        if (sw == NULL) {
            fprintf (stderr, "Error: attributes not set at union %s\n", name);
            exit (1);
        }
        if (strlen (sw->key.name) > 0 && !eq (sw->key.name, "switch")) {
            emit ("%s// #switchname %s %s\n\n", name, sw->key.name);
        }
        print ("union %s switch (%s) {\n", name, sw->key.type);
        indentlevel++;
        umla = umla->next;
        while (umla != NULL) {
            char *member = umla->key.name;
            check_umlattr (&umla->key, name);
            print ("case %s:\n", umla->key.value);
            print ("  %s %s;\n", umla->key.type, member);
            umla = umla->next;
        }
        close_scope ();

    } else if (is_typedef_stereo (stype)) {
        /* Conventions for CORBATypedef:
           The first (and only) attribute contains the following:
           Name:   Empty - the name is taken from the class.
           Type:   Name of the original type which is typedefed.
           Value:  Optionally contains array dimension(s) of the typedef.
                   These dimensions are given in square brackets, e.g.
                   [3][10]
         */
        if (umla == NULL) {
            fprintf (stderr, "Error: first attribute (impl type) not set "
                             "at typedef %s\n", name);
            exit (1);
        }
        if (umla->key.name != NULL && strlen (umla->key.name) > 0)  {
            fprintf (stderr, "Warning: typedef %s: ignoring name field "
                             "in implementation type attribute\n", name);
        }
        print ("typedef %s %s%s;\n\n", umla->key.type, name, umla->key.value);

    } else if (eq (stype, "CORBAValue")) {
        print ("");
        if (node->key->isabstract) {
            emit ("abstract ");
        }
        emit ("valuetype %s", name);
        if (node->parents != NULL) {
            umlclasslist parents = node->parents;
            emit (" :");
            while (parents != NULL) {
                emit (" %s", parents->key->name);
                parents = parents->next;
                if (parents != NULL) {
                    emit (", ");
                }
            }
        }
        emit (" {\n");
        indentlevel++;
        if (node->associations) {
            umlassoclist associations = node->associations;
            print ("// Associations\n");
            while (associations != NULL) {
                /* Explicit usage of associations->composite is not
                   supported by IDL. Perhaps we could give some
                   intelligent warning here.  */
                /* How to map valuetype associations is still an issue of
                   discussion.  For now, we follow the generate_code_java
                   style and map them to private members.
                   FIXME: The user currently has no control over the order
                   in which the members are generated.  */
                print ("private %s %s;\n",
                        associations->key->name, associations->name);
                associations = associations->next;
            }
        }
        if (umla) {
            print ("// State members\n");
            while (umla != NULL) {
                char *member = umla->key.name;
                if (umla->key.isstatic)
                    fprintf (stderr, "%s/%s: ignoring staticness\n", name, member);
                print ("");
                if (umla->key.visibility == '1')
                    emit ("private");
                else
                    emit ("public");
                emit (" %s %s;\n", umla->key.type, member);
                umla = umla->next;
            }
        }
        do_operations (name, node->key->operations);
        close_scope ();
    } else {
        print ("// %s\n", stype);
        gen_interface (node);
    }
}


static void
put_hfence (char *name)
{
    char *tmpname = strtoupper (name);
    emit ("#ifndef %s_IDL\n", tmpname);
    emit ("#define %s_IDL\n\n", tmpname);
    free (tmpname);
}

void
generate_code_idl (batch *b)
{
    declaration *d;
    umlclasslist tmplist = b->classlist;

    FILE *licensefile = NULL;

    if (file_ext == NULL)
        file_ext = "idl";

    /* open license file */
    if ( b->license != NULL ) {
        licensefile = fopen(b->license, "r");
        if(!licensefile) {
            fprintf (stderr, "Can't open the license file.\n");
            exit(2);
        }
    }

    while (tmplist != NULL) {
        char *name = tmplist->key->name;
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
        char *name;
        char filename[BIG_BUFFER];

        if (d->decl_kind == dk_module) {
            name = d->u.this_module->pkg->name;
        } else {         /* dk_class */
            name = d->u.this_class->key->name;
        }

        sprintf (filename, "%s.%s", name, file_ext);
        /**
         * The filename is no longer lowercased.
         *  Selectively do this as soon as we have a
         *  command line option for it.
        */

        spec = open_outfile (filename, b);
        if (spec == NULL) {
            d = d->next;
            continue;
        }

        /* add license to the header */
        if (b->license != NULL) {
            char lc;
            rewind (licensefile);
            while ((lc = fgetc (licensefile)) != EOF)
                emit ("%c",lc);
        }

        put_hfence (name);

        includes = NULL;
        determine_includes (d, b);
        if (includes) {
            namelist incfile = includes;
            while (incfile != NULL) {
                if (!eq (incfile->name, name)) {
                    emit ("#include \"%s.%s\"\n", incfile->name, file_ext);
                }
                incfile = incfile->next;
            }
            emit ("\n");
        }

        gen_decl (d);

        emit ("#endif\n");   /* from hfence */
        fclose (spec);

        d = d->next;
    }
}
