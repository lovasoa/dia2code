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

// Other things to be fixed:
// The code that determines the parent class and implementations needs to go in order from "extends" to "implements".

#define JAVA_EXTENDS 0
#define JAVA_IMPLEMENTS 1

char *dia_visibility_to_string(int visibility)
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

// This function dumps out the list of interfaces and extensions, as necessary.
//
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

void generate_code_java(batch *b)
{
    umlclasslist tmplist, parents;
    umlassoclist associations;
    umlattrlist umla, tmpa;
    umlpackagelist tmppcklist;
    umloplist umlo;
    char *tmpname;
    char outfilename[90];
    FILE * outfile, *dummyfile, *licensefile = NULL;
    int file_found = 0;
    umlclasslist used_classes;

    #define CLASSTYPE_CLASS 0
    #define CLASSTYPE_ABSTRACT 1
    #define CLASSTYPE_INTERFACE 2

    int classtype;

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
        if ( ! ( is_present(b->classes, tmplist->key->name) ^ b->mask ) )
        {
            tmpname = tmplist->key->name;

            /* This prevents buffer overflows */
            tmpfilelgth = strlen(tmpname);
            if (tmpfilelgth + tmpdirlgth > sizeof(*outfilename) - 2)
            {
                fprintf(stderr, "Sorry, name of file too long ...\nTry a smaller dir name\n");
                exit(4);
            }

            sprintf(outfilename, "%s/%s.java", b->outdir, tmplist->key->name);

            dummyfile = fopen(outfilename, "r");
            file_found = dummyfile != NULL;
            if (file_found)
            {
                d2c_parse_impl(dummyfile, "//", "");
                close(dummyfile);
                if (d2c_backup(outfilename))
                    exit(5);
            }

            if ( b->clobber || ! file_found )
            {
                outfile = fopen(outfilename, "w");
                if ( outfile == NULL )
                {
                    fprintf(stderr, "Can't open file %s for writing\n", outfilename);
                    exit(3);
                }

                d2c_dump_impl(outfile, "opening", "");

                /* add license to the header */
                if(b->license != NULL)
                {
                    char lc;
                    rewind(licensefile);
                    while((lc = fgetc(licensefile)) != EOF)
                        d2c_fputc(lc, outfile);
                }

                tmppcklist = make_package_list(tmplist->key->package);
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
                        if ( strcmp(tmppcklist->key->id,tmplist->key->package->id))
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

                d2c_dump_impl(outfile, "import", "");

                d2c_fprintf(outfile, "\npublic ");

                tmpname = strtolower(tmplist->key->stereotype);
                if (eq("interface", tmpname) )
                    classtype = CLASSTYPE_INTERFACE;
                else
                {
                    if (tmplist->key->isabstract)
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

                d2c_fprintf(outfile, "%s", tmplist->key->name);

                if (java_manage_parents(outfile, tmplist->parents, JAVA_EXTENDS) == 0)
                {
                    d2c_fprintf(outfile, "\n");
                    d2c_dump_impl(outfile, "extends", "");
                }
                java_manage_parents(outfile, tmplist->parents, JAVA_IMPLEMENTS);

                // At this point we need to make a decision:
                // If you want to implement flexibility to add "extends", then the brace must be on the next line.
                if (1) // if (dumping_implementations)
                    d2c_fprintf(outfile, "\n");
                d2c_dump_impl(outfile, "inheritence", "");

                d2c_open_brace(outfile, "");

                umla = tmplist->key->attributes;

                if( umla != NULL)
                    d2c_fprintf(outfile, "/** Attributes */\n");

                while ( umla != NULL)
                {
					d2c_fprintf(outfile, "%s ", dia_visibility_to_string(umla->key.visibility));

                    if (umla->key.isstatic)
                        d2c_fprintf(outfile, "static ");
                    d2c_fprintf(outfile, "%s %s", umla->key.type, umla->key.name);
                    if ( umla->key.value[0] != 0 )
                        d2c_fprintf(outfile, " = %s", umla->key.value);
                    d2c_fprintf(outfile, ";\n");
                    umla = umla->next;
                }

                if (classtype != CLASSTYPE_INTERFACE)
                    d2c_dump_impl(outfile, "attributes", "");

                associations = tmplist->associations;
	            if (associations != NULL)
	                d2c_fprintf(outfile, "/** Associations */\n");

                while ( associations != NULL )
                {
                    d2c_fprintf(outfile, "private %s %s;\n", associations->key->name, associations->name);
                    associations = associations->next;
                }

                d2c_dump_impl(outfile, "associations", "");

				// Operations here
				//
                umlo = tmplist->key->operations;
                while ( umlo != NULL)
                {
                    d2c_fprintf(outfile, "/**\n");
                    d2c_fprintf(outfile, " * Operation\n");
                    d2c_fprintf(outfile, " *\n");
                    tmpa = umlo->key.parameters;
                    while (tmpa != NULL)
                    {
                        d2c_fprintf(outfile, " * @param %s\n", tmpa->key.name);
                        tmpa = tmpa->next;
                    }
                    if(strcmp(umlo->key.attr.type, "void"))
                        d2c_fprintf(outfile, " * @return %s\n", umlo->key.attr.type);
                    d2c_fprintf(outfile, " */\n");
                    //d2c_fprintf(outfile, "  ");

                    if ( umlo->key.attr.isabstract )
                    {
                        d2c_fprintf(outfile, "abstract ");
                        umlo->key.attr.value[0] = '0';
                    }

					d2c_fprintf(outfile, "%s ", dia_visibility_to_string(umlo->key.attr.visibility));

                    if ( umlo->key.attr.isstatic )
                        d2c_fprintf(outfile, "static ");

                    if (strlen(umlo->key.attr.type) > 0)
                        d2c_fprintf(outfile, "%s ", umlo->key.attr.type);

                    d2c_fprintf(outfile, "%s ( ", umlo->key.attr.name);
                    tmpa = umlo->key.parameters;
                    while (tmpa != NULL)
                    {
                        d2c_fprintf(outfile, "%s %s", tmpa->key.type, tmpa->key.name);
                        /*
                        if ( tmpa->key.value[0] != 0 ){
                            d2c_fprintf(outfile," = %s",tmpa->key.value);
                        }  */
                        tmpa = tmpa->next;
                        if (tmpa != NULL)
                            d2c_fprintf(outfile, ", ");
                    }
                    d2c_fprintf(outfile, " )");
                    // RK - Not sure this is right but I did it. This
                    //      prevents curly braces from being applied when CLASSTYPE is interface.
                    if ( umlo->key.attr.isabstract || classtype == CLASSTYPE_INTERFACE)
                        d2c_fprintf(outfile, ";\n");
                    else
                    {
                        d2c_open_brace(outfile, "");
                        d2c_dump_impl(outfile, "method", d2c_operation_mangle_name(umlo));
                        if ( umlo->key.implementation != NULL )
                            d2c_fprintf(outfile, "%s\n", umlo->key.implementation);
                        d2c_close_brace(outfile, "");
                    }
                    umlo = umlo->next;
                }
                if (classtype != CLASSTYPE_INTERFACE)
                    d2c_dump_impl(outfile, "other.operations", "");

                d2c_close_brace(outfile, "\n");
                d2c_dump_impl(outfile, "closing", "");

                d2c_deprecate_impl(outfile, "//", "");

                fclose(outfile);
            }
        }
        tmplist = tmplist->next;
    }
}

