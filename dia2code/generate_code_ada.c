/***************************************************************************
                          generate_code_ada.c  -  description
                             -------------------
    begin                : 2001-07-18
    copyright            : (C) 2001 by Thomas Preymesser
                           (C) 2002 by Oliver Kellogg
    email                : tp@thopre.de
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

#include "dia2code.h"
#include "decls.h"
#include "includes.h"

#define PACKAGE_EXT "_types"
#define SPEC_EXT "ads"
#define BODY_EXT "adb"

int use_convention_c = 0;  /* Temporarily here. */
int strict_corba = 0;      /* Should be turned into command line options.  */


static batch *gb;   /* The current batch being processed.  */

/* Utilities.  */

static void
check_umlattr (umlattribute *u, char *typename)
{
    /* Check settings that don't make sense for Ada generation.  */
    if (u->visibility == '1')
        fprintf (stderr, "%s/%s: ignoring non-visibility\n", typename, u->name);
    if (u->isstatic)
        fprintf (stderr, "%s/%s: ignoring staticness\n", typename, u->name);
}

static char *
subst (char *str, char search, char replace)
{
    char *p;
    while ((p = strchr (str, search)) != NULL)
        *p = replace;
    return str;
}

static char *
nospc (char *str)
{
    return subst (str, ' ', '_');
}

static int
is_oo_class (umlclass *cl)
{
    char *st;
    if (cl == NULL)
        return 0;
    st = cl->stereotype;
    if (strlen (st) == 0)
        return 1;
    return (!is_const_stereo (st) &&
            !is_typedef_stereo (st) &&
            !is_enum_stereo (st) &&
            !is_struct_stereo (st) &&
            !eq (st, "CORBAUnion") &&
            !eq (st, "CORBAException"));
}

static int
has_oo_class (declaration *d)
{
    while (d != NULL) {
        if (d->decl_kind == dk_module) {
            if (has_oo_class (d->u.this_module->contents))
                return 1;
        } else {         /* dk_class */
            if (is_oo_class (d->u.this_class->key))
                return 1;
        }
        d = d->next;
    }
    return 0;
}

static char *
adaname (char *name)
{
    static char buf[SMALL_BUFFER];
    if (use_corba &&
        (eq (name, "boolean") ||
         eq (name, "char") ||
         eq (name, "octet") ||
         eq (name, "short") ||
         eq (name, "long") ||
         eq (name, "long long") ||
         eq (name, "unsigned short") ||
         eq (name, "unsigned long") ||
         eq (name, "unsigned long long") ||
         eq (name, "float") ||
         eq (name, "double") ||
         eq (name, "string") ||
         eq (name, "any"))) {
        sprintf (buf, "CORBA.%s", nospc (strtoupperfirst (name)));
    } else {
        umlclassnode *ref = find_by_name (gb->classlist, name);
        strcpy (buf, name);
        if (ref != NULL && is_oo_class (ref->key))
            strcat (buf, ".Value_Ref");
    }
    return buf;
}


static void
do_operations (char *typename, umloplist umlo, int in_valuetype)
{
    if (umlo == NULL)
        return;

    print ("-- Operations\n\n");

    while (umlo != NULL) {
        int use_procedure = (strlen (umlo->key.attr.type) == 0 ||
                                 eq (umlo->key.attr.type, "void"));
        umlattrlist parm = umlo->key.parameters;

        print ("");
        if (use_procedure)
            emit ("procedure");
        else
            emit ("function ");
        emit (" %s (", umlo->key.attr.name);
        if (! umlo->key.attr.isstatic) {
            emit ("Self : access Object");
            if (parm != NULL)
                emit (";\n");
        } else {
            emit ("\n");
        }
        indentlevel += 5;

        while (parm != NULL) {
            /* FIXME: Add support for parameter modes in dia.  */
            print ("%s : in %s", parm->key.name, adaname (parm->key.type));
            if (parm->key.value[0] != 0)
                emit (" := %s", parm->key.value);
            parm = parm->next;
            if (parm != NULL) {
                emit (";\n");
            }
        }

        emit (")");

        if (! use_procedure)
            emit (" return %s", adaname (umlo->key.attr.type));

        if (umlo->key.attr.isabstract || in_valuetype)
            emit (" is abstract");
        /* TBH, we have no way of generating a meaningful implementation.
           Instead, the user shall derive from this class and implement the
           UML defined methods in the derived class.  */

        emit (";\n\n");
        indentlevel -= 5;
        umlo = umlo->next;
    }
}


