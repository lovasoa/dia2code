/***************************************************************************
         generate_code_cpp.c  -  Function that generates C++ code
                             -------------------
    begin                : Sat Dec 16 2000
    copyright            : (C) 2000-2001 by Javier O'Hara
                           (C) 2002 by Oliver Kellogg
    email                : joh314@users.sourceforge.net
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

/* NB: If you use CORBA stereotypes, you will need the file p_orb.h
   found in the runtime/cpp directory.  */

#include "dia2code.h"
#include "decls.h"
#include "includes.h"

#define SPEC_EXT "h"
#define BODY_EXT "cpp"

#define eq  !strcmp

static batch *gb;   /* The current batch being processed.  */

/* Utilities.  */

static void
check_umlattr (umlattribute *u, char *typename)
{
    /* Check settings that don't make sense for C++ generation.  */
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
pass_by_reference (umlclass *cl)
{
    char *st;
    if (cl == NULL)
        return 0;
    st = cl->stereotype;
    if (strlen (st) == 0)
        return 1;
    if (eq (st, "CORBATypedef")) {
        umlattrlist umla = cl->attributes;
        umlclassnode *ref = find_by_name (gb->classlist, cl->name);
        if (ref == NULL)
            return 0;
        return pass_by_reference (ref->key);
    }
    return (!eq (st, "CORBAConstant") &&
            !is_enum_stereo (st));
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
    return (!eq (st, "CORBAConstant") &&
            !eq (st, "CORBATypedef") &&
            !is_enum_stereo (st) &&
            !eq (st, "CORBAStruct") &&
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
cppname (char *name)
{
    static char buf[80];
    if (use_corba) {
        if (eq (name, "boolean") ||
            eq (name, "char") ||
            eq (name, "octet") ||
            eq (name, "short") ||
            eq (name, "long") ||
            eq (name, "float") ||
            eq (name, "double") ||
            eq (name, "string") ||
            eq (name, "any")) {
            sprintf (buf, "CORBA::%s", strtoupperfirst (name));
        } else if (eq (name, "long long")) {
            sprintf (buf, "CORBA::LongLong");
        } else if (eq (name, "unsigned short")) {
            sprintf (buf, "CORBA::UShort");
        } else if (eq (name, "unsigned long")) {
            sprintf (buf, "CORBA::ULong");
        } else if (eq (name, "unsigned long long")) {
            sprintf (buf, "CORBA::ULongLong");
        } else {
            strcpy (buf, name);
        }
    } else {
        strcpy (buf, name);
    }
    return buf;
}


static char *
fqname (umlclassnode *node, int use_ref_type)
{
    static char buf[256];

    buf[0] = '\0';
    if (node == NULL)
        return buf;
    if (node->key->package != NULL) {
        umlpackagelist pkglist = make_package_list (node->key->package);
        while (pkglist != NULL) {
            strcat (buf, pkglist->key->name);
            strcat (buf, "::");
            pkglist = pkglist->next;
        }
    }
    strcat (buf, node->key->name);
    if (use_ref_type)
        strcat (buf, "*");
    return buf;
}

static void
check_visibility (int *curr_vis, int new_vis)
{
    if (*curr_vis == new_vis)
        return;
    indentlevel--;
    switch (new_vis) {
      case '0':
        print ("public:\n");
        break;
      case '1':
        print ("private:\n");
        break;
      case '2':
        print ("protected:\n");
        break;
    }
    *curr_vis = new_vis;
    indentlevel++;
}

static void
gen_class (umlclassnode *node)
{
    char *name = node->key->name;
    char *stype = node->key->stereotype;
    int is_valuetype = 0;

    if (strlen (stype) > 0) {
        print ("// %s\n", stype);
        is_valuetype = eq (stype, "CORBAValue");
    }

    print("/// class %s - %s\n", name, node->key->comment);

    if (node->key->templates != NULL) {
        umltemplatelist template = node->key->templates;
        if (is_valuetype) {
            fprintf (stderr, "CORBAValue %s: template ignored\n", name);
        } else {
            print ("template <");
            while (template != NULL) {
                print ("%s %s", template->key.type, template->key.name);
                template = template->next;
                if (template != NULL)
                    emit (", ");
            }
            emit (">\n");
        }
    }

    print ("class %s", name);
    if (node->parents != NULL) {
        umlclasslist parent = node->parents;
        emit (" : ");
        while (parent != NULL) {
            emit ("public %s", parent->key->name);
            parent = parent->next;
            if (parent != NULL)
                emit (", ");
        }
    } else if (is_valuetype) {
        emit (" : CORBA::ValueBase");
    }
    emit (" {\n");
    indentlevel++;

    if (node->associations != NULL) {
        umlassoclist assoc = node->associations;
        print ("// Associations\n");
        /*
         * The associations are mapped as private members.
         * Is that really what we want?
         * (For example, other UML tools additionally generate
         * setters/getters.)  Ideas and comments welcome.
        */
        while (assoc != NULL) {
            umlclassnode *ref;
            ref = find_by_name (gb->classlist, assoc->key->name);
            print ("");
            if (ref != NULL)
                emit ("%s", fqname (ref, !assoc->composite));
            else
                emit ("%s", cppname (assoc->key->name));
            emit (" %s;\n", assoc->name);
            assoc = assoc->next;
        }
    }

    if (node->key->attributes != NULL) {
        umlattrlist umla = node->key->attributes;
        if (is_valuetype) {
            print ("// Public state members\n");
            indentlevel--;
            print ("public:\n");
            indentlevel++;
            while (umla != NULL) {
                char *member = umla->key.name;
                umlclassnode *ref;
                if (umla->key.visibility != '0') {
                    umla = umla->next;
                    continue;
                }
                print ("");
                if (umla->key.isstatic) {
                    fprintf (stderr, "CORBAValue %s/%s: static not supported\n",
                                     name, member);
                }
                ref = find_by_name (gb->classlist, umla->key.type);
                if (ref != NULL)
                    eboth ("%s", fqname (ref, 1));
                else
                    eboth ("%s", cppname (umla->key.type));
                emit (" %s () { return _%s; }\n", member, member);
                print ("void %s (", member);
                if (ref != NULL) {
                    int by_ref = pass_by_reference (ref->key);
                    if (by_ref)
                        emit ("const ");
                    emit ("%s", fqname (ref, 1));
                    if (by_ref)
                        emit ("&");
                } else {
                    emit ("%s", cppname (umla->key.type));
                }
                emit (" value_) { _%s = value_; }\n");
                umla = umla->next;
            }
        } else {
            int tmpv = -1;
            print ("// Attributes\n");
            while (umla != NULL) {
                check_visibility (&tmpv, umla->key.visibility);
                if (strlen(umla->key.comment)) {
                    print("/// %s\n", umla->key.comment);
                }
                print ("");
                if (umla->key.isstatic) {
                    emit ("static ");
                }
                emit ("%s %s", umla->key.type, umla->key.name);
                emit (";\n");
                umla = umla->next;
            }
        }
    }

    if (node->key->operations != NULL) {
        umloplist umlo = node->key->operations;
        int tmpv = -1;
        print ("// Operations\n");
        if (is_valuetype) {
            indentlevel--;
            print ("public:\n");
            indentlevel++;
        }
        while (umlo != NULL) {
            umlattrlist tmpa = umlo->key.parameters;
            if (is_valuetype) {
                if (umlo->key.attr.visibility != '0')
                    fprintf (stderr, "CORBAValue %s/%s: must be public\n",
                                     name, umlo->key.attr.name);
            } else {
                check_visibility (&tmpv, umlo->key.attr.visibility);
            }

            /* print comments on operation */
if (strlen(umlo->key.attr.comment)) {
    print("/// %s\n", umlo->key.attr.comment);
    tmpa = umlo->key.parameters;
    while (tmpa != NULL) {
         print("/// @param %s\t\t(%s) %s\n",
               tmpa->key.name,
               kind_str(tmpa->key.kind),
               tmpa->key.comment);
               tmpa = tmpa->next;
    }
}
            /* print operation */
            print ("");
            if (umlo->key.attr.isabstract || is_valuetype) {
                emit ("virtual ");
                umlo->key.attr.value[0] = '0';
            }
            if (umlo->key.attr.isstatic) {
                if (is_valuetype)
                    fprintf (stderr, "CORBAValue %s/%s: static not supported\n",
                                     name, umlo->key.attr.name);
                else
                    emit ("static ");
            }
            if (strlen (umlo->key.attr.type) > 0) {
                emit ("%s ", cppname (umlo->key.attr.type));
            }
            emit ("%s (", umlo->key.attr.name);
            while (tmpa != NULL) {
                emit ("%s %s", tmpa->key.type, tmpa->key.name);
                if (tmpa->key.value[0] != 0) {
                    if (is_valuetype)
                        fprintf (stderr, "CORBAValue %s/%s: param default "
                                 "not supported\n", name, umlo->key.attr.name);
                    else
                       emit (" = %s", tmpa->key.value);
                }
                tmpa = tmpa->next;
                if (tmpa != NULL) {
                    emit (", ");
                }
            }
            emit (")");
            if (umlo->key.attr.value[0]) {
                emit (" = %s", umlo->key.attr.value);
            }
            emit (";\n");
            umlo = umlo->next;
        }
    }

    if (node->key->attributes != NULL && is_valuetype) {
        umlattrlist umla = node->key->attributes;
        emit ("\n");
        indentlevel--;
        print ("private:  // State member implementation\n");
        indentlevel++;
        while (umla != NULL) {
            umlclassnode *ref = find_by_name (gb->classlist, umla->key.type);
            print ("");
            if (ref != NULL) {
                emit ("%s", fqname (ref, is_oo_class (ref->key)));
                /*
                 * FIXME: Find a better way to decide whether to use
                 * a pointer.
                */
            } else
                emit ("%s", cppname (umla->key.type));
            emit (" _%s;\n", umla->key.name);
            umla = umla->next;
        }
    }

    indentlevel--;
    print ("};\n\n");
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
        print ("namespace %s {\n\n", name);
        indentlevel++;
        d = d->u.this_module->contents;
        while (d != NULL) {
            gen_decl (d);
            d = d->next;
        }
        indentlevel--;
        print ("};\n\n", name);
        return;
    }

    node = d->u.this_class;
    stype = node->key->stereotype;
    name = node->key->name;
    umla = node->key->attributes;

    if (strlen (stype) == 0) {
        gen_class (node);
        return;
    }

    if (eq (stype, "CORBANative")) {
        print ("// CORBANative: %s \n\n", name);

    } else if (eq (stype, "CORBAConstant")) {
        if (umla == NULL) {
            fprintf (stderr, "Error: first attribute not set at %s\n", name);
            exit (1);
        }
        if (strlen (umla->key.name) > 0)
            fprintf (stderr, "Warning: ignoring attribute name at %s\n", name);

        print ("const %s %s = %s;\n\n", cppname (umla->key.type), name,
                                                 umla->key.value);

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
        indentlevel--;
        print ("};\n\n");

    } else if (eq (stype, "CORBAStruct")) {
        print ("struct %s {\n", name);
        indentlevel++;
        while (umla != NULL) {
            check_umlattr (&umla->key, name);
            print ("%s %s", cppname (umla->key.type), umla->key.name);
            if (strlen (umla->key.value) > 0)
                fprintf (stderr, "%s/%s: ignoring value\n",
                                 name, umla->key.name);
            emit (";\n");
            umla = umla->next;
        }
        indentlevel--;
        print ("};\n\n");

    } else if (eq (stype, "CORBAException")) {
        fprintf (stderr, "%s: CORBAException not yet implemented\n", name);

    } else if (eq (stype, "CORBAUnion")) {
        umlattrnode *sw = umla;
        if (sw == NULL) {
            fprintf (stderr, "Error: attributes not set at union %s\n", name);
            exit (1);
        }
        fprintf (stderr, "%s: CORBAUnion not yet fully implemented\n", name);
        print ("class %s {  // CORBAUnion\n", name);
        print ("public:\n", name);
        indentlevel++;
        print ("%s _d();  // body TBD\n\n", umla->key.type);
        umla = umla->next;
        while (umla != NULL) {
            check_umlattr (&umla->key, name);
            print ("%s %s ();  // body TBD\n",
                   cppname (umla->key.type), umla->key.name);
            print ("void %s (%s _value);  // body TBD\n\n", umla->key.name,
                   cppname (umla->key.type));
            umla = umla->next;
        }
        indentlevel--;
        print ("};\n\n");

    } else if (eq (stype, "CORBATypedef")) {
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
        print ("typedef %s %s%s;\n\n", cppname (umla->key.type), name,
                                                umla->key.value);
    } else {
        gen_class (node);
    }
}


void
generate_code_cpp (batch *b)
{
    declaration *d;
    umlclasslist tmplist = b->classlist;
    FILE *licensefile = NULL;

    gb = b;

    if (file_ext == NULL)
        file_ext = "h";
    /*
    if (body_file_ext == NULL)
        body_file_ext = "cpp";
     */

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

    /* Generate a file for each outer declaration.  */
    d = decls;
    while (d != NULL) {
        char *name;
        char filename[256];

        if (d->decl_kind == dk_module) {
            name = d->u.this_module->pkg->name;
        } else {         /* dk_class */
            name = d->u.this_class->key->name;
        }
        sprintf (filename, "%s.%s", name, file_ext);

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
                print ("%c", lc);
        }

        includes = NULL;
        determine_includes (d, b);
        if (use_corba)
            print ("#include <p_orb.h>\n\n");
        if (includes) {
            namelist incfile = includes;
            while (incfile != NULL) {
                if (!eq (incfile->name, name)) {
                    print ("#include \"%s.%s\"\n", incfile->name, file_ext);
                }
                incfile = incfile->next;
            }
            print ("\n");
        }

        gen_decl (d);

        fclose (spec);

        d = d->next;
    }
}

