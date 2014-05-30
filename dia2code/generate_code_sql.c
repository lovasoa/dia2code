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

#include "dia2code.h"
#include "parse_diagram.h"
#include "code_generators.h"

void inherit_attributes(umlclasslist parents, umlattrlist umla) {
    umlattrlist pumla;

    while (parents != NULL) {
        pumla = parents->key->attributes;
        insert_attribute(copy_attributes(pumla), umla);
        parents = parents->next;
    }
}

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
    char outfilename[BIG_BUFFER];
    FILE * outfilesql, *dummyfile;

    int tmpdirlgth, tmpfilelgth;

    if (b->outdir == NULL) {
        b->outdir = ".";
    }

    tmpdirlgth = strlen(b->outdir);

    tmplist = b->classlist;
    
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
        outfilesql = fopen(outfilename, "w"); /* Moved this one to be able to OVERWRITE old file, changed from "a" to "w"... */
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
    }


    while ( tmplist != NULL ) {

        if ( ! ( is_present(b->classes, tmplist->key->name) ^ b->mask ) ) {
            char seenFirst = 0;

            if (tmplist->key->isabstract) {
                tmplist = tmplist->next;
                continue;
            }

            /* Class (table) */
            fprintf(outfilesql, "CREATE TABLE %s(\n", tmplist->key->name);

            /* Attributes (columns) */
            fprintf(outfilesql, "-- Attributes --\n");
            tmpv = -1;
            umla = tmplist->key->attributes;
            inherit_attributes (tmplist->parents, umla);
            while ( umla != NULL) {
                fprintf(outfilesql, "%s %s", umla->key.name, umla->key.type);
                if (umla->next != NULL) {
                    fprintf(outfilesql, ",\n");
                }
                umla = umla->next;
            }
            fprintf(outfilesql, ");\n\n");

            /* IsStatic attribute (Primary Key) */
            umla = tmplist->key->attributes;
            while ( umla != NULL) {
                if( umla->key.isstatic ) {
                    if( !seenFirst ) {
                            seenFirst = 1;
                            fprintf(outfilesql, "ALTER TABLE  %s ADD\n", tmplist->key->name);
                            fprintf(outfilesql, "    CONSTRAINT  PK_%s  PRIMARY KEY\n    (\n", tmplist->key->name);
                    }
                    fprintf(outfilesql, "        %s", umla->key.name);
                    if (umla->next != NULL && umla->next->key.isstatic) {
                        fprintf(outfilesql, ",");
                    }
                    fprintf(outfilesql, "\n");
                }
                umla = umla->next;
            }
            if( seenFirst )
                fprintf(outfilesql, "    );\n\n\n");
        }

        tmplist = tmplist->next;
    }

    /* Adding associations LAST since we want to have all tables around FIRST */
    tmplist = b->classlist;
    while( tmplist != NULL )
    {
        umlassocnode* temp = tmplist->associations;
        while( temp != NULL )
        {
            /*
              AI: use "surrogate key" trailing _id convention of most popular ORMs
              http://en.wikipedia.org/wiki/Surrogate_key
            */
            char tail[4];
            const char *fk_col =  temp->name;
            strncpy(tail, fk_col + strlen(fk_col) - 3, 3);
            if (!strcmp("_id", tail)) {
                fk_col = "id";
            } else {
                printf("warning: association name %s does not end in _id but rather %s,\n",
                       temp->name, tail);
                printf("make sure FK cols have the same name on both tables\n");
                fk_col = temp->name;
            }
            fprintf( outfilesql, "\n\nALTER TABLE %s ADD\n", temp->key->name );
            fprintf( outfilesql, "    CONSTRAINT  FK_%s_%s  FOREIGN KEY(%s) REFERENCES %s (%s);\n", 
                     temp->key->name, 
                     tmplist->key->name, 
                     temp->name, 
                     tmplist->key->name, 
                     fk_col );
            temp = temp->next;
        }
        tmplist = tmplist->next;
    }
    fprintf(stderr, "Finished!");
    fclose(outfilesql);
}

