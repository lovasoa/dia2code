/***************************************************************************
          generate_code_php.c  -  Function that generates Php code
                             -------------------
    begin                : Sat Dec 12 2001
    copyright            : (C) 2001 by Harald Fielker
    email                : fielker@softsolutions.de
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

#define TABS "    "  /* 4 */

void generate_code_php(batch *b) {
    umlclasslist tmplist, parents, incparent;
    umlassoclist associations;
    umlattrlist umla, tmpa, parama;
    umlpackagelist tmppcklist;
    umloplist umlo;
    char *tmpname;
    char outfilename[90];
    FILE * outfile, *dummyfile, *licensefile = NULL;
    umlclasslist used_classes;

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

            tmpname = tmplist->key->name;

            /* This prevents buffer overflows */
            tmpfilelgth = strlen(tmpname);
            if (tmpfilelgth + tmpdirlgth > sizeof(*outfilename) - 2) {
                fprintf(stderr, "Sorry, name of file too long ...\nTry a smaller dir name\n");
                exit(4);
            }

            sprintf(outfilename, "%s/%s.class.php", b->outdir, tmplist->key->name);
            dummyfile = fopen(outfilename, "r");
            if ( b->clobber || ! dummyfile ) {

                outfile = fopen(outfilename, "w");
                if ( outfile == NULL ) {
                    fprintf(stderr, "Can't open file %s for writing\n", outfilename);
                    exit(3);
                }
/* header */
                /* add license to the header */
                if(b->license != NULL){
                    char lc;
                    rewind(licensefile);
                    while((lc = fgetc(licensefile)) != EOF){
                        fprintf(outfile,"%c",lc);
                    }
                }

fprintf(outfile,"<?php\n\n" );
                /* We generate the include clauses */
                used_classes = list_classes(tmplist, b);
                while (used_classes != NULL) {
                    tmppcklist = make_package_list(used_classes->key->package);
                    if ( tmppcklist != NULL ){
                        if ( strcmp(tmppcklist->key->id,tmplist->key->package->id)){
                            /* This class' package and our current class' package are
                               not the same */
                            fprintf(outfile, "#include (\"");
                            fprintf(outfile,"%s",tmppcklist->key->name);
                            tmppcklist=tmppcklist->next;
                            while ( tmppcklist != NULL ){
                                fprintf(outfile, "/%s", tmppcklist->key->name);
                                tmppcklist=tmppcklist->next;
                            }
                            fprintf(outfile,"/");
                            fprintf(outfile,"%s.class.php\");\n",used_classes->key->name);
                        }
                    } else {
                        /* No info for this class' package, we include it directly */
                        fprintf(outfile, "#include( \"%s.class.php\" );\n",used_classes->key->name);
                    }
                    used_classes = used_classes->next;
                }

                incparent = tmplist->parents;
                if (incparent != NULL) {
                    while ( incparent!= NULL ) {
                        tmpname = strtolower(incparent->key->stereotype);
                        fprintf(outfile, "include( \"");
                        free(tmpname);
                        fprintf(outfile, "%s.class.php\" );\n", incparent->key->name);
                        incparent = incparent->next;
                    }
                }


fprintf(outfile,"/**\n" );
fprintf(outfile," * XXX detailed description\n" );
fprintf(outfile," *\n" );
fprintf(outfile," * @author    XXX\n" );
fprintf(outfile," * @copyright XXX\n" );

                tmppcklist = make_package_list(tmplist->key->package);
                if ( tmppcklist != NULL ){
                    int packcounter = 0;
fprintf(outfile," * @package %s",tmppcklist->key->name);
                    tmppcklist=tmppcklist->next;
                    while ( tmppcklist != NULL ){
                        if( packcounter == 1 ) {
                           fprintf(outfile,"\n" );
                           fprintf(outfile," * @subpackage %s",tmppcklist->key->name);
                        } else {
                           fprintf(outfile,".%s",tmppcklist->key->name);
                        }
                        tmppcklist=tmppcklist->next;
                        packcounter++;
                    }
                    fprintf(outfile,"\n");
                }

                tmpname = strtolower(tmplist->key->stereotype);
/*
                if ( ! strcmp("interface", tmpname) ) {
fprintf(outfile," * @interface\n" );
                } else {
*/
                    if (tmplist->key->isabstract) {
fprintf(outfile," * @abstract\n" );
                    }
/*
                }
*/
                free(tmpname);

fprintf(outfile," */\n" );

                fprintf(outfile, "class %s", tmplist->key->name);

                parents = tmplist->parents;
                if (parents != NULL) {
                    while ( parents != NULL ) {
                        tmpname = strtolower(parents->key->stereotype);
                        fprintf(outfile, " extends ");
                        free(tmpname);
                        fprintf(outfile, "%s", parents->key->name);
                        parents = parents->next;
                    }
                }
                fprintf(outfile, " {\n");

                fprintf(outfile, "%s// Attributes\n", TABS);

                umla = tmplist->key->attributes;
                while ( umla != NULL) {
fprintf(outfile,"   /**\n" );
fprintf(outfile,"    *    XXX\n" );
fprintf(outfile,"    *    @access " );
                    switch (umla->key.visibility) {
                    case '0':
                        fprintf (outfile, "public");
                        break;
                    case '1':
                        fprintf (outfile, "private");
                        break;
                    case '2':
                        fprintf (outfile, "protected");
                        break;
                    }
fprintf(outfile,"\n" );
fprintf(outfile,"    */\n" );

/*
                    if (umla->key.isstatic) {
                        fprintf(outfile, "static ");
                    }
*/
/*                    fprintf(outfile, "%s %s", umla->key.type, umla->key.name);
*/
                    fprintf(outfile, TABS);

                    fprintf(outfile, "var $%s", umla->key.name);
                    if ( umla->key.value[0] != 0 ) {
                        fprintf(outfile, " = %s", umla->key.value);
                    }
                    fprintf(outfile, ";\n\n");
                    umla = umla->next;
                }

                fprintf(outfile, "%s// Associations\n", TABS);
                associations = tmplist->associations;
                while ( associations != NULL ) {
fprintf(outfile,"   /**\n" );
fprintf(outfile,"    *    XXX\n" );
fprintf(outfile,"    *    @accociation %s to %s\n",
                        associations->key->name, associations->name);
fprintf(outfile,"    *    @access private\n" );
fprintf(outfile,"    */\n" );
                    fprintf(outfile, "%s#var $%s;\n\n", 
                     TABS, associations->name);
                    associations = associations->next;
                }

                umlo = tmplist->key->operations;
                fprintf(outfile, "%s// Operations\n", TABS);
                while ( umlo != NULL) {

fprintf(outfile,"   /**\n" );
fprintf(outfile,"    *    XXX\n" );
fprintf(outfile,"    *    \n" );

                    if ( umlo->key.attr.isabstract ) {
fprintf(outfile,"    *    @abstract\n" );
                        umlo->key.attr.value[0] = '0';
                    }

fprintf(outfile,"    *    @access " );
                    switch (umlo->key.attr.visibility) {
                    case '0':
                        fprintf (outfile, "public ");
                        break;
                    case '1':
                        fprintf (outfile, "private ");
                        break;
                    case '2':
                        fprintf (outfile, "protected ");
                        break;
                    }
fprintf(outfile,"\n" );

/*                    if ( umlo->key.attr.isstatic ) {
                        fprintf(outfile, "static ");
                    }
*/
                    if (strlen(umlo->key.attr.type) > 0) {
fprintf(outfile,"    *    @returns %s\n", umlo->key.attr.type);
                    }
                    parama = umlo->key.parameters;
// document parameters
                    while (parama != NULL) {
fprintf(outfile,"    *    @param %s $%s XXX\n", parama->key.type, parama->key.name);
                        parama= parama->next;
                    }

fprintf(outfile,"    */\n" );

                    fprintf(outfile, TABS);
                    fprintf(outfile, "function %s ( ", umlo->key.attr.name);
                    tmpa = umlo->key.parameters;
                    while (tmpa != NULL) {
                        fprintf(outfile, "$%s", tmpa->key.name);
                        if ( tmpa->key.value[0] != 0 ){
                            fprintf(outfile," = %s",tmpa->key.value);
                        }
                        tmpa = tmpa->next;
                        if (tmpa != NULL) fprintf(outfile, ", ");
                    }
                    fprintf(outfile, " )");
/*                    if ( umlo->key.attr.isabstract ) {
                        fprintf(outfile, ";\n");
                    } else {
*/
                        fprintf(outfile, "{\n");
                        if ( umlo->key.implementation != NULL ) {
                            fprintf(outfile, "%s\n", umlo->key.implementation);
                        }
                        fprintf(outfile, "%s}\n\n", TABS);
/*                    }
*/
                    umlo = umlo->next;
                }
                fprintf(outfile, "}\n\n");

fprintf(outfile,"\n\n?>\n" );
                fclose(outfile);
            }
        }
        tmplist = tmplist->next;
    }
}
