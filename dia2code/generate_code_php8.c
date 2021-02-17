/*
 * generate_code_php8.c  -  Function that generates PHP 8 code
 *                             -------------------
 *    begin                : 2021-02-15 by Åsmund Stavdahl
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "dia2code.h"

#define TABS "    " /* 4 */

/**
 * return the string declaring the class type
 */
char *d2c_php8_class_type(umlclasslist classList)
{
    char *tmpname = strtolower(classList->key->stereotype);
    if (eq("interface", tmpname))
    {
        return "interface";
    }
    else
    {
        if (classList->key->isabstract)
        {
            return "abstract class";
        }
        else
        {
            return "class";
        }
    }
}

char *d2c_php8_visibility(char vis)
{
    switch (vis)
    {
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

/**
 * php8 has flexible type hinting, so we will always type arguments
 */
int d2c_php8_show_arg(char *arg)
{
    return 1;
}

/**
 * write comment block for a function
 */
void d2c_php8_print_func_comments(FILE *outfile, umloplist umlo)
{
    if (umlo->key.attr.comment[0] != 0)
    {
        fprintf(outfile, "%s/**\n", TABS);
        fprintf(outfile, "%s * %s\n", TABS, umlo->key.attr.comment);
        fprintf(outfile, "%s */\n", TABS);
    }
}

/**
 * write the function body
 */
void d2c_php8_print_func_code(FILE *outfile, umloplist umlo, umlclasslist classList)
{
    char *visibility = d2c_php8_visibility(umlo->key.attr.visibility);
    fprintf(outfile, TABS);
    fprintf(outfile, "%s%s %sfunction %s(",
            (umlo->key.attr.isabstract) ? "abstract " : "",
            visibility,
            (umlo->key.attr.isstatic) ? "static " : "",
            umlo->key.attr.name);

    // START parameter list
    umlattrlist param = umlo->key.parameters;
    while (param != NULL)
    {
        // type hinting of parameter
        if (d2c_php8_show_arg(param->key.type))
        {
            fprintf(outfile, "%s ", param->key.type);
        }
        // name of parameter
        fprintf(outfile, "$%s", param->key.name);
        // default value of parameter
        if (param->key.value[0] != 0)
        {
            fprintf(outfile, " = %s", param->key.value);
        }
        // proceed with next parameter, if any
        param = param->next;
        if (param != NULL)
            fprintf(outfile, ", ");
    }
    // END parameter list
    fprintf(outfile, ")");

    // return type hinting
    if (umlo->key.attr.type[0] != 0)
    {
        fprintf(outfile, ": %s", umlo->key.attr.type);
    }

    // implementation scaffold
    if (!umlo->key.attr.isabstract && !(eq(classList->key->stereotype, "interface")))
    {
        fprintf(outfile, "\n%s{\n", TABS);
        if (umlo->key.implementation != NULL)
        {
            fprintf(outfile, "%s\n", umlo->key.implementation);
        }
        else if (!umlo->key.attr.isabstract)
        {
            fprintf(outfile,
                    "%s%sthrow new Error('%s() not implemented');\n",
                    TABS, TABS, umlo->key.attr.name);
        }
        fprintf(outfile, "%s}\n", TABS);
    }
    else
    {
        // don't print an empty body for abstract methods
        fprintf(outfile, ";\n");
    }
}

/**
 * print out classes associated with this class
 */
int d2c_php8_print_associations(FILE *outfile, umlclasslist classList)
{
    umlassoclist associations = classList->associations;
    while (associations != NULL)
    {
        fprintf(outfile, "%s/**\n", TABS);
        fprintf(outfile, "%s * XXX\n", TABS);
        fprintf(outfile, "%s *\n", TABS);
        fprintf(outfile, "%s * @var    %s $%s\n",
                TABS, associations->key->name, associations->name);
        fprintf(outfile, "%s * @access private\n", TABS);
        fprintf(outfile, "%s * @accociation %s to %s\n",
                TABS, associations->key->name, associations->name);
        fprintf(outfile, "%s */\n", TABS);
        fprintf(outfile, "%s#var $%s;\n\n",
                TABS, associations->name);
        associations = associations->next;
    }
    return 0;
}
/**
 * print out this classes atributes with appropriate comment blocks
 */
int d2c_php8_print_attributes(FILE *outfile, umlclasslist classList)
{
    umlattrlist umla = classList->key->attributes;
    while (umla != NULL)
    {
        // document the attribute
        if (umla->key.comment[0] != 0)
        {
            fprintf(outfile, "%s/**\n", TABS);
            fprintf(outfile, "%s * %s\n", TABS, umla->key.comment);
            fprintf(outfile, "%s */\n", TABS);
        }

        // print the actual variable declaration
        char *visibility = d2c_php8_visibility(umla->key.visibility);
        char *staticity = umla->key.isstatic ? "static " : "";
        char *type = umla->key.type[0] != 0 ? umla->key.type : "";
        char *name = umla->key.name;

        fprintf(outfile, "%s%s %s%s%s$%s", TABS, visibility, staticity, type, type[0] == 0 ? "" : " ", name);
        if (umla->key.value[0] != 0)
        {
            fprintf(outfile, " = %s", umla->key.value);
        }
        fprintf(outfile, ";\n\n");
        umla = umla->next;
    }
    return 0;
}

/**
 * print class methods
 */
void d2c_php8_print_operations(FILE *outfile, umlclasslist classList)
{
    umloplist umlo = classList->key->operations;
    while (umlo != NULL)
    {
        // print each function
        d2c_php8_print_func_comments(outfile, umlo);
        d2c_php8_print_func_code(outfile, umlo, classList);
        umlo = umlo->next;
    }
}

int d2c_php8_print_license(FILE *outfile, FILE *licensefile)
{
    char lc;
    rewind(licensefile);
    while ((lc = fgetc(licensefile)) != EOF)
    {
        fprintf(outfile, "%c", lc);
    }
    return 0;
}

int d2c_php8_print_includes(FILE *outfile, umlclasslist classList, batch *b)
{
    /* We generate the include clauses */
    umlpackagelist tmppcklist;
    umlclasslist used_classes = list_classes(classList, b);
    int noIncludes = 1;
    while (used_classes != NULL)
    {
        tmppcklist = make_package_list(used_classes->key->package);
        if (tmppcklist != NULL)
        {
            if (strcmp(tmppcklist->key->id, classList->key->package->id))
            {
                /* This class' package and our current class' package are
            not the same */
                fprintf(outfile, "require_once '");
                fprintf(outfile, "%s", tmppcklist->key->name);
                tmppcklist = tmppcklist->next;
                while (tmppcklist != NULL)
                {
                    fprintf(outfile, "/%s", tmppcklist->key->name);
                    tmppcklist = tmppcklist->next;
                }
                fprintf(outfile, "/");
                fprintf(outfile, "%s.php';\n", used_classes->key->name);
                noIncludes = 0;
            }
        }
        else
        {
            /* XXX - If the used class is different from the
        actual class, we include it. I don't know
        if this is ok. */
            if (strcmp(used_classes->key->name, classList->key->name))
            {
                fprintf(outfile, "require_once '%s.php';\n",
                        used_classes->key->name);
                noIncludes = 0;
            }
        }
        used_classes = used_classes->next;
    }
    if (!noIncludes)
    {
        fprintf(outfile, "\n");
    }

    return 0;
}

int d2c_php8_print_class_desc(FILE *outfile, umlclasslist classList)
{
    if (classList->key->comment[0] != 0)
    {
        fprintf(outfile, "/**\n");
        fprintf(outfile, " * %s\n", classList->key->comment);
        fprintf(outfile, " */\n");
    }
    return 0;
}

/**
 * print out the first class declaration line
 */

int d2c_php8_print_class_decl(FILE *outfile, umlclasslist classList)
{
    char *tmpname;
    umlclasslist parents;
    tmpname = d2c_php8_class_type(classList);
    // print class 'type' and name
    fprintf(outfile, "%s %s", tmpname, classList->key->name);

    int hasImplementedSomething = 0;
    int hasExtendedSomething = 0;

    parents = classList->parents;
    if (parents != NULL)
    {
        while (parents != NULL)
        {
            tmpname = strtolower(parents->key->stereotype);
            if (eq(tmpname, "interface") && !(eq(classList->key->stereotype, "interface")))
            {
                if (!hasImplementedSomething)
                {
                    fprintf(outfile, " implements ");
                }
                else
                {
                    fprintf(outfile, ", ");
                }
                hasImplementedSomething = 1;
            }
            else
            {
                if (!hasExtendedSomething)
                {
                    fprintf(outfile, " extends ");
                }
                else
                {
                    fprintf(outfile, ", ");
                }
                hasExtendedSomething = 1;
            }
            free(tmpname);
            fprintf(outfile, "%s", parents->key->name);
            parents = parents->next;
        }
    }
    fprintf(outfile, "\n{\n");
    return 0;
}

/**
 * return an opened file handle, null for failure
 */
FILE *d2c_php8_getoutfile(umlclasslist classList, batch *b, FILE *outfile, int maxlen)
{
    char outfilename[maxlen + 2];
    int tmpfilelgth = strlen(classList->key->name);
    if (tmpfilelgth + strlen(b->outdir) > maxlen)
    {
        return NULL;
    }
    sprintf(outfilename, "%s/%s.php", b->outdir, classList->key->name);
    FILE *dummy = fopen(outfilename, "r");
    if (b->clobber || !dummy)
    {
        outfile = fopen(outfilename, "w");
    }
    fclose(dummy);
    return outfile;
}

/**
 * main function called to begin output
 */
void generate_code_php8(batch *b)
{
    umlclasslist classList;
    char *tmpname;
    char outfilename[90];
    FILE *outfile, *dummyfile, *licensefile = NULL;
    int tmpdirlgth, tmpfilelgth;

    if (b->outdir == NULL)
    {
        b->outdir = ".";
    }
    tmpdirlgth = strlen(b->outdir);

    classList = b->classlist;

    /* open license file */
    if (b->license != NULL)
    {
        licensefile = fopen(b->license, "r");
        if (!licensefile)
        {
            fprintf(stderr, "Can't open the license file.\n");
            exit(2);
        }
    }
    // for each class
    while (classList != NULL)
    {
        if (!(is_present(b->classes, classList->key->name) ^ b->mask))
        {
            tmpname = classList->key->name;
            /* This prevents buffer overflows */
            tmpfilelgth = strlen(tmpname);
            if (tmpfilelgth + tmpdirlgth > sizeof(*outfilename) - 2)
            {
                fprintf(stderr, "Sorry, name of file too long ...\nTry a smaller dir name\n");
                exit(4);
            }
            sprintf(outfilename, "%s/%s.php", b->outdir, classList->key->name);
            dummyfile = fopen(outfilename, "r");
            if (b->clobber || !dummyfile)
            {
                outfile = fopen(outfilename, "w");
                if (outfile == NULL)
                {
                    fprintf(stderr, "Can't open file %s for writing\n", outfilename);
                    exit(3);
                }

                fprintf(outfile, "<?php\n\ndeclare(strict_types=1);\n\n");
                /* header */
                if (b->license != NULL)
                {
                    /* add license to the header */
                    d2c_php8_print_license(outfile, licensefile);
                }
                d2c_php8_print_includes(outfile, classList, b);
                d2c_php8_print_class_desc(outfile, classList);
                d2c_php8_print_class_decl(outfile, classList);
                d2c_php8_print_attributes(outfile, classList);
                d2c_php8_print_associations(outfile, classList);
                d2c_php8_print_operations(outfile, classList);
                // end class declaration
                fprintf(outfile, "}\n");
                fclose(outfile);
            }
        }
        classList = classList->next;
    }
}
