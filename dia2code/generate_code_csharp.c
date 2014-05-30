/***************************************************************************
          generate_code_csharp.c  -  Function that generates C# code
                             -------------------
    begin                : Wed Dec 04 2002
    copyright            : (C) 2000-2002 by Javier O'Hara,
    email                : joh314@users.sourceforge.net

    Authors              : Javier O'Hara <joh314@users.sourceforge.net>
                           Thomas Hansen <thomas.hansen@adramatch.com>
                           Oliver Kellogg <okellogg@users.sourceforge.net>
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

static char *
pkgname (umlpackagelist pkg)
{
    static char buf[BIG_BUFFER];

    buf[0] = '\0';
    while (pkg != NULL) {
        strcat (buf, pkg->key->name);
        pkg = pkg->next;
        if (pkg != NULL)
            strcat (buf, ".");
    }
    return buf;
}

void generate_code_csharp (batch *b) {
    umlclasslist tmplist = b->classlist;
    FILE *licensefile = NULL;

    /* open license file */
    if (b->license != NULL) {
        licensefile = fopen (b->license, "r"); 
        if (!licensefile) {
            fprintf (stderr, "Can't open the license file.\n");
            exit (1);
        }
    }

    while (tmplist != NULL) {
        char *tmpname;
        char outfilename[SMALL_BUFFER];
        umlclasslist used_classes;

        if (is_present (b->classes, tmplist->key->name) ^ b->mask) {
            tmplist = tmplist->next;
            continue;
        }

        sprintf (outfilename, "%s.cs", tmplist->key->name);

        spec = open_outfile (outfilename, b);
        if (spec == NULL) {
            tmplist = tmplist->next;
            continue;
        }

        /* add license to the header */
        if (b->license != NULL) {
            char lc;
            rewind (licensefile);
            while ((lc = fgetc (licensefile)) != EOF)
                print ("%c", lc);
        }
        emit ("%s", "using System;\n\n");

        /* We generate the import clauses */
        used_classes = list_classes (tmplist, b);
        if (used_classes != NULL) {
            while (used_classes != NULL) {
                umlpackagelist pkg = make_package_list (used_classes->key->package);
                if (pkg != NULL) {
                    if (strcmp (pkg->key->id, tmplist->key->package->id)) {
                        /* This class' package and our current class' package are
                           not the same */
                        emit ("using %s.%s;\n", pkgname (pkg), used_classes->key->name);
                    }
                } else {
                    /* No info for this class' package, we include it directly */
                    /*emit ("import %s;\n",used_classes->key->name);*/
                }
                used_classes = used_classes->next;
            }
            emit ("\n");
        }

        if (tmplist->key->package != NULL) {
            umlpackagelist pkg = make_package_list (tmplist->key->package);
            emit ("namespace %s {\n\n", pkgname (pkg));
            indentlevel++;
        }

        print ("public ");

        tmpname = strtolower (tmplist->key->stereotype);
        if (eq ("interface", tmpname)) {
            emit ("interface ");
        } else {
            if (tmplist->key->isabstract)
            {
                emit ("interface ");
            }
            else
                emit ("class ");
        }
        free (tmpname);

        emit ("%s", tmplist->key->name);

        if (tmplist->parents != NULL) {
            umlclasslist parents = tmplist->parents;
            while (parents != NULL) {
                tmpname = strtolower (parents->key->stereotype);
                if (eq (tmpname, "interface")) {
                    emit (" : ");
                } else {
                    emit (" : ");
                }
                free (tmpname);
                emit ("%s", parents->key->name);
                parents = parents->next;
            }
        }
        emit (" {\n\n");
        indentlevel++;

        if (tmplist->key->attributes != NULL) {
            umlattrlist umla = tmplist->key->attributes;

            print ("// Attributes and properties\n");

            while (umla != NULL) {
                switch (umla->key.visibility) {
                case '0':
                    print ("public ");
                    break;
                case '1':
                    print ("private ");
                    break;
                case '2':
                    print ("protected ");
                    break;
                }

                if (umla->key.isstatic) {
                    emit ("static ");
                }
                emit ("%s %s", umla->key.type, umla->key.name);
                if (umla->key.value[0] != 0) {
                    emit (" = %s", umla->key.value);
                }
                int wp = 0, rp = 0;
                if (eq (umla->key.comment, "rproperty"))
                    rp = 1;
                else if (eq (umla->key.comment, "wproperty"))
                    wp = 1;
                else if (eq (umla->key.comment, "rwproperty"))
                    rp = wp = 1;
                if (wp || rp) {
                    emit (" {\n");
                    indentlevel++;
                    if (rp)
                        print ("get;\n");
                    if (wp)
                        print ("set;\n");
                    indentlevel--;
                    print ("}\n");
                }
                else
                    emit (";\n");
                umla = umla->next;
            }
            emit ("\n");
        }

        if (tmplist->associations != NULL) {
            umlassoclist assoc = tmplist->associations;

            print ("// Associations \n");
            while (assoc != NULL)
            {
                /* Not sure how to do this actually...*/
                if (assoc->composite )
                    print ("protected ");
                else
                    print ("private ");
                emit ("%s %s;\n", assoc->key->name, assoc->name);
                assoc = assoc->next;
            }
            emit ("\n");
        }

        if (tmplist->key->operations != NULL) {
            umloplist umlo = tmplist->key->operations;
            while (umlo != NULL) {
                umlattrlist tmpa = umlo->key.parameters;

                print ("// Operation\n");
                while (tmpa != NULL) {
                    print ("// param %s\n", tmpa->key.name);
                    tmpa = tmpa->next;
                }
                if (strcmp (umlo->key.attr.type, "void"))
                    print ("// return %s\n", umlo->key.attr.type);


                print ("");

                if (umlo->key.attr.isabstract)
                {
                    /*emit ("public ");*/
                    umlo->key.attr.value[0] = '0';
                }
                else
                {
                    switch (umlo->key.attr.visibility) {
                    case '0':
                        emit ("public ");
                        break;
                    case '1':
                        emit ("private ");
                        break;
                    case '2':
                        emit ("protected ");
                        break;
                    }
                }
                if (umlo->key.attr.isstatic) {
                    emit ("static ");
                }
                if (strlen (umlo->key.attr.type) > 0) {
                    emit ("%s ", umlo->key.attr.type);
                }
                emit ("%s (", umlo->key.attr.name);
                tmpa = umlo->key.parameters;
                while (tmpa != NULL) {
                    emit ("%s %s", tmpa->key.type, tmpa->key.name);
                    /*
                    if ( tmpa->key.value[0] != 0 ){
                        emit (" = %s",tmpa->key.value);
                    }  */
                    tmpa = tmpa->next;
                    if (tmpa != NULL)
                        emit (", ");
                }
                emit (")");
                if (umlo->key.attr.isabstract ) {
                    emit (";\n");
                } else {
                    emit ("\n");
                    print ("{\n");
                    indentlevel++;
                    if (umlo->key.implementation != NULL) {
                        print ("%s\n", umlo->key.implementation);
                    } else if (strcmp (umlo->key.attr.type, "void") != 0) {
                        print ("%s\n", "throw new System.Exception (\"Not implemented yet!\");");
                    }
                    indentlevel--;
                    print ("}\n");
                }
                umlo = umlo->next;
            }
            emit ("\n");
        }
        indentlevel--;
        print ("}\n\n");

        if (tmplist->key->package != NULL) {
            indentlevel--;
            print ("}\n\n");
        }

        fclose (spec);
        tmplist = tmplist->next;
    }
}