static char *
fqname (umlclassnode *node, int use_ref_type)
{
    static char buf[BIG_BUFFER];

    buf[0] = '\0';
    if (node == NULL)
        return buf;
    if (node->key->package != NULL) {
        umlpackagelist pkglist = make_package_list (node->key->package);
        while (pkglist != NULL) {
            strcat (buf, pkglist->key->name);
            strcat (buf, ".");
            pkglist = pkglist->next;
        }
    }
    strcat (buf, node->key->name);
    if (is_oo_class (node->key)) {
        if (use_ref_type)
            strcat (buf, ".Value_Ref");
        else
            strcat (buf, ".Object");
    }
    return buf;
}

static int
gen_static_attributes (umlattrlist umla, int do_private)
{
    int number_of_static_attributes = 0;
    int did_output = 0;

    while (umla != NULL) {
        char *member = umla->key.name;
        umlclassnode *ref;

        if (!umla->key.isstatic ||
            (umla->key.visibility != '0' && !do_private)) {
            umla = umla->next;
            continue;
        }
        number_of_static_attributes++;
        if (! did_output) {
            print ("-- Static attributes\n\n");
            did_output = 1;
        }
        ref = find_by_name (gb->classlist, umla->key.type);
        if (umla->key.visibility == '0' && !do_private) {
            pboth ("procedure Set_%s (To : ", member);
            if (ref != NULL)
                eboth ("%s", fqname (ref, 1));
            else
                eboth ("%s", adaname (umla->key.type));
            eboth (")");
            emit (";\n");
            ebody (" is\n");
            pbody ("begin\n");
            pbody ("   %s := To;\n", member);
            pbody ("end Set_%s;\n\n", member);
            pboth ("function Get_%s return ", member);
            if (ref != NULL)
                eboth ("%s", fqname (ref, 1));
            else
                eboth ("%s", adaname (umla->key.type));
            emit (";\n\n");
            ebody (" is\n");
            pbody ("begin\n");
            pbody ("   return %s;\n", member);
            pbody ("end Get_%s;\n\n", member);
        }
        if (do_private) {
            print ("%s : ", member);
            if (ref != NULL)
                emit ("%s", fqname (ref, 0));
            else
                emit ("%s", adaname (umla->key.type));
            emit (";\n\n");
        }
        umla = umla->next;
    }
    return number_of_static_attributes;
}

static void
gen_class (umlclassnode *node, int do_valuetype)
{
    char *name = node->key->name;
    char parentname[SMALL_BUFFER];
    int n_static_attrs;

    pboth ("package ");
    ebody ("body ");
    eboth ("%s is\n\n", name);
    indentlevel++;
    n_static_attrs = gen_static_attributes (node->key->attributes, 0);
    if (do_valuetype && n_static_attrs > 0) {
        fprintf (stderr, "Static attributes not permitted for %s\n", name);
    }
    print ("type Object is ");
    if (node->key->isabstract)
        emit ("abstract ");
    parentname[0] = '\0';
    if (node->parents != NULL) {
        umlclassnode *parent = node->parents;
        sprintf (parentname, "%s.Object", parent->key->name);
        if (parent->next != NULL)
            fprintf (stderr, "Warning: multiple inheritance NYI (%s)\n", name);
    } else if (do_valuetype) {
        sprintf (parentname, "CORBA.Value.Base");
    }
    if (parentname[0])
        emit ("new %s with", parentname);
    else
        emit ("tagged");
    emit (" private;\n\n");
    print ("type Value_Ref is access all Object'Class;\n\n");
    if (node->key->attributes) {
        umlattrlist umla = node->key->attributes;
        int did_output = 0;
        while (umla != NULL) {
            char *member = umla->key.name;
            umlclassnode *ref;
    
            if (umla->key.visibility != '0' || umla->key.isstatic) {
                umla = umla->next;
                continue;
            }
            if (! did_output) {
                print ("-- Public attributes\n\n");
                did_output = 1;
            }
            pboth ("procedure Set_%s (Self : access Object; To : ", member);
            ref = find_by_name (gb->classlist, umla->key.type);
            if (ref != NULL)
                eboth ("%s", fqname (ref, 1));
            else
                eboth ("%s", adaname (umla->key.type));
            eboth (")");
            emit (";\n");
            ebody (" is\n");
            pbody ("begin\n");
            pbody ("   Self.%s := To;\n", member);
            pbody ("end Set_%s;\n\n", member);
            pboth ("function Get_%s (Self : access Object) return ", member);
            if (ref != NULL)
                eboth ("%s", fqname (ref, 1));
            else
                eboth ("%s", adaname (umla->key.type));
            emit (";\n\n");
            ebody (" is\n");
            pbody ("begin\n");
            pbody ("   return Self.%s;\n", member);
            pbody ("end Get_%s;\n\n", member);
            umla = umla->next;
        }
    }
    do_operations (name, node->key->operations, do_valuetype);
    indentlevel--;
    print ("private\n\n");
    indentlevel++;
    n_static_attrs = gen_static_attributes (node->key->attributes, 1);
    if (do_valuetype && n_static_attrs > 0) {
        fprintf (stderr, "Static attributes not permitted for %s\n", name);
    }
    print ("type Object is ");
    if (node->key->isabstract)
        emit ("abstract ");
    if (parentname[0])
        emit ("new %s with", parentname);
    else
        emit ("tagged");
    emit (" record\n");
    indentlevel++;
    if (node->key->attributes) {
        umlattrlist umla = node->key->attributes;
        print ("-- Attributes\n");
        while (umla != NULL) {
            umlclassnode *ref;
            if (umla->key.isstatic) {
                umla = umla->next;
                continue;
            }
            print ("%s : ", umla->key.name);
            ref = find_by_name (gb->classlist, umla->key.type);
            if (ref != NULL)
                emit ("%s", fqname (ref, 1));
            else
                emit ("%s", adaname (umla->key.type));
            emit (";\n");
            umla = umla->next;
        }
    } else if (node->associations == NULL) {
        print ("null;\n");
    }
    if (node->associations) {
        umlassoclist assoc = node->associations;
        print ("-- Associations\n");
        while (assoc != NULL) {
            umlclassnode *ref;
            ref = find_by_name (gb->classlist, assoc->key->name);
            print ("%s : ", assoc->name);
            if (ref != NULL) {
                if (is_oo_class (ref->key) && do_valuetype && assoc->composite)
                    fprintf (stderr, "Association %s cannot be composite\n",
                                     assoc->key->name);
                emit ("%s", fqname (ref, !assoc->composite));
            } else {
                emit ("%s", adaname (assoc->key->name));
            }
            emit (";\n");
            assoc = assoc->next;
        }
    }
    indentlevel--;
    print ("end record;\n\n");
    indentlevel--;
    pboth ("end %s;\n\n", name);
}


