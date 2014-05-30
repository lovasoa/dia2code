/***************************************************************************
         generate_code_c.c  -  Function that generates C code
                             -------------------
    begin                : Tue Jan 2 2001
    copyright            : (C) 2001 by Ruben Lopez
    email                : ryu@gpul.org
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

void generate_code_c(batch *b) {
    umlclasslist tmplist, parents;
    umlassoclist associations;
    int tmpv;
    umlattrlist umla, tmpa;
    umloplist umlo;
    char *tmpname;
    char outfilename[BIG_BUFFER];
    FILE * outfileh, *outfilecpp, *dummyfile, *licensefile = NULL;
    namelist classes_used, tmpclass;

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

                sprintf(outfilename, "%s/%s.c", b->outdir, tmpname);
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
                fprintf(outfileh, "#ifndef __%s_H__\n", tmpname);
                fprintf(outfileh, "#define __%s_H__\n\n", tmpname);
                fprintf(outfileh, "#define %s(OBJ) ((%s*)OBJ)\n\n", tmpname, tmplist->key->name);
                free(tmpname);

                fprintf(outfileh, "#ifndef String\n#define String char*\n#endif\n\n");

                classes_used = find_classes(tmplist, b);
                tmpclass = classes_used;
                while (tmpclass != NULL) {
                    tmpname = strtolower(tmpclass->name);
                    fprintf(outfileh, "#include \"%s.h\"\n", tmpname);
                    tmpclass = tmpclass->next;
                    free(tmpname);
                }

                fprintf(outfileh, "\n");

                if ( strlen(tmplist->key->stereotype) > 0 ) {
                    fprintf(outfileh, "// %s\n", tmplist->key->stereotype);
                }
                fprintf(outfileh, "typedef struct _%s %s;\n\n", tmplist->key->name, tmplist->key->name);
                fprintf(outfileh, "struct _%s", tmplist->key->name);

                fprintf(outfileh, " {\n");
                parents = tmplist->parents;
                if (parents != NULL) {
                    fprintf(outfileh, "%s super;\n", parents->key->name);
                }

                fprintf(outfileh, "  /** Attributes **/\n");
                tmpv = -1;
                umla = tmplist->key->attributes;
                while ( umla != NULL) {
                    if (!umla->key.isstatic) {
                        fprintf(outfileh, "  ");
                        if ( tmpv != umla->key.visibility ) {
                            switch (umla->key.visibility) {
                            case '0':
                                fprintf (outfileh, "/*public*/\n    ");
                                break;
                            case '1':
                                fprintf (outfileh, "/*private*/\n    ");
                                break;
                            case '2':
                                fprintf (outfileh, "/*protected*/\n    ");
                                break;
                            }
                            tmpv = umla->key.visibility;
                        } else {
                            fprintf (outfileh, "  ");
                        }

                        fprintf(outfileh, "%s %s", umla->key.type, umla->key.name);
                        /*if ( umla->key.value[0] != 0 && umla->key.isstatic) {
                            fprintf(outfilecpp,"%s %s::%s",umla->key.type,tmplist->key->name,umla->key.name);
                            fprintf(outfilecpp," = %s",umla->key.value);
                            fprintf(outfilecpp,";\n");
                    }*/

                        fprintf(outfileh, ";\n");

                    } else {
                        fprintf(outfilecpp, "static %s %s", umla->key.type, umla->key.name);
                        fprintf(outfilecpp, " = %s", umla->key.value);
                        fprintf(outfilecpp, ";\n");
                    }
                    umla = umla->next;
                }

                fprintf(outfileh, "  /** Associations **/\n");
                associations = tmplist->associations;
                while ( associations != NULL) {
                    fprintf(outfileh, "   %s ", associations->key->name);
                    if (associations->composite == 0) {
                        fprintf(outfileh, "* ");
                    }
                    fprintf(outfileh, "%s;\n", associations->name);
                    associations = associations->next;
                }

                /***** VIRTUAL METHODS *****/
                /* Virtuald methods should be in the structure */
                umlo = tmplist->key->operations;
                fprintf(outfileh, "/** Operations **/\n");
                tmpv = -1;
                while ( umlo != NULL) {
                    if ( umlo->key.attr.isabstract ) {
                        if ( tmpv != umlo->key.attr.visibility ) {
                            switch (umlo->key.attr.visibility) {
                            case '0':
                                fprintf(outfileh, "/*public*/\n");
                                break;
                            case '1':
                                fprintf(outfileh, "/*private*/\n");
                                break;
                            case '2':
                                fprintf(outfileh, "/*protected*/\n");
                                break;
                            }
                            tmpv = umlo->key.attr.visibility;
                        }

                        if ( umlo->key.attr.isstatic ) {
                            /*static virtual methods are not supported yet */
                        } else {
                            fprintf(outfileh, "%s (*%s) ( %s *this", umlo->key.attr.type, umlo->key.attr.name, tmplist->key->name);
                            if ( ! umlo->key.attr.isabstract ) {
                                fprintf(outfilecpp, "%s %s_%s ( %s *this", umlo->key.attr.type, tmplist->key->name, umlo->key.attr.name, tmplist->key->name);
                            }
                        }
                        tmpa = umlo->key.parameters;
                        if (tmpa != NULL) {
                            fprintf(outfileh, ", ");
                        }
                        while (tmpa != NULL) {
                            fprintf(outfileh, "%s %s", tmpa->key.type, tmpa->key.name);
                            if ( ! umlo->key.attr.isabstract ) {
                                fprintf(outfilecpp, "%s %s", tmpa->key.type, tmpa->key.name);
                            }
                            /*if ( tmpa->key.value[0] != 0 ) {
                             fprintf(outfileh," = %s",tmpa->key.value);
                             if ( ! umlo->key.attr.isabstract ) {
                             fprintf(outfilecpp," = %s",tmpa->key.value);
                             }
                             }*/
                            tmpa = tmpa->next;
                            if (tmpa != NULL) {
                                fprintf(outfileh, ", ");
                            }
                        }
                        fprintf(outfileh, " )");
                        if ( umlo->key.attr.value[0] != 0 ) {
                            fprintf(outfileh, " = %s", umlo->key.attr.value);
                        }
                        fprintf(outfileh, ";\n");
                    }
                    umlo = umlo->next;
                }


                fprintf(outfileh, "};\n\n");

                /***** ALL METHODS ******/
                umlo = tmplist->key->operations;
                fprintf(outfileh, "/** Operations **/\n");
                tmpv = -1;
                while ( umlo != NULL) {
                    if ( tmpv != umlo->key.attr.visibility ) {
                        switch (umlo->key.attr.visibility) {
                        case '0':
                            fprintf(outfileh, "/*public*/\n");
                            break;
                        case '1':
                            fprintf(outfileh, "/*private*/\n");
                            break;
                        case '2':
                            fprintf(outfileh, "/*protected*/\n");
                            break;
                        }
                        tmpv = umlo->key.attr.visibility;
                    }

                    if ( umlo->key.attr.isstatic ) {
                        /*static methods doesn't receive the instance */
                        fprintf(outfileh, "%s %s_%s ( ", umlo->key.attr.type, tmplist->key->name, umlo->key.attr.name);
                        fprintf(outfilecpp, "%s %s_%s ( ", umlo->key.attr.type, tmplist->key->name, umlo->key.attr.name);
                    } else {
                        fprintf(outfileh, "%s %s_%s ( %s *this", umlo->key.attr.type, tmplist->key->name, umlo->key.attr.name, tmplist->key->name);
                        fprintf(outfilecpp, "%s %s_%s ( %s *this", umlo->key.attr.type, tmplist->key->name, umlo->key.attr.name, tmplist->key->name);
                    }
                    tmpa = umlo->key.parameters;
                    if (tmpa != NULL) {
                        fprintf(outfileh, ", ");
                        fprintf(outfilecpp, ", ");
                    }
                    while (tmpa != NULL) {
                        fprintf(outfileh, "%s %s", tmpa->key.type, tmpa->key.name);
                        fprintf(outfilecpp, "%s %s", tmpa->key.type, tmpa->key.name);
                        /*if ( tmpa->key.value[0] != 0 ) {
                         fprintf(outfileh," = %s",tmpa->key.value);
                         if ( ! umlo->key.attr.isabstract ) {
                         fprintf(outfilecpp," = %s",tmpa->key.value);
                         }
                         }*/
                        tmpa = tmpa->next;
                        if (tmpa != NULL) {
                            fprintf(outfileh, ", ");
                            if ( ! umlo->key.attr.isabstract ) {
                                fprintf(outfilecpp, ", ");
                            }
                        }
                    }
                    fprintf(outfileh, " )");
                    fprintf(outfilecpp, " )");

                    if ( umlo->key.attr.value[0] != 0 ) {
                        fprintf(outfileh, " = %s", umlo->key.attr.value);
                    }
                    fprintf(outfileh, ";\n");
                    if ( umlo->key.attr.isabstract ) {
                        fprintf(outfilecpp, "{\n    this->%s(", umlo->key.attr.name);
                        tmpa = umlo->key.parameters;
                        if (tmpa != NULL) {
                            fprintf(outfilecpp, "this, ");
                        } else {
                            fprintf(outfilecpp, "this");
                        }
                        while (tmpa != NULL) {
                            fprintf(outfilecpp, "%s", tmpa->key.name);
                            tmpa = tmpa->next;
                            if (tmpa != NULL) {
                                fprintf(outfilecpp, ", ");
                            }
                        }
                        fprintf(outfilecpp, ");\n}\n\n");
                    } else {
                        fprintf(outfilecpp, "{\n}\n\n");
                    }
                    umlo = umlo->next;
                }

                fprintf(outfileh, "#endif\n");

                fclose(outfileh);
                fclose(outfilecpp);
            }

        }
        tmplist = tmplist->next;
    }
}
