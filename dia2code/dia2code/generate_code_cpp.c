/***************************************************************************
         generate_code_cpp.c  -  Function that generates C++ code
                             -------------------
    begin                : Sat Dec 16 2000
    copyright            : (C) 2000-2001 by Javier O'Hara
    email                : joh314@users.sourceforge.net
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

void generate_code_cpp(batch *b) {
    umlclasslist tmplist, parents;
    umltemplatelist templates;
    umlassoclist associations;
    namelist used_classes, tmpnamelist;
    int tmpv;
    umlattrlist umla, tmpa;
    umloplist umlo;
    char *tmpname;
    char outfilename[256];
    FILE * outfileh, *outfilecpp, *dummyfile, *licensefile = NULL;

    int tmpdirlgth, tmpfilelgth;

    if (b->outdir == NULL) {
        b->outdir = ".";
    }

    tmpdirlgth = strlen(b->outdir);

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

        if ( ! ( is_present(b->classes, tmplist->key->name) ^ b->mask ) ) {

            tmpname = strtolower(tmplist->key->name);

            /* This prevents buffer overflows */
            tmpfilelgth = strlen(tmpname);
            if (tmpfilelgth + tmpdirlgth > sizeof(*outfilename) - 2) {
                fprintf(stderr, "Sorry, name of file too long ...\nTry a smaller dir name\n");
                exit(4);
            }

            sprintf(outfilename, "%s/%s.h", b->outdir, tmpname);
            dummyfile = fopen(outfilename, "r");
            if ( b->clobber || ! dummyfile ) {

                outfileh = fopen(outfilename, "w");
                if ( outfileh == NULL ) {
                    fprintf(stderr, "Can't open file %s for writing\n", outfilename);
                    exit(3);
                }

                /* This prevents buffer overflows */
                tmpfilelgth = strlen(tmpname);
                if (tmpfilelgth + tmpdirlgth > sizeof(*outfilename) - 2) {
                    fprintf(stderr, "Sorry, name of file too long ...\nTry a smaller dir name\n");
                    exit(4);
                }

                sprintf(outfilename, "%s/%s.cpp", b->outdir, tmpname);
                outfilecpp = fopen(outfilename, "w");
                if ( outfilecpp == NULL ) {
                    fprintf(stderr, "Can't open file %s for writing\n", outfilename);
                    exit(3);
                }

                /* add license to the header */
                if(b->license != NULL){
                    char lc;
                    rewind(licensefile);
                    while((lc = fgetc(licensefile)) != EOF){
                        fprintf(outfileh,"%c",lc);
                        fprintf(outfilecpp,"%c",lc);
                    }
                }


                fprintf(outfilecpp, "#include \"%s.h\"\n\n", tmpname);

                free(tmpname);

                tmpname = strtoupper(tmplist->key->name);
                fprintf(outfileh, "#ifndef %s_H\n", tmpname);
                fprintf(outfileh, "#define %s_H\n\n", tmpname);
                free(tmpname);

                used_classes = find_classes(tmplist, b);
                tmpnamelist = used_classes;
                while (tmpnamelist != NULL) {
                    tmpname = strtolower(tmpnamelist->name);
                    fprintf(outfileh, "#include \"%s.h\"\n", tmpname);
                    tmpnamelist = tmpnamelist->next;
                    free(tmpname);
                }

                fprintf(outfileh, "\n");

                templates = tmplist->key->templates;
                if (templates != NULL) {
                    fprintf(outfileh, "template <");
                }
                while ( templates != NULL ) {
                    if (templates->next != NULL) {
                        fprintf(outfileh, "%s %s,", templates->key.type, templates->key.name);
                    } else {
                        fprintf(outfileh, "%s %s>\n", templates->key.type, templates->key.name);
                    }
                    templates = templates->next;
                }

                if ( strlen(tmplist->key->stereotype) > 0 ) {
                    fprintf(outfileh, "// %s\n", tmplist->key->stereotype);
                }
                fprintf(outfileh, "class %s", tmplist->key->name);

                parents = tmplist->parents;
                if (parents != NULL) {
                    fprintf(outfileh, ": ");
                    while ( parents != NULL ) {
                        fprintf(outfileh, "public %s", parents->key->name);
                        parents = parents->next;
                        if ( parents != NULL ) {
                            fprintf(outfileh, ", ");
                        }
                    }
                }
                fprintf(outfileh, " {\n");

                fprintf(outfileh, "  // Associations\n");
                associations = tmplist->associations;
                while (associations != NULL) {
                    fprintf(outfileh, "   %s ", associations->key->name);
                    if (associations->composite == 0) {
                        fprintf(outfileh, "* ");
                    }
                    fprintf(outfileh, "%s;\n", associations->name);
                    associations = associations->next;
                }

                fprintf(outfileh, "  // Attributes\n");
                tmpv = -1;
                umla = tmplist->key->attributes;
                while ( umla != NULL) {
                    fprintf(outfileh, "  ");
                    if ( tmpv != umla->key.visibility ) {
                        switch (umla->key.visibility) {
                        case '0':
                            fprintf (outfileh, "public:\n    ");
                            break;
                        case '1':
                            fprintf (outfileh, "private:\n    ");
                            break;
                        case '2':
                            fprintf (outfileh, "protected:\n    ");
                            break;
                        }
                        tmpv = umla->key.visibility;
                    } else {
                        fprintf (outfileh, "  ");
                    }

                    if (umla->key.isstatic) {
                        fprintf(outfileh, "static ");
                    }
                    fprintf(outfileh, "%s %s", umla->key.type, umla->key.name);
                    if ( umla->key.value[0] != 0 && umla->key.isstatic) {
                        fprintf(outfilecpp, "%s %s::%s", umla->key.type, tmplist->key->name, umla->key.name);
                        fprintf(outfilecpp, " = %s", umla->key.value);
                        fprintf(outfilecpp, ";\n");
                    }

                    fprintf(outfileh, ";\n");

                    umla = umla->next;
                }

                umlo = tmplist->key->operations;
                fprintf(outfileh, "  // Operations\n");
                tmpv = -1;
                while ( umlo != NULL) {
                    fprintf(outfileh, "  ");
                    if ( tmpv != umlo->key.attr.visibility ) {
                        switch (umlo->key.attr.visibility) {
                        case '0':
                            fprintf(outfileh, "public:\n    ");
                            break;
                        case '1':
                            fprintf(outfileh, "private:\n    ");
                            break;
                        case '2':
                            fprintf(outfileh, "protected:\n    ");
                            break;
                        }
                        tmpv = umlo->key.attr.visibility;
                    } else {
                        fprintf (outfileh, "  ");
                    }

                    if ( umlo->key.attr.isabstract ) {
                        fprintf(outfileh, "virtual ");
                        umlo->key.attr.value[0] = '0';
                    }
                    if ( umlo->key.attr.isstatic ) {
                        fprintf(outfileh, "static ");
                    }
                    if (strlen(umlo->key.attr.type) > 0) {
                        fprintf(outfileh, "%s ", umlo->key.attr.type);
                    }
                    fprintf(outfileh, "%s ( ", umlo->key.attr.name);
                    if ( ! umlo->key.attr.isabstract ) {
                        if (strlen(umlo->key.attr.type) > 0) {
                            fprintf(outfilecpp, "%s ", umlo->key.attr.type);
                        }
                        fprintf(outfilecpp, "%s::%s ( ", tmplist->key->name, umlo->key.attr.name);
                    }
                    tmpa = umlo->key.parameters;
                    while (tmpa != NULL) {
                        fprintf(outfileh, "%s %s", tmpa->key.type, tmpa->key.name);
                        if ( ! umlo->key.attr.isabstract ) {
                            fprintf(outfilecpp, "%s %s", tmpa->key.type, tmpa->key.name);
                        }
                        if ( tmpa->key.value[0] != 0 ) {
                            fprintf(outfileh, " = %s", tmpa->key.value);
                            if ( ! umlo->key.attr.isabstract ) {
                                fprintf(outfilecpp, " = %s", tmpa->key.value);
                            }
                        }
                        tmpa = tmpa->next;
                        if (tmpa != NULL) {
                            fprintf(outfileh, ", ");
                            if ( ! umlo->key.attr.isabstract ) {
                                fprintf(outfilecpp, ", ");
                            }
                        }
                    }
                    fprintf(outfileh, " )");
                    if ( ! umlo->key.attr.isabstract ) {
                        fprintf(outfilecpp, " )");
                    }
                    if ( umlo->key.attr.value[0] != 0 ) {
                        fprintf(outfileh, " = %s", umlo->key.attr.value);
                    }
                    fprintf(outfileh, ";\n");
                    if ( ! umlo->key.attr.isabstract ) {
                        fprintf(outfilecpp, "{\n}\n\n");
                    }
                    umlo = umlo->next;
                }
                fprintf(outfileh, "};\n\n");

                fprintf(outfileh, "#endif\n");

                fclose(outfileh);
                fclose(outfilecpp);
            }

        }
        tmplist = tmplist->next;
    }
}