static void
convention_c (char *name)
{
    if (use_convention_c)
        print ("pragma Convention (C, %s);\n", name);
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
        pboth ("package ");
        ebody ("body ");
        eboth ("%s is\n\n", name);
        indentlevel++;
        d = d->u.this_module->contents;
        while (d != NULL) {
            gen_decl (d);
            d = d->next;
        }
        indentlevel--;
        pboth ("end %s;\n\n", name);
        return;
    }

    node = d->u.this_class;
    stype = node->key->stereotype;
    name = node->key->name;
    umla = node->key->attributes;

    if (strlen (stype) == 0) {
        gen_class (node, 0);
        return;
    }

    if (eq (stype, "CORBANative")) {
        print ("-- CORBANative: %s\n\n", name);

    } else if (is_const_stereo (stype)) {
        if (umla == NULL) {
            fprintf (stderr, "Error: first attribute not set at %s\n", name);
            exit (1);
        }
        if (strlen (umla->key.name) > 0)
            fprintf (stderr, "Warning: ignoring attribute name at %s\n", name);

        print ("%s : constant %s := %s;\n\n", name, adaname (umla->key.type),
                                                             umla->key.value);

    } else if (is_enum_stereo (stype)) {
        print ("type %s is (\n", name);
        indentlevel++;
        while (umla != NULL) {
            char *literal = umla->key.name;
            check_umlattr (&umla->key, name);
            if (strlen (umla->key.type) > 0)
                fprintf (stderr, "%s/%s: ignoring type\n", name, literal);
            print ("%s", literal);
            if (umla->next)
                emit (",\n");
            umla = umla->next;
        }
        emit (");\n");
        indentlevel--;
        convention_c (name);
        emit ("\n");

    } else if (is_struct_stereo (stype)) {
        print ("type %s is record\n", name);
        indentlevel++;
        while (umla != NULL) {
            check_umlattr (&umla->key, name);
            print ("%s : %s", umla->key.name, adaname (umla->key.type));
            if (strlen (umla->key.value) > 0)
                emit (" := %s", umla->key.value);
            emit (";\n");
            umla = umla->next;
        }
        indentlevel--;
        print ("end record;\n");
        convention_c (name);
        emit ("\n");

    } else if (eq (stype, "CORBAException")) {
        print ("%s : exception;\n\n", name);
        if (strict_corba) {
            print ("type %s_Members is new CORBA.IDL_Exception_Members"
                   " with record\n", name);
            indentlevel++;
            while (umla != NULL) {
                check_umlattr (&umla->key, name);
                print ("%s : %s;\n", umla->key.name, adaname (umla->key.type));
                umla = umla->next;
            }
            indentlevel--;
            print ("end record;\n\n");
        }

    } else if (eq (stype, "CORBAUnion")) {
        umlattrnode *sw = umla;
        char swname[SMALL_BUFFER];
        if (sw == NULL) {
            fprintf (stderr, "Error: attributes not set at union %s\n", name);
            exit (1);
        }
        if (strlen (sw->key.name) == 0)
            sprintf (swname, "Switch");
        else
            sprintf (swname, "%s", sw->key.name);
        print ("type %s (%s : %s := %s'First) is record\n",
               name, swname, sw->key.type, sw->key.type);
        indentlevel++;
        print ("case %s is\n", swname);
        indentlevel++;
        umla = umla->next;
        while (umla != NULL) {
            check_umlattr (&umla->key, name);
            print ("when %s =>\n", umla->key.value);
            print ("   %s : %s;\n", umla->key.name, adaname (umla->key.type));
            umla = umla->next;
        }
        indentlevel--;
        print ("end case;\n");
        indentlevel--;
        print ("end record;\n");
        convention_c (name);
        emit ("\n");

    } else if (is_typedef_stereo (stype)) {
        char dim[SMALL_BUFFER];

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
        if (strlen (umla->key.name) > 0)  {
            fprintf (stderr, "Warning: typedef %s: ignoring name field "
                        "in implementation type attribute\n", name);
        }
        if (*umla->key.value) {
            strcpy (dim, umla->key.value);
            subst (dim, '[', '(');
            subst (dim, ']', ')');
        } else {
            dim[0] = '\0';
        }
        print ("");
        if (!*dim && !strict_corba)
            emit ("sub");
        emit ("type %s is ", name);
        if (*dim) {
            emit ("array (");
            if (strict_corba)
                emit ("0 .. ");
            else
                emit ("1 .. ");
            emit ("%s", dim);  /* multi-dimension support is TBD */
            emit (" of ");
        } else if (strict_corba) {
            emit ("new ");
        }
        emit ("%s;\n\n", adaname (umla->key.type));

    } else if (eq (stype, "CORBAValue")) {
        gen_class (node, 1);

    } else {
        print ("--  %s\n", stype);
        gen_class (node, 0);
    }
}

