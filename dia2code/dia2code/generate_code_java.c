/***************************************************************************
          generate_code_java.c  -  Function that generates Java code
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

void generate_code_java(batch *b) {
    umlclasslist tmplist, parents;
    umlassoclist associations;
    umlattrlist umla, tmpa;
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

            sprintf(outfilename, "%s/%s.java", b->outdir, tmplist->key->name);
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

                tmppcklist = make_package_list(tmplist->key->package);
                if ( tmppcklist != NULL ){
                    fprintf(outfile,"package %s",tmppcklist->key->name);
                    tmppcklist=tmppcklist->next;
                    while ( tmppcklist != NULL ){
                        fprintf(outfile,".%s",tmppcklist->key->name);
                        tmppcklist=tmppcklist->next;
                    }
                    fprintf(outfile,";\n\n");
                }

                /* We generate the import clauses */
                used_classes = list_classes(tmplist, b);
                while (used_classes != NULL) {
                    tmppcklist = make_package_list(used_classes->key->package);
                    if ( tmppcklist != NULL ){
                        if ( strcmp(tmppcklist->key->id,tmplist->key->package->id)){
                            /* This class' package and our current class' package are
                               not the same */
                            fprintf(outfile, "import ");
                            fprintf(outfile,"%s",tmppcklist->key->name);
                            tmppcklist=tmppcklist->next;
                            while ( tmppcklist != NULL ){
                                fprintf(outfile, ".%s", tmppcklist->key->name);
                                tmppcklist=tmppcklist->next;
                            }
                            fprintf(outfile,".");
                            fprintf(outfile,"%s;\n",used_classes->key->name);
                        }
                    } else {
                        /* No info for this class' package, we include it directly */
                        fprintf(outfile, "import %s;\n",used_classes->key->name);
                    }
                    used_classes = used_classes->next;
                }

                fprintf(outfile, "\n");
                fprintf(outfile, "public ");

                tmpname = strtolower(tmplist->key->stereotype);
                if ( ! strcmp("interface", tmpname) ) {
                    fprintf(outfile, "interface ");
                } else {
                    if (tmplist->key->isabstract) {
                        fprintf(outfile, "abstract ");
                    }
                    fprintf(outfile, "class ");
                }
                free(tmpname);

                fprintf(outfile, "%s", tmplist->key->name);

                parents = tmplist->parents;
                if (parents != NULL) {
                    while ( parents != NULL ) {
                        tmpname = strtolower(parents->key->stereotype);
                        if ( ! strcmp(tmpname, "interface") ) {
                            fprintf(outfile, " implements ");
                        } else {
                            fprintf(outfile, " extends ");
                        }
                        free(tmpname);
                        fprintf(outfile, "%s", parents->key->name);
                        parents = parents->next;
                    }
                }
                fprintf(outfile, " {\n");

                umla = tmplist->key->attributes;

                if( umla != NULL)
                    fprintf(outfile, "  /** Attributes */\n");

                while ( umla != NULL) {
                    switch (umla->key.visibility) {
                    case '0':
                        fprintf (outfile, "  public ");
                        break;
                    case '1':
                        fprintf (outfile, "  private ");
                        break;
                    case '2':
                        fprintf (outfile, "  protected ");
                        break;
                    }

                    if (umla->key.isstatic) {
                        fprintf(outfile, "static ");
                    }
                    fprintf(outfile, "%s %s", umla->key.type, umla->key.name);
                    if ( umla->key.value[0] != 0 ) {
                        fprintf(outfile, " = %s", umla->key.value);
                    }
                    fprintf(outfile, ";\n");
                    umla = umla->next;
                }

                associations = tmplist->associations;
	        if (associations != NULL)
                    fprintf(outfile, "  /** Associations */\n");

                while ( associations != NULL ) {
                    fprintf(outfile, "  private %s %s;\n", associations->key->name, associations->name);
                    associations = associations->next;
                }

		umlo = tmplist->key->operations;
                while ( umlo != NULL) {
                    fprintf(outfile, "  /**\n");
                    fprintf(outfile, "   * Operation\n");
                    fprintf(outfile, "   *\n");
                    tmpa = umlo->key.parameters;
                    while (tmpa != NULL) {
                        fprintf(outfile, "   * @param %s\n", tmpa->key.name);
                        tmpa = tmpa->next;
		    }
                    if(strcmp(umlo->key.attr.type, "void"))
                        fprintf(outfile, "   * @return %s\n", umlo->key.attr.type);
                    fprintf(outfile, "   */\n");


                    fprintf(outfile, "  ");

                    if ( umlo->key.attr.isabstract ) {
                        fprintf(outfile, "abstract ");
                        umlo->key.attr.value[0] = '0';
                    }

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

                    if ( umlo->key.attr.isstatic ) {
                        fprintf(outfile, "static ");
                    }
                    if (strlen(umlo->key.attr.type) > 0) {
                        fprintf(outfile, "%s ", umlo->key.attr.type);
                    }
                    fprintf(outfile, "%s ( ", umlo->key.attr.name);
                    tmpa = umlo->key.parameters;
                    while (tmpa != NULL) {
                        fprintf(outfile, "%s %s", tmpa->key.type, tmpa->key.name);
                        /*
                        if ( tmpa->key.value[0] != 0 ){
                            fprintf(outfile," = %s",tmpa->key.value);
                    }  */
                        tmpa = tmpa->next;
                        if (tmpa != NULL) fprintf(outfile, ", ");
                    }
                    fprintf(outfile, " )");
                    if ( umlo->key.attr.isabstract ) {
                        fprintf(outfile, ";\n");
                    } else {
                        fprintf(outfile, "{\n");
                        if ( umlo->key.implementation != NULL ) {
                            fprintf(outfile, "%s\n", umlo->key.implementation);
                        }
                        fprintf(outfile, "  }\n");
                    }
                    umlo = umlo->next;
                }
                fprintf(outfile, "}\n\n");

                fclose(outfile);
            }
        }
        tmplist = tmplist->next;
    }
}
