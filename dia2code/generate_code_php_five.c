/***************************************************************************
generate_code_php5.c  -  Function that generates Php 5 code
                             -------------------
    begin                : Sat Dec 12 2001
    copyright            : (C) 2001 by Harald Fielker
    email                : fielker@softsolutions.de
    modified by          : Leandro Lucarella <luca@lugmen.org.ar>
    modified by          : tim rodger <tim.rodger@gmail.com>
    modified by          : Charles Schaefer <charlesschaefer@gmail.com>
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

/*
 * return the string declaring the class type
 */
char *d2c_php_class_type(umlclasslist tmplist)
{
    char* tmpname = strtolower(tmplist->key->stereotype);
    if (eq("interface", tmpname)) {
        return "interface";
    }
    else {
        if (tmplist->key->isabstract) {
            return "abstract class";
        }
        else {
            return "class";
        }
    }
}

char *d2c_php_visibility(char vis)
{
    switch (vis) {
        case '0':
            return "public";
        case '1':
            return "private";
        case '2':
            return "protected";
        default:
            return "public";
    }
}

/* 
 * php5 allows declaration of method arg types,
 * but only for non-built in types, return empty string for them
 */
int d2c_php_show_arg(char *arg)
{
/* convert to lower case ? */
    char *tmp = strtolower(arg);
    int result = 1;    
    if (eq("int", tmp)||
        eq("integer", tmp)||
        eq("bool", tmp) ||
        eq("boolean", tmp) ||
        eq("string", tmp) ||
        eq("float", tmp) ||
        eq("array", tmp)) {
        result = 0;
    }
    
    free(tmp);
    return result;
}

/**
 * write comment block for a function
 */
int d2c_php_print_func_comments(FILE *outfile, umloplist umlo)
{
    
    umlattrlist parama;
    char *tmpname;
    /* begin function comments */
    fprintf(outfile, "%s/**\n", TABS);
    if (strlen(umlo->key.attr.comment) > 0)
        fprintf(outfile, "%s * %s\n", TABS, umlo->key.attr.comment);
    else
        fprintf(outfile, "%s * XXX\n", TABS);
    fprintf(outfile, "%s * \n", TABS);

    parama = umlo->key.parameters;
    /* document parameters */
    while (parama != NULL) {
        fprintf(outfile, "%s * @param  %s $%s ",
                TABS, parama->key.type, parama->key.name);
        if (strlen(parama->key.comment) > 0)
          fprintf(outfile, "%s\n", parama->key.comment);
        else
            fprintf(outfile, "XXX\n");
        parama= parama->next;
    }
    
    if (strlen(umlo->key.attr.type) > 0) {
        fprintf(outfile, "%s * @return %s XXX\n",
                TABS, umlo->key.attr.type);
    }
    
    /*fprintf(outfile, "%s * @access ", TABS );
    tmpname = d2c_php_visibility(umlo->key.attr.visibility);
    fprintf(outfile, "%s\n", tmpname);*/
    
    if (umlo->key.attr.isabstract) {
        fprintf(outfile, "%s * @abstract\n", TABS );
        umlo->key.attr.value[0] = '0';
    }
    
    if (umlo->key.attr.isstatic) {
        fprintf(outfile, "%s * @static ", TABS);
    }
    
    fprintf(outfile, "%s */\n", TABS );
    return 0;
}

/*
 *    write the function body 
 */
int d2c_php_print_func_code(FILE *outfile, umloplist umlo)
{
    char *tmpname = d2c_php_visibility(umlo->key.attr.visibility);
    fprintf(outfile, TABS);
    fprintf(outfile, "%s%s %sfunction %s(",
                     (umlo->key.attr.isabstract) ? "abstract " : "",
                     tmpname,
                     (umlo->key.attr.isstatic) ? "static " : "",
                     umlo->key.attr.name);

    umlattrlist tmpa = umlo->key.parameters;
    while (tmpa != NULL) {
        if (d2c_php_show_arg(tmpa->key.type)) {
            fprintf(outfile, "%s ", tmpa->key.type);
        }
        fprintf(outfile, "$%s", tmpa->key.name);
        if (tmpa->key.value[0] != 0) {
            fprintf(outfile, " = %s", tmpa->key.value);
        }
        tmpa = tmpa->next;
        if (tmpa != NULL)
            fprintf(outfile, ", ");
    }
    fprintf(outfile, ")");
    if (!umlo->key.attr.isabstract) {
        fprintf(outfile, "\n%s {\n", TABS);
        if (umlo->key.implementation != NULL) {
            fprintf(outfile, "%s\n", umlo->key.implementation);
        } else if (!umlo->key.attr.isabstract) {
            fprintf(outfile,
                    "%s%strigger_error('Not Implemented!', E_USER_WARNING);\n",
                    TABS, TABS);
        }
        fprintf(outfile, "%s}\n\n", TABS);
    }
    else {
        /* don't print an empty body for abstract methods */
        fprintf(outfile, ";\n\n");
    }
    return 0;
}

