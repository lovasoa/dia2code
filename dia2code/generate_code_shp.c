/***************************************************************************
                          generate_code_shp.c  - generate batch file for
                                                 shapefile creation 
                             -------------------
    begin                : Tue Oct 16 2001
    copyright            : (C) by Steffen Macke
    email                : sdteffen@yahoo.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/* this requires shpcreate and dbfcreate from shapelib */

#include "dia2code.h"

void generate_code_shp(batch *b) {
    /*
    umlclasslist tmplist,parents,dependencies;
    umlassoclist associations;
    namelist used_classes,tmpnamelist;
    int tmpv;
    umlattrlist umla,tmpa;
    umloplist umlo;
    */
    umlclasslist tmplist;
    umlclasslist parentlist, parentlist2;
    umlattrlist umla;
    int tmpv;
    char *tmpname;
    char outfilename[BIG_BUFFER];
    FILE * outfileshp, *dummyfile;

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

            sprintf(outfilename, "%s/createshapefiles.bat", b->outdir);
            dummyfile = fopen(outfilename, "r");
            if ( b->clobber || ! dummyfile ) {

                outfileshp = fopen(outfilename, "a");
                if ( outfileshp == NULL ) {
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

                tmpv = -1;
  
                if(tmplist->key->isabstract == 0) {

                    /* create attribute table */
                    umla = tmplist->key->attributes;
                    parentlist = tmplist;
                    fprintf(outfileshp, "dbfcreate %s", tmplist->key->name);

                    while ( umla != NULL) {
                        if((strcmp(umla->key.name,"Shape") != 0)&&
                           (umla->key.visibility != 1)) {
                            if(strcmp(umla->key.type,"String") == 0) {
                                fprintf(outfileshp, " -s %s 255", 
                                        umla->key.name);
                            }
                            else if((strcmp(umla->key.type, 
                                           "CodedValue") == 0)||
                                strcmp(umla->key.type, "Integer") ==0 ) {
                                fprintf(outfileshp, " -n %s 16 0",
                                         umla->key.name);
                            }
                            else if(strcmp(umla->key.type, "Float") == 0) {
                                fprintf(outfileshp, " -n %s 16 3",
                                        umla->key.name);
                            }
                        }                
                        umla = umla->next;

                        if((umla == NULL)&&(parentlist != NULL)) {
                            parentlist = parentlist->parents;
                            if(parentlist != NULL) {
                                umla = parentlist->key->attributes;
                                parentlist2 = b->classlist;
                                while((strcmp(parentlist->key->name, 
                                              parentlist2->key->name) != 0)&&
                                      (parentlist2 != NULL))
                                    parentlist2 = parentlist2->next;
                                parentlist = parentlist2;
                            }
                        }

                    }
                    fprintf(outfileshp, "\n");
                    
                    /* create shp file */
                    umla = tmplist->key->attributes;
                       parentlist = tmplist;
                    while ( umla != NULL) {
                        if(strcmp(umla->key.name,"Shape") == 0) {
                            if(strcmp(strtolower(umla->key.type), 
                                      "polyline") == 0) {
                                strcpy(umla->key.type, "arc");
                            }
                            fprintf(outfileshp, "shpcreate %s %s\n\n", 
                                    tmplist->key->name, 
                                    strtolower(umla->key.type));
                            break;
                        }
                        umla = umla->next;

                        if((umla == NULL)&&(parentlist != NULL)) {
                            parentlist = parentlist->parents;
                            if(parentlist != NULL) {
                                umla = parentlist->key->attributes;
                                parentlist2 = b->classlist;
                                while((strcmp(parentlist->key->name, 
                                              parentlist2->key->name) != 0)&&
                                      (parentlist2 != NULL))
                                    parentlist2 = parentlist2->next;
                                parentlist = parentlist2;
                            }
                        }

                    }
                }
            }            
            fclose(outfileshp);
        }
        tmplist = tmplist->next;
    }
}

