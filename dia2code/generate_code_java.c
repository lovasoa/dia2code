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
#include "comment_helper.h"
#include "source_parser.h"

/* Other things to be fixed:
 * The code that determines the parent class and implementations needs to go
 * in order from "extends" to "implements".
 */

#define CLASSTYPE_CLASS      0
#define CLASSTYPE_ABSTRACT   1
#define CLASSTYPE_INTERFACE  2

#define JAVA_EXTENDS         0
#define JAVA_IMPLEMENTS      1

/**
 * get the visibility java keyword from the Dia visibility code
 * @param int the dia visibility constant
 * @return the java keyword for visibility 
 */
char *java_visibility_to_string(int visibility)
{
    switch (visibility)
    {
    case '0':
        return "public";
    case '1':
        return "private";
    case '2':
        return "protected";
    default:
        return "";
    }
}


/**
 * This function dumps out the list of interfaces and extensions, as necessary.
 */
int java_manage_parents(FILE *f, umlclasslist parents, int stereotype)
{
    char *tmpname;
    int cnt = 0;
    while ( parents != NULL )
    {
        tmpname = strtolower(parents->key->stereotype);
        if (eq(tmpname, "interface"))
        {
            if (stereotype == JAVA_IMPLEMENTS)
            {
                d2c_fprintf(f, " implements %s", parents->key->name);
                cnt++;
            }
        }
        else
        {
            if (stereotype == JAVA_EXTENDS)
            {
                d2c_fprintf(f, " extends %s", parents->key->name);
                cnt++;
            }
        }
        free(tmpname);
        parents = parents->next;
    }
    return cnt;
}

/**
 * generate and output the code for an attribute
 * @param the output file
 * @param the attribute struct to generate
 */
int java_generate_attribute( FILE * outfile, umlattribute *attr )
{
    debug( DBG_GENCODE, "generate attribute %s\n", attr->name );
    generate_attribute_comment( outfile, NULL, attr );
    d2c_fprintf(outfile, "%s ", java_visibility_to_string(attr->visibility));
    if (attr->isstatic)
        d2c_fprintf(outfile, "static ");
    d2c_fprintf(outfile, "%s %s", attr->type, attr->name);
    if ( attr->value[0] != 0 )
        d2c_fprintf(outfile, " = %s", attr->value);
    d2c_fprintf(outfile, ";\n");
    return 0;
}

/**
 * generate and output the code for an operation
 * @param the output file
 * @param the operation struct to generate
 */
int java_generate_operation( FILE * outfile, umloperation *ope, int classtype )
{
    umlattrlist tmpa;
    debug( DBG_GENCODE, "generate method %s\n", ope->attr.name );
    /** comment_helper function that generate the javadoc comment block */
    generate_operation_comment( outfile, NULL, ope );

    /* method declaration */
    if ( ope->attr.isabstract ){
        d2c_fprintf(outfile, "abstract ");
        ope->attr.value[0] = '0';
    }
    d2c_fprintf(outfile, "%s ", java_visibility_to_string(ope->attr.visibility));
    if ( ope->attr.isstatic )
        d2c_fprintf(outfile, "static ");
    if (strlen(ope->attr.type) > 0)
        d2c_fprintf(outfile, "%s ", ope->attr.type);
    d2c_fprintf(outfile, "%s ( ", ope->attr.name);
    tmpa = ope->parameters;
    while (tmpa != NULL)
    {
        d2c_fprintf(outfile, "%s %s", tmpa->key.type, tmpa->key.name);
        tmpa = tmpa->next;
        if (tmpa != NULL)
            d2c_fprintf(outfile, ", ");
    }
    d2c_fprintf(outfile, " )");
    if ( classtype == CLASSTYPE_ABSTRACT || classtype == CLASSTYPE_INTERFACE) {
        d2c_fprintf(outfile, ";\n");
    } 
    else {
        if ( ope->implementation != NULL ) {
            debug( DBG_GENCODE, "implementation found" );
            d2c_fprintf(outfile, "{%s}", ope->implementation);
        }
        else {
            d2c_fprintf(outfile, "{");
            d2c_fprintf(outfile, "}");
        }
    }
    d2c_fprintf(outfile, "\n" );
    return 0;
}


