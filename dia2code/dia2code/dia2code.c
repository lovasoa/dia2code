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

