/***************************************************************************
                          generate_code_ada.c  -  description
                             -------------------
    begin                : 2001-07-18
    copyright            : (C) 2001 by Thomas Preymesser
    email                : tp@thopre.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#define PACKAGE_EXT "_types"
#define SPEC_EXT "ads"
#define BODY_EXT "adb"

#include "dia2code.h"

void generate_code_ada(batch *b) {
    umlclasslist tmplist, parents;
    namelist used_classes, tmpnamelist;
    int tmpv;
    umlattrlist umla, tmpa;
    umloplist umlo;
    char *tmpname;
    char outfilename[256];
    FILE * outfileads, *outfileadb, *dummyfile, *licensefile = NULL;

    int tmpdirlgth, tmpfilelgth;
    int first_arg;

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

            sprintf(outfilename, "%s/%s_types.ads", b->outdir, tmpname);
            dummyfile = fopen(outfilename, "r");
            if ( b->clobber || ! dummyfile ) {

                outfileads = fopen(outfilename, "w");
                if ( outfileads == NULL ) {
                    fprintf(stderr, "Can't open file %s for writing\n", outfilename);
                    exit(3);
                }

                /* This prevents buffer overflows */
                tmpfilelgth = strlen(tmpname);
                if (tmpfilelgth + tmpdirlgth > sizeof(*outfilename) - 2) {
                    fprintf(stderr, "Sorry, name of file too long ...\nTry a smaller dir name\n");
                    exit(4);
                }

                sprintf(outfilename, "%s/%s_types.adb", b->outdir, tmpname);
                outfileadb = fopen(outfilename, "w");
                if ( outfileadb == NULL ) {
                    fprintf(stderr, "Can't open file %s for writing\n", outfilename);
                    exit(3);
                }

                /* add license to the header */
                if(b->license != NULL){
                    char lc;
                    rewind(licensefile);
                    while((lc = fgetc(licensefile)) != EOF){
                        fprintf(outfileads,"%c",lc);
                        fprintf(outfileadb,"%c",lc);
                    }
                }

                /*prey: fprintf(outfileadb,"#include \"%s.ads\"\n\n",tmpname);*/

                free(tmpname);

                tmpname = strtoupper(tmplist->key->name);
                free(tmpname);

                used_classes = find_classes(tmplist, b);
                tmpnamelist = used_classes;
                while (tmpnamelist != NULL) {
                    tmpname = strtolower(tmpnamelist->name);
                    fprintf(outfileads, "with %s_types;\nuse %s_types\n", tmpname, tmpname);
                    tmpnamelist = tmpnamelist->next;
                    free(tmpname);
                }

                tmpname = strtolower(tmplist->key->name);
                fprintf(outfileads, "package %s_types is\n",  tmpname);
                fprintf(outfileadb, "package body %s_types is\n\n",  tmpname);
                free(tmpname);

                fprintf(outfileads, "\n");

                if ( strlen(tmplist->key->stereotype) > 0 ) {
                    fprintf(outfileads, "   --  %s\n", tmplist->key->stereotype);
                }

                parents = tmplist->parents;
                if (parents != NULL) {
                    //prey: Mehrfachvererbung. In Ada nicht moeglich
                    //prey: Wir lassen absichtlich ungueltigen Code erzeugen,
                    //prey: falls Mehrfachvererbung angegeben wurde ...
                    while ( parents != NULL ) {
                        fprintf(outfileads, "type %s is new %s with record",
                                tmplist->key->name,
                                parents->key->name);
                        /*prey:fprintf(outfileads,"public %s",parents->key->name);*/
                        /*prey:fprintf(outfileads," %s",parents->key->name);*/
                        parents = parents->next;
                        if ( parents != NULL ) {
                            fprintf(outfileads, ", ");
                        }
                    }
                } else {
                    fprintf(outfileads, "   type %s is tagged record", tmplist->key->name);
                }
                fprintf(outfileads, "\n");

                fprintf(outfileads, "      --  Attributes\n");
                tmpv = -1;
                umla = tmplist->key->attributes;
                while ( umla != NULL) {
                    fprintf(outfileads, "   ");
                    if ( tmpv != umla->key.visibility ) {
                        switch (umla->key.visibility) {
                        case '0':
                            fprintf (outfileads, "   --  public:\n      ");
                            break;
                        case '1':
                            fprintf (outfileads, "   --  private:\n      ");
                            break;
                        case '2':
                            fprintf (outfileads, "   --  protected:\n      ");
                            break;
                        }
                        tmpv = umla->key.visibility;
                    } else {
                        fprintf (outfileads, "   ");
                    }

                    if (umla->key.isstatic) {
                        fprintf(outfileads, "static ");
                    }
                    fprintf(outfileads, "%-20s : %s", umla->key.name, umla->key.type);
                    if ( umla->key.value[0] != 0 && umla->key.isstatic) {
                        /*prey:fprintf(outfileadb,"%s %s::%s",umla->key.type,tmplist->key->name,umla->key.name);*/
                        fprintf(outfileadb, "function %s return %s",
                                umla->key.name,
                                umla->key.type);
                        fprintf(outfileadb, " = %s", umla->key.value);
                        fprintf(outfileadb, ";\n");
                    }

                    fprintf(outfileads, ";\n");

                    umla = umla->next;
                }

                umlo = tmplist->key->operations;
                fprintf(outfileads, "   end record;\n");
                fprintf(outfileads, "\n--   Operations\n");
                tmpv = -1;
                while ( umlo != NULL) {
                    fprintf(outfileads, "  ");
                    if ( tmpv != umlo->key.attr.visibility ) {
                        switch (umlo->key.attr.visibility) {
                        case '0':
                            fprintf(outfileads, "--   public:\n    ");
                            break;
                        case '1':
                            fprintf(outfileads, "--   private:\n    ");
                            break;
                        case '2':
                            fprintf(outfileads, "--   protected:\n    ");
                            break;
                        }
                        tmpv = umlo->key.attr.visibility;
                    } else {
                        fprintf (outfileads, "  ");
                    }

                    if ( umlo->key.attr.isabstract ) {
                        fprintf(outfileads, "virtual ");
                        umlo->key.attr.value[0] = '0';
                    }
                    if ( umlo->key.attr.isstatic ) {
                        fprintf(outfileads, "static ");
                    }
                    fprintf(outfileads, "function %s ", umlo->key.attr.name);
                    if ( ! umlo->key.attr.isabstract ) {
                        fprintf(outfileadb, "function %s ", umlo->key.attr.name);
                    }
                    tmpa = umlo->key.parameters;
                    first_arg = 0;
                    while (tmpa != NULL) {
                        if ( first_arg == 0 ) {
                            fprintf(outfileads, "( ");
                            fprintf(outfileadb, "( ");
                            first_arg = 1;
                        }
                        fprintf(outfileads, "%s : %s",
                                tmpa->key.name,
                                tmpa->key.type
                               );
                        if ( ! umlo->key.attr.isabstract ) {
                            fprintf(outfileadb, "%s: %s",
                                    tmpa->key.name,
                                    tmpa->key.type
                                   );
                        }
                        if ( tmpa->key.value[0] != 0 ) {
                            fprintf(outfileads, " := %s", tmpa->key.value);
                            if ( ! umlo->key.attr.isabstract ) {
                                fprintf(outfileadb, " := %s", tmpa->key.value);
                            }
                        }
                        tmpa = tmpa->next;
                        if (tmpa != NULL) {
                            fprintf(outfileads, ", ");
                            if ( ! umlo->key.attr.isabstract ) {
                                fprintf(outfileadb, ", ");
                            }
                        }
                        if ( first_arg > 0 ) {
                            fprintf(outfileads, " ) ");
                            fprintf(outfileadb, " ) ");
                        }
                    }
                    fprintf(outfileads, "return ");
                    fprintf(outfileadb, "return ");
                    /*prey: 3 Zeilen nach hier unten verschoben*/
                    if (strlen(umlo->key.attr.type) > 0) {
                        fprintf(outfileads, "%s", umlo->key.attr.type);
                        fprintf(outfileadb, "%s", umlo->key.attr.type);
                    }
                    if ( ! umlo->key.attr.isabstract ) {
                        if ( first_arg > 0 ) {
                            ;  /*prey: fprintf(outfileadb," )");*/
                        }
                    }
                    if ( umlo->key.attr.value[0] != 0 ) {
                        fprintf(outfileads, " = %s", umlo->key.attr.value);
                    }
                    fprintf(outfileads, ";\n");
                    if ( ! umlo->key.attr.isabstract ) {
                        fprintf(outfileadb, " is\nbegin\nend %s;\n\n",
                                umlo->key.attr.name);
                    }
                    umlo = umlo->next;
                }
                tmpname = strtolower(tmplist->key->name);
                fprintf(outfileads, "end %s_types;\n", tmpname);
                fprintf(outfileadb, "begin\n");
                fprintf(outfileadb, "end %s_types;\n", tmpname);
                free(tmpname);

                fclose(outfileads);
                fclose(outfileadb);
            }

        }
        tmplist = tmplist->next;
    }
}