/*
 * print out classes associated with this class
 * */

int d2c_php_print_associations(FILE *outfile, umlclasslist tmplist)
{
    umlassoclist associations = tmplist->associations;
    if (associations != NULL)
        fprintf(outfile, "%s// Associations\n", TABS);

    while ( associations != NULL ) {
        fprintf(outfile, "%s/**\n", TABS );
        fprintf(outfile, "%s * XXX\n", TABS );
        fprintf(outfile, "%s *\n", TABS );
        fprintf(outfile, "%s * @var    %s $%s\n",
                TABS, associations->key->name, associations->name);
        fprintf(outfile, "%s * @access private\n", TABS );
        fprintf(outfile, "%s * @accociation %s to %s\n",
                TABS, associations->key->name, associations->name);
        fprintf(outfile, "%s */\n", TABS );
        fprintf(outfile, "%s#var $%s;\n\n",
                TABS, associations->name);
        associations = associations->next;
    }
    return 0;
}
/*
 * print out this classes atributes with appropriate comment blocks
 */
int d2c_php_print_attributes(FILE *outfile, umlclasslist tmplist)
{
    umlattrlist umla = tmplist->key->attributes;
    if (umla != NULL)
        fprintf(outfile, "%s// Attributes\n", TABS);

    while (umla != NULL) {
        fprintf(outfile, "%s/**\n", TABS);
        if (strlen(umla->key.comment) > 0)
            fprintf(outfile, "%s * %s\n", TABS, umla->key.comment);
        else
            fprintf(outfile, "%s * XXX\n", TABS );
        fprintf(outfile, "%s *\n", TABS );
        fprintf(outfile, "%s * @var    %s\n", TABS, umla->key.type);
        //fprintf(outfile, "%s * @access ", TABS); 
    
        char *tmpname = d2c_php_visibility(umla->key.visibility);
        //fprintf(outfile, "%s\n", tmpname);*/
        if (umla->key.isstatic) {
            fprintf(outfile, "%s * @static", TABS);
        }
        fprintf(outfile, "%s */\n", TABS);
        /* print the actual variable declaration */
        fprintf(outfile, "%s%s %s$%s", TABS, tmpname,
                        (umla->key.isstatic) ? "static " : "",
                    umla->key.name);
        if (umla->key.value[0] != 0) {
            fprintf(outfile, " = %s", umla->key.value);
        }
        fprintf(outfile, ";\n\n");
        umla = umla->next;
   }
    return 0;
}

/*
 * print class methods
 */
int d2c_php_print_operations(FILE *outfile, umlclasslist tmplist)
{
    umloplist umlo = tmplist->key->operations;
    if (umlo != NULL)
        fprintf(outfile, "%s// Operations\n", TABS);

    while (umlo != NULL) {
        /* print each function */
        int result = d2c_php_print_func_comments(outfile, umlo);
        int r2 = d2c_php_print_func_code(outfile, umlo);
        umlo = umlo->next;
    }
    return 0;
}


int d2c_php_print_license(FILE *outfile, FILE *licensefile)
{
    char lc;
    rewind(licensefile);
    while ((lc = fgetc(licensefile)) != EOF) {
        fprintf(outfile, "%c", lc);
    }
    return 0;
}

int d2c_php_print_includes(FILE *outfile, umlclasslist tmplist, batch *b)
{
    /* We generate the include clauses */
    umlpackagelist tmppcklist;
    umlclasslist used_classes = list_classes(tmplist, b);
    while (used_classes != NULL) {
        tmppcklist = make_package_list(used_classes->key->package);
        if ( tmppcklist != NULL ) {
            if ( strcmp(tmppcklist->key->id, tmplist->key->package->id)) {
            /* This class' package and our current class' package are
            not the same */
                fprintf(outfile, "require_once '");
                fprintf(outfile, "%s", tmppcklist->key->name);
                tmppcklist=tmppcklist->next;
                while ( tmppcklist != NULL ) {
                    fprintf(outfile, "/%s", tmppcklist->key->name);
                    tmppcklist=tmppcklist->next;
                }
                fprintf(outfile, "/");
                fprintf(outfile, "%s.php';\n", used_classes->key->name);
            }
        } else {
        /* XXX - If the used class is different from the
        actual class, we include it. I don't know
        if this is ok. */
            if ( strcmp(used_classes->key->name, tmplist->key->name) ) {
                fprintf(outfile, "require_once '%s.php';\n",
                used_classes->key->name);
            }
        }
        used_classes = used_classes->next;
    }
    fprintf(outfile, "\n");
    return 0;
}

