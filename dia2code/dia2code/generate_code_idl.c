/***************************************************************************
         generate_code_idl.c  -  Function that generates CORBA IDL
                             -------------------
    begin                : Sun Dec 30 2001
    copyright            : (C) 2000-2001 by Chris McGee
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

#include "dia2code.h"

void generate_code_idl(batch *b) {
    umlclasslist tmplist, parents;
    umltemplatelist templates;
    umlassoclist associations;
    namelist used_classes, tmpnamelist;
    int tmpv;
    umlattrlist umla, tmpa;
    umloplist umlo;
    char *tmpname;
    char outfilename[256];
    FILE * outfile, *dummyfile, *licensefile = NULL;

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

            sprintf(outfilename, "%s/%s.idl", b->outdir, tmpname);
            dummyfile = fopen(outfilename, "r");
            if ( b->clobber || ! dummyfile ) {

                outfile = fopen(outfilename, "w");
                if ( outfile == NULL ) {
                    fprintf(stderr, "Can't open file %s for writing\n", outfilename);
                    exit(3);
                }

                /* add license to the header */
                if(b->license != NULL){
                    char lc;
                    rewind(licensefile);
                    while((lc = fgetc(licensefile)) != EOF){
                        fprintf(outfile,"%c",lc);
                    }
                }

                free(tmpname);

                tmpname = strtoupper(tmplist->key->name);
                fprintf(outfile, "#ifndef %s_IDL\n", tmpname);
                fprintf(outfile, "#define %s_IDL\n\n", tmpname);
                free(tmpname);

                used_classes = find_classes(tmplist, b);
                tmpnamelist = used_classes;
                while (tmpnamelist != NULL) {
                    tmpname = strtolower(tmpnamelist->name);
                    fprintf(outfile, "#include \"%s.idl\"\n", tmpname);
                    tmpnamelist = tmpnamelist->next;
                    free(tmpname);
                }

                fprintf(outfile, "\n");

                if ( strlen(tmplist->key->stereotype) > 0 ) {
                    fprintf(outfile, "// %s\n", tmplist->key->stereotype);
                }
                fprintf(outfile, "interface %s", tmplist->key->name);

                parents = tmplist->parents;
                if (parents != NULL) {
                    fprintf(outfile, ":");
                    while ( parents != NULL ) {
                        fprintf(outfile, " %s", parents->key->name);
                        parents = parents->next;
                        if ( parents != NULL ) {
                            fprintf(outfile, ": ");
                        }
                    }
                }
                fprintf(outfile, " {\n");

                fprintf(outfile, "  // Associations\n");
                associations = tmplist->associations;
                while (associations != NULL) {
                    fprintf(outfile, "   %s ", associations->key->name);

										/* I'm not sure whether pointers are actually supported in CORBA
										   in the mean time, pointers will be disabled.
                      if (associations->composite == 0) {
                        fprintf(outfile, "* ");
                    }*/

                    fprintf(outfile, "%s;\n", associations->name);
                    associations = associations->next;
                }

                fprintf(outfile, "  // Attributes\n");
                tmpv = -1;
                umla = tmplist->key->attributes;
                while ( umla != NULL) {
                    fprintf(outfile, "  ");

										switch (umla->key.visibility) {
												case '1':
														fprintf (outfile, "private ");
														break;
										}

										/* Not sure if there's support for class variables
                    if (umla->key.isstatic) {
                        fprintf(outfile, "static ");
                    }*/

                    fprintf(outfile, "attribute %s %s", umla->key.type, umla->key.name);

                    fprintf(outfile, ";\n");

                    umla = umla->next;
                }

                umlo = tmplist->key->operations;
                fprintf(outfile, "  // Operations\n");
                tmpv = -1;
                while ( umlo != NULL) {
                    fprintf(outfile, "  ");

                    if ( umlo->key.attr.isabstract ) {
                        fprintf(outfile, "abstract ");
                        umlo->key.attr.value[0] = '0';
                    }

										switch (umlo->key.attr.visibility) {
												case '1':
														fprintf(outfile, "private ");
                            break;
										}

                    /*if ( umlo->key.attr.isstatic ) {
                        fprintf(outfileh, "static ");
                    }*/

                    if (strlen(umlo->key.attr.type) > 0) {
                        fprintf(outfile, "%s ", umlo->key.attr.type);
                    }
                    fprintf(outfile, "%s ( ", umlo->key.attr.name);

                    tmpa = umlo->key.parameters;
                    while (tmpa != NULL) {
                        fprintf(outfile, "in %s %s", tmpa->key.type, tmpa->key.name);

												/* I'm not sure if idl supports the notion of default values for
												    parameters
                        if ( tmpa->key.value[0] != 0 ) {
                            fprintf(outfile, " = %s", tmpa->key.value);
                        }*/

                        tmpa = tmpa->next;
                        if (tmpa != NULL) {
                            fprintf(outfile, ", ");
                        }
                    }

                    fprintf(outfile, " )");

                    if ( umlo->key.attr.value[0] != 0 ) {
                        fprintf(outfile, " = %s", umlo->key.attr.value);
                    }
                    fprintf(outfile, ";\n");
                    umlo = umlo->next;
                }
                fprintf(outfile, "};\n\n");

                fprintf(outfile, "#endif\n");

                fclose(outfile);
            }

        }
        tmplist = tmplist->next;
    }
}
