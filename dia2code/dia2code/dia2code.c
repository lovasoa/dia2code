/***************************************************************************
                          dia2code.c  -  Global functions
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

/**
 * This function returns the upper case char* of the one taken on input
 * The char * received may be freed by the caller
*/
char *strtoupper(char *s) {
    char *tmp = strdup(s);
    int i, n;
    if (tmp == NULL) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    n = strlen(tmp);
    for (i = 0; i < n; i++) {
        tmp[i] = toupper(tmp[i]);
    }
    return tmp;
}

/**
  * This function returns the lower case char* of the one taken on input
  * The char * received may be freed by the caller
*/
char *strtolower(char *s) {
    char *tmp = strdup(s);
    int i, n;
    if (tmp == NULL) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    n = strlen(tmp);
    for (i = 0; i < n; i++) {
        tmp[i] = tolower(tmp[i]);
    }
    return tmp;
}

/**
  * This function returns the a char* that has the first
  * character in upper case and the rest unchanged.
  * The char * received may be freed by the caller
*/
char *strtoupperfirst(char *s) {
    char *tmp = strdup(s);
    int i, n;
    if (tmp == NULL) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    n = strlen(tmp);
    tmp[0] = toupper(tmp[0]);
    for (i = 1; i < n; i++) {
        tmp[i] = tmp[i];
    }
    return tmp;
}


namelist parse_class_names(const char *s) {
    char *cp, *token;
    const char *delim = ",";
    namelist list = NULL, tmp;

    cp = strdup(s);
    if (cp == NULL) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    token = strtok (cp, delim);
    while ( token != NULL ) {
        tmp = (namelist) malloc (sizeof(namenode));
        if (tmp == NULL) {
            fprintf(stderr, "Out of memory\n");
            exit(1);
        }
        tmp->name = strdup(token);
        if (tmp->name == NULL) {
            fprintf(stderr, "Out of memory\n");
            exit(1);
        }
        tmp->next = list;
        list = tmp;
        token = strtok (NULL, delim);
    }
    free(cp);
    return list;
}

int is_present(namelist list, const char *name) {
    while (list != NULL) {
        int len;
        char* mask;
        if ( ! strcmp(list->name, name) ) {
            return 1;
        }
        if ( (len = strlen(list->name)) && (2 <= len <= strlen(name))
                && (mask = strchr(list->name, '*')) != NULL
                && mask == strrchr(list->name, '*') ) {
            len--;
            if ( mask == list->name && ! strcmp(list->name+1, name+strlen(name)-len) ) {
                return 1;
            }
            if ( mask == list->name+len && ! strncmp(list->name, name, len) ) {
                return 1;
            }
        }
        list = list->next;
    }
    return 0;
}

void * my_malloc( size_t size ) {
    void * tmp;
    tmp = malloc(size);
    if (tmp == NULL) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    return tmp;
}


/*
    Builds a package list from the hierarchy of parents of package.
    The topmost package will be the first on the list and the initial
    package will be the last.
*/
umlpackagelist make_package_list(umlpackage * package){
    umlpackagelist dummylist,tmplist=NULL;

    while ( package != NULL ){
        dummylist = (umlpackagelist) my_malloc(sizeof(umlpackagenode));
        dummylist->next = tmplist;
        tmplist = dummylist;
        tmplist->key = package;
        package = package -> parent;
    }
    return tmplist;
}

int indentlevel = 0;
static int number_of_spaces_for_one_indentation = 2;

void set_number_of_spaces_for_one_indentation(int n)
{
    number_of_spaces_for_one_indentation = n;
}

char *spc()
{
   static char spcbuf[132];
   int n_spaces = number_of_spaces_for_one_indentation * indentlevel;
   if (n_spaces >= sizeof(spcbuf)) {
       fprintf (stderr, "spc(): spaces buffer overflow\n");
       exit (1);
   }
   memset (spcbuf, ' ', n_spaces);
   spcbuf[n_spaces] = '\0';
   return spcbuf;
}

FILE *spec = NULL, *body = NULL;

/* Auxiliary define for the emit/print functions  */
#define var_arg_to_str(first_arg) \
    va_list vargu; \
    char str[4096]; \
    va_start (vargu, first_arg); \
    vsnprintf (str, 4096, first_arg, vargu); \
    va_end (vargu)

void emit (char *msg, ...)
{
    var_arg_to_str (msg);
    fputs (str, spec);
}

void ebody (char *msg, ...)
{
    var_arg_to_str (msg);
    if (body != NULL)
        fputs (str, body);
}

void eboth (char *msg, ...)
{
    var_arg_to_str (msg);
    fputs (str, spec);
    if (body != NULL)
        fputs (str, body);
}


void print (char *msg, ...)
{
    var_arg_to_str (msg);
    fprintf (spec, "%s%s", spc(), str);
}

void pbody (char *msg, ...)
{
    var_arg_to_str (msg);
    if (body != NULL)
        fprintf (body, "%s%s", spc(), str);
}

void pboth (char *msg, ...)
{
    var_arg_to_str (msg);
    fprintf (spec, "%s%s", spc(), str);
    if (body != NULL)
        fprintf (body, "%s%s", spc(), str);
}

char *file_ext = NULL;
char *body_file_ext = NULL;

FILE * open_outfile (char *filename, batch *b)
{
    static char outfilename[512];
    FILE *o;
    int tmpdirlgth, tmpfilelgth;

    if (b->outdir == NULL) {
        b->outdir = ".";
    }

    tmpdirlgth = strlen (b->outdir);
    tmpfilelgth = strlen (filename);

    /* This prevents buffer overflows */
    if (tmpfilelgth + tmpdirlgth > sizeof(outfilename) - 2) {
        fprintf (stderr, "Sorry, name of file too long ...\n"
                    "Try a smaller dir name\n");
        exit (1);
    }

    sprintf (outfilename, "%s/%s", b->outdir, filename);
    o = fopen (outfilename, "r");
    if (o != NULL && !b->clobber) {
        fclose (o);
        return NULL;
    }
    o = fopen (outfilename, "w");
    if (o == NULL) {
        fprintf (stderr, "Can't open file %s for writing\n", outfilename);
        exit (1);
    }
    return o;
}


int
is_enum_stereo (char *stereo)
{
    return (!strcmp (stereo, "CORBAEnum") ||
            !strcmp (stereo, "Enumeration") ||
            !strcmp (stereo, "enumeration"));
}