int d2c_php_print_class_desc(FILE *outfile, umlclasslist tmplist)
{
    umlpackagelist tmppcklist;
    fprintf(outfile, "/**\n" );
    
    if (strlen(tmplist->key->comment) > 0)
        fprintf(outfile, " * %s\n", tmplist->key->comment);
    else
        fprintf(outfile, " * XXX detailed description\n" );
    fprintf(outfile, " *\n" );
    fprintf(outfile, " * @author    XXX\n" );
    fprintf(outfile, " * @version   XXX\n" );
    fprintf(outfile, " * @copyright XXX\n" );
    
    tmppcklist = make_package_list(tmplist->key->package);
    if ( tmppcklist != NULL ) {
        int packcounter = 0;
        fprintf(outfile, " * @package   %s", tmppcklist->key->name);
        tmppcklist=tmppcklist->next;
        while (tmppcklist != NULL) {
            if( packcounter == 1 ) {
                fprintf(outfile, "\n" );
                fprintf(outfile, " * @subpackage %s", tmppcklist->key->name);
            } else {
                fprintf(outfile, ".%s", tmppcklist->key->name);
            }
            tmppcklist=tmppcklist->next;
            packcounter++;
        }
        fprintf(outfile, "\n");
    }
    
    if (tmplist->key->isabstract) {
        fprintf(outfile, " * @abstract\n" );
    }
    
    fprintf(outfile, " */\n" );
        return 0;
}

/*
 * print out the first class declaration line
 */

int d2c_php_print_class_decl(FILE *outfile, umlclasslist tmplist)
{
    char *tmpname;
    umlclasslist parents;
    tmpname = d2c_php_class_type(tmplist);
    /* print class 'type' and name */
    fprintf(outfile, "%s %s", tmpname, tmplist->key->name);
    
    parents = tmplist->parents;
    if (parents != NULL) {
        while (parents != NULL) {
            tmpname = strtolower(parents->key->stereotype);
            if (eq(tmpname, "interface")) {
                fprintf(outfile, " implements ");
            }    
            else {
                fprintf(outfile, " extends ");
            }
            free(tmpname);
            fprintf(outfile, "%s", parents->key->name);
            parents = parents->next;
        }
    }
    fprintf(outfile, " {\n");
    return 0;
}

/*
 * return an opened file handle, null for failure
 */
FILE *d2c_php_getoutfile(umlclasslist tmplist, batch *b, FILE *outfile, int maxlen)
{
    char outfilename[maxlen+2];
    int tmpfilelgth = strlen(tmplist->key->name);
    if (tmpfilelgth + strlen(b->outdir) > maxlen) {
        return NULL;
    }
    sprintf(outfilename, "%s/%s.php", b->outdir, tmplist->key->name);
    FILE *dummy = fopen(outfilename, "r");
    if (b->clobber || !dummy) {
        outfile = fopen(outfilename, "w");
    }
    fclose(dummy);
    return outfile;
}

/*
 * main function called to begin output
 * */
void generate_code_php_five(batch *b) 
{
    umlclasslist tmplist; 
    char *tmpname;
    char outfilename[BIG_BUFFER];
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
    /* for each class */
    while ( tmplist != NULL ) {
        if ( ! ( is_present(b->classes, tmplist->key->name) ^ b->mask ) ) {
            tmpname = tmplist->key->name;
            /* This prevents buffer overflows */
            tmpfilelgth = strlen(tmpname);
            if (tmpfilelgth + tmpdirlgth > sizeof(*outfilename) - 2) {
                fprintf(stderr, "Sorry, name of file too long ...\nTry a smaller dir name\n");
                exit(4);
            }
            sprintf(outfilename, "%s/%s.php", b->outdir, tmplist->key->name);
            dummyfile = fopen(outfilename, "r");
            if ( b->clobber || ! dummyfile ) {

                outfile = fopen(outfilename, "w");
                if ( outfile == NULL ) {
                    fprintf(stderr, "Can't open file %s for writing\n", outfilename);
                    exit(3);
                }

                fprintf(outfile, "<?php\n" );
                /* header */
                if(b->license != NULL) {
                    /* add license to the header */
                    d2c_php_print_license(outfile, licensefile);
                }
                d2c_php_print_includes(outfile, tmplist, b);
                d2c_php_print_class_desc(outfile, tmplist);
                d2c_php_print_class_decl(outfile, tmplist);
                d2c_php_print_attributes(outfile, tmplist);
                d2c_php_print_associations(outfile, tmplist);
                d2c_php_print_operations(outfile, tmplist);
                /* end class declaration */
                fprintf(outfile, "}\n\n");
                fprintf(outfile, "?>\n" );
                fclose(outfile);
            }
        }
        tmplist = tmplist->next;
    }
}

