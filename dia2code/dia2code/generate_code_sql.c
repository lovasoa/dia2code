/***************************************************************************
                          generate_code_sql.c  -  description
                             -------------------
    begin                : Sat Jun 2 2001
    email                : sirnewton_01@yahoo.ca
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "../dia2code/dia2code.h"

void generate_code_sql(batch *b) {
    /*
    umlclasslist tmplist,parents,dependencies;
    umlassoclist associations;
    namelist used_classes,tmpnamelist;
    int tmpv;
    umlattrlist umla,tmpa;
    umloplist umlo;
    */
    umlclasslist tmplist;
    umlattrlist umla;
    int tmpv;
    char *tmpname;
    char outfilename[256];
    FILE * outfilesql, *dummyfile;

    int tmpdirlgth, tmpfilelgth;

    if (b->outdir == NULL) {
        b->outdir = ".";
    }

    tmpdirlgth = strlen(b->outdir);

    tmplist = b->classlist;

    while ( tmplist != NULL ) {

        if ( ! ( is_present(b->classes, tmplist->key->name) ^ b->mask ) ) {

            tmpname = strtolower(tmplist->key->name);

            /* This prevents buffer overflows */
            tmpfilelgth = strlen(tmpname);
            if (tmpfilelgth + tmpdirlgth > sizeof(*outfilename) - 2) {
                fprintf(stderr, "Sorry, name of file too long ...\nTry a smaller dir name\n");
                exit(4);
            }

            sprintf(outfilename, "%s/DEFINITION.SQL", b->outdir);
            dummyfile = fopen(outfilename, "r");
            if ( b->clobber || ! dummyfile ) {

                outfilesql = fopen(outfilename, "a");
                if ( outfilesql == NULL ) {
                    fprintf(stderr, "Can't open file %s for writing\n", outfilename);
                    exit(3);
                }

                /* This prevents buffer overflows */
                tmpfilelgth = strlen(tmpname);
                if (tmpfilelgth + tmpdirlgth > sizeof(*outfilename) - 2) {
                    fprintf(stderr, "Sorry, name of file too long ...\nTry a smaller dir name\n");
                    exit(4);
                }

                free(tmpname);

                fprintf(outfilesql, "CREATE TABLE %s(\n", tmplist->key->name);

                fprintf(outfilesql, "-- Attributes --\n");
                tmpv = -1;
                umla = tmplist->key->attributes;
                while ( umla != NULL) {
                    fprintf(outfilesql, "%s %s", umla->key.name, umla->key.type);
                    if (umla->next != NULL) {
                        fprintf(outfilesql, ",\n");
                    }
                    umla = umla->next;
                }

                fprintf(outfilesql, ");\n\n");
                fclose(outfilesql);
            }

        }
        tmplist = tmplist->next;
    }

}
