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

void generate_code_csharp(batch *b) {
    umlclasslist tmplist, parents;
    umlassoclist associations;
    umlattrlist umla, tmpa;
    umlpackagelist tmppcklist;
    umloplist umlo;
    char *tmpname;
    char outfilename[90];
    FILE *licensefile = NULL;
    umlclasslist used_classes;

    tmplist = b->classlist;

    /* open license file */
    if ( b->license != NULL ) {
        licensefile = fopen(b->license, "r"); 
        if(!licensefile) {
            fprintf(stderr, "Can't open the license file.\n");
            exit(2);
        }
    }

    while ( tmplist != NULL ) {

        if (is_present (b->classes, tmplist->key->name) ^ b->mask) {
            tmplist = tmplist->next;
            continue;
        }

        sprintf(outfilename, "%s.cs", tmplist->key->name);

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
        emit ("%s","using System;\n");

        tmppcklist = make_package_list(tmplist->key->package);
        if ( tmppcklist != NULL ){
            emit ("package %s",tmppcklist->key->name);
            tmppcklist=tmppcklist->next;
            while (tmppcklist != NULL) {
                emit (".%s",tmppcklist->key->name);
                tmppcklist=tmppcklist->next;
            }
            emit (";\n\n");
        }

        /* We generate the import clauses */
        used_classes = list_classes(tmplist, b);
        while (used_classes != NULL) {
            tmppcklist = make_package_list(used_classes->key->package);
            if (tmppcklist != NULL) {
                if (strcmp(tmppcklist->key->id,tmplist->key->package->id)) {
                    /* This class' package and our current class' package are
                       not the same */
                    emit ("using ");
                    emit ("%s",tmppcklist->key->name);
                    tmppcklist=tmppcklist->next;
                    while (tmppcklist != NULL) {
                        emit (".%s", tmppcklist->key->name);
                        tmppcklist=tmppcklist->next;
                    }
                    emit (".");
                    emit ("%s;\n",used_classes->key->name);
                }
            } else {
                /* No info for this class' package, we include it directly */
                /*emit ("import %s;\n",used_classes->key->name);*/
            }
            used_classes = used_classes->next;
        }

        emit ("\n");
        emit ("public ");

        tmpname = strtolower(tmplist->key->stereotype);
        if ( ! strcmp("interface", tmpname) ) {
            emit ("interface ");
        } else {
            if (tmplist->key->isabstract)
            {
                emit ("interface ");
            }
            else
                emit ("class ");
        }
        free(tmpname);

        emit ("%s", tmplist->key->name);

        parents = tmplist->parents;
        if (parents != NULL) {
            while (parents != NULL) {
                tmpname = strtolower(parents->key->stereotype);
                if (! strcmp (tmpname, "interface")) {
                    emit (" : ");
                } else {
                    emit (" : ");
                }
                free(tmpname);
                emit ("%s", parents->key->name);
                parents = parents->next;
            }
        }
        emit (" {\n");

        umla = tmplist->key->attributes;

        if( umla != NULL)
            emit ("\n\t// Attributes\n");

        while ( umla != NULL) {
            switch (umla->key.visibility) {
            case '0':
                emit ("\tpublic ");
                break;
            case '1':
                emit ("\tprivate ");
                break;
            case '2':
                emit ("\tprotected ");
                break;
            }

            if (umla->key.isstatic) {
                emit ("static ");
            }
            emit ("%s %s", umla->key.type, umla->key.name);
            if (umla->key.value[0] != 0) {
                emit (" = %s", umla->key.value);
            }
            emit (";\n");
            umla = umla->next;
        }

        associations = tmplist->associations;
        if (associations != NULL)
            emit ("\n\t// Associations \n");

        while (associations != NULL)
        {
            /* Not sure how to do this actually...*/
            if( associations->composite )
                emit ("\tprotected %s %s;\n", associations->key->name, associations->name);
            else
                emit ("\tprivate %s %s;\n", associations->key->name, associations->name);
            associations = associations->next;
        }

        umlo = tmplist->key->operations;
        while (umlo != NULL) {
            emit ("\n");
            emit ("\t// Operation\n");
            tmpa = umlo->key.parameters;
            while (tmpa != NULL) {
                emit ("\t// param %s\n", tmpa->key.name);
                tmpa = tmpa->next;
            }
            if (strcmp (umlo->key.attr.type, "void"))
                emit ("\t// return %s\n", umlo->key.attr.type);


            emit ("\t");

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
            emit ("%s(", umlo->key.attr.name);
            tmpa = umlo->key.parameters;
            while (tmpa != NULL) {
                emit ("%s %s", tmpa->key.type, tmpa->key.name);
                /*
                if ( tmpa->key.value[0] != 0 ){
                    emit (" = %s",tmpa->key.value);
                }  */
                tmpa = tmpa->next;
                if (tmpa != NULL) emit (", ");
            }
            emit (")");
            if (umlo->key.attr.isabstract ) {
                emit (";\n");
            } else {
                emit ("\n\t{\n");
                if (umlo->key.implementation != NULL) {
                    emit ("\n\t\t%s\n", umlo->key.implementation);
                }
                else if (strcmp(umlo->key.attr.type, "void") != 0) {
                    emit ("\t\t%s\n", "throw new System.Exception( \"Not implemented yet!\");");
                }
                emit ("\t}\n");
            }
            umlo = umlo->next;
        }
        emit ("}\n\n");

        fclose(spec);
        tmplist = tmplist->next;
    }
}