void generate_code_java(batch *b)
{
    umlclasslist tmplist;
    umlassoclist associations;
    umlattrlist umla;
    umlpackagelist tmppcklist;
    umloplist umlo;
    char *tmpname;
    char outfilename[BIG_BUFFER];
    FILE * outfile, *dummyfile, *licensefile = NULL;
    int file_found = 0;
    umlclasslist used_classes;
    umlclass *class;
    int classtype;
    sourcecode *source = NULL;
    int tmpdirlgth, tmpfilelgth;

    if (b->outdir == NULL)
        b->outdir = ".";

    tmpdirlgth = strlen(b->outdir);

    tmplist = b->classlist;

    /* open license file */
    if ( b->license != NULL )
    {
        licensefile = fopen(b->license, "r");
        if(!licensefile)
        {
            fprintf(stderr, "Can't open the license file.\n");
            exit(2);
        }
    }

    while ( tmplist != NULL )
    {
        class = tmplist->key;
        if ( is_present(b->classes,class->name) ^ b->mask ) {
            tmplist = tmplist->next;
            continue;
        }
        tmpname = class->name;

        /* This prevents buffer overflows */
        tmpfilelgth = strlen(tmpname);
        if (tmpfilelgth + tmpdirlgth > sizeof(*outfilename) - 2)
        {
            fprintf(stderr, "Sorry, name of file too long ...\nTry a smaller dir name\n");
            exit(4);
        }
        
        tmppcklist = make_package_list(tmplist->key->package);

        if (tmppcklist) {
            /* here we  calculate and create the directory if necessary */
            char *outdir = create_package_dir( b, tmppcklist->key );
            sprintf(outfilename, "%s/%s.java", outdir, tmplist->key->name);
        } else {
            sprintf(outfilename, "%s.java", tmplist->key->name);
        }

        /* get implementation code from the existing file */
        source_preserve( b, tmplist->key, outfilename, source );
        
        if ( b->clobber )
        {
            outfile = fopen(outfilename, "w");
            if ( outfile == NULL )
            {
                fprintf(stderr, "Can't open file %s for writing\n", outfilename);
                exit(3);
            }

            /* add license to the header */
            if(b->license != NULL)
            {
                char lc;
                rewind(licensefile);
                while((lc = fgetc(licensefile)) != EOF)
                    d2c_fputc(lc, outfile);
            }

            
            tmppcklist = make_package_list(class->package);
            if ( tmppcklist != NULL ){
                d2c_fprintf(outfile,"package %s",tmppcklist->key->name);
                tmppcklist=tmppcklist->next;
                while ( tmppcklist != NULL )
                {
                    d2c_fprintf(outfile,".%s",tmppcklist->key->name);
                    tmppcklist=tmppcklist->next;
                }
                d2c_fputs(";\n\n", outfile);
            }

            /* We generate the import clauses */
            used_classes = list_classes(tmplist, b);
            while (used_classes != NULL)
            {
                tmppcklist = make_package_list(used_classes->key->package);
                if ( tmppcklist != NULL )
                {
                    if ( strcmp(tmppcklist->key->id,class->package->id))
                    {
                        /* This class' package and our current class' package are
                           not the same */
                        d2c_fprintf(outfile,"import %s",tmppcklist->key->name);
                        tmppcklist=tmppcklist->next;
                        while ( tmppcklist != NULL )
                        {
                            d2c_fprintf(outfile, ".%s", tmppcklist->key->name);
                            tmppcklist=tmppcklist->next;
                        }
                        d2c_fprintf(outfile,".%s;\n",used_classes->key->name);
                    }
                }
                else
                {
                    /* No info for this class' package, we include it directly */
                    d2c_fprintf(outfile, "import %s;\n",used_classes->key->name);
                }
                used_classes = used_classes->next;
            }

            d2c_fprintf(outfile, "\npublic ");

            tmpname = strtolower(class->stereotype);
            if (eq("interface", tmpname))
                classtype = CLASSTYPE_INTERFACE;
            else
            {
                if (class->isabstract)
                    classtype = CLASSTYPE_ABSTRACT;
                else
                    classtype = CLASSTYPE_CLASS;
            }
            free(tmpname);

            switch(classtype)
            {
            case CLASSTYPE_INTERFACE:   d2c_fprintf(outfile, "interface "); break;
            case CLASSTYPE_ABSTRACT:    d2c_fprintf(outfile, "abstract class "); break;
            case CLASSTYPE_CLASS:       d2c_fprintf(outfile, "class "); break;
            }

            d2c_fprintf(outfile, "%s", class->name);

            if (java_manage_parents(outfile, tmplist->parents, JAVA_EXTENDS) == 0)
            {
                d2c_fprintf(outfile, "\n");
            }
            java_manage_parents(outfile, tmplist->parents, JAVA_IMPLEMENTS);

            /* At this point we need to make a decision:
               If you want to implement flexibility to add "extends", then
               the brace must be on the next line. */
            d2c_open_brace(outfile, "");

            d2c_shift_code();
            umla = class->attributes;

            if( umla != NULL)
                d2c_fprintf(outfile, "/** Attributes */\n");

            while ( umla != NULL)
            {
                java_generate_attribute(outfile, &umla->key);
                umla = umla->next;
            }

            associations = tmplist->associations;
            if (associations != NULL)
                d2c_fprintf(outfile, "/** Associations */\n");

            while ( associations != NULL )
            {
                d2c_fprintf(outfile, "private %s %s;\n", associations->key->name, associations->name);
                associations = associations->next;
            }

            /* Operations */
            umlo = class->operations;
            while ( umlo != NULL)
            {
                java_generate_operation( outfile, &umlo->key, classtype );
                umlo = umlo->next;
            }

            d2c_unshift_code();
            d2c_close_brace(outfile, "\n");

            fclose(outfile);
        }
        tmplist = tmplist->next;
    }
}