static char *
make_filename (char *name, int do_body)
{
    static char outfname[BIG_BUFFER];
    char *filebase = strtolower (name);

    subst (filebase, '.', '-');
    if (do_body)
        sprintf (outfname, "%s.%s", filebase, body_file_ext);
    else
        sprintf (outfname, "%s.%s", filebase, file_ext);
    return outfname;
}


void
generate_code_ada (batch *b)
{
    declaration *d;
    umlclasslist tmplist = b->classlist;
    FILE *licensefile = NULL;

    gb = b;

    if (file_ext == NULL)
        file_ext = SPEC_EXT;
    if (body_file_ext == NULL)
        body_file_ext = BODY_EXT;

    /* open license file */
    if (b->license != NULL) {
        licensefile = fopen (b->license, "r");
        if (!licensefile) {
            fprintf (stderr, "Can't open the license file.\n");
            exit (1);
        }
    }

    while (tmplist != NULL) {
        if (! (is_present (b->classes, tmplist->key->name) ^ b->mask)) {
            push (tmplist, b);
        }
        tmplist = tmplist->next;
    }

    set_number_of_spaces_for_one_indentation (3);

    /* Generate a file for each outer declaration.  */
    d = decls;
    while (d != NULL) {
        char *name, basename[BIG_BUFFER];
        int synthesize_package = 0;
        int need_body = 0;

        if (d->decl_kind == dk_module) {
            name = d->u.this_module->pkg->name;
            strcpy (basename, name);
            need_body = has_oo_class (d->u.this_module->contents);
        } else {         /* dk_class */
            name = d->u.this_class->key->name;
            strcpy (basename, name);
            if (is_oo_class (d->u.this_class->key)) {
                need_body = 1;
            } else {
                strcat (basename, PACKAGE_EXT);
                synthesize_package = 1;
            }
        }

        spec = open_outfile (make_filename (basename, 0), b);
        if (spec == NULL) {
            d = d->next;
            continue;
        }

        /* add license to the header */
        if (b->license != NULL) {
            char lc;
            rewind (licensefile);
            while ((lc = fgetc (licensefile)) != EOF)
                print ("%c", lc);
        }

        includes = NULL;
        determine_includes (d, b);
        if (use_corba)
            print ("with CORBA.Value;\n\n");
        if (includes) {
            namelist incfile = includes;
            while (incfile != NULL) {
                if (!eq (incfile->name, name)) {
                    print ("with %s;\n", incfile->name);
                }
                incfile = incfile->next;
            }
            print ("\n");
        }

        if (synthesize_package) {
            emit ("package %s is\n\n", basename);
            indentlevel++;
        } else if (need_body) {
            body = open_outfile (make_filename (basename, 1), b);
        } else {
            body = NULL;
        }

        gen_decl (d);

        if (synthesize_package) {
            indentlevel--;
            emit ("end %s;\n\n", basename);
        } else if (body != NULL) {
            fclose (body);
            body = NULL;
        }

        fclose (spec);

        d = d->next;
    }
}

