/*********************************************1******************************
                          dia2code.h  -  Global types and functions
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

 #ifndef DIA2CODE_H
 #define DIA2CODE_H

#include <string.h>
#include <ctype.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

/* libxml[2] headers */
#include <libxml/parser.h>
#include <libxml/tree.h>

#define VERSION "0.8.3"

#ifdef DSO
#include <dlfcn.h>
#define MODULE_DIR ".dia2code"
#define DSO_PREFIX "generate_code_"
#define DSO_SUFFIX ".so"
#endif

#define kind_str(A)   ((A)=='1'?"in":((A)=='2'?"in/out":((A)=='3'?"out":"???")))

struct umlattribute {
    char name[80];
    char value[80];
    char type [80];
    char comment [80];
    char visibility;
    char isabstract;
    char isstatic;
    char kind;
};
typedef struct umlattribute umlattribute;

struct umlattrnode {
    umlattribute key;
    struct umlattrnode *next;
};

typedef struct umlattrnode umlattrnode;
typedef umlattrnode *umlattrlist;

struct umloperation{
    umlattribute attr;
    umlattrlist parameters;
    char *implementation;
};
typedef struct umloperation umloperation;

struct umlopnode {
    umloperation key;
    struct umlopnode *next;
};

struct umltemplate{
	char name[80];
	char type[80];
};
typedef struct umltemplate umltemplate;

typedef struct umlopnode umlopnode;
typedef umlopnode *umloplist;

struct umltemplatenode {
		umltemplate key;
		struct umltemplatenode *next;
};

typedef struct umltemplatenode umltemplatenode;
typedef umltemplatenode *umltemplatelist;

struct geometry {
    float pos_x;
    float pos_y;
    float width;
    float height;
};
typedef struct geometry geometry;

struct umlpackage {
    char id[80];
    char name[80];
    geometry geom;
    struct umlpackage * parent;
};
typedef struct umlpackage umlpackage;

struct umlpackagenode {
    umlpackage * key;
    struct umlpackagenode * next;
};
typedef struct umlpackagenode umlpackagenode;
typedef umlpackagenode * umlpackagelist;

struct umlclass {
    char id[80];
    char name[80];
    char stereotype[80];
    char comment[80];
    int isabstract;
    umlattrlist attributes;
    umloplist operations;
    umltemplatelist templates;
    umlpackage * package;
    geometry geom;
};
typedef struct umlclass umlclass;

struct umlassocnode {
	umlclass * key;
	char name[80];
	char composite;
	struct umlassocnode * next;
};

struct  umlclassnode {
    umlclass * key;
    struct umlclassnode * parents;
    struct umlassocnode * associations;
    struct umlclassnode * dependencies;
    struct umlclassnode * next;
};
typedef struct umlassocnode umlassocnode;
typedef umlassocnode * umlassoclist;

typedef struct  umlclassnode umlclassnode;
typedef umlclassnode * umlclasslist;

struct namenode {
   char *name;
   struct namenode *next;
};

typedef struct namenode namenode;
typedef namenode * namelist;


struct batch {
    umlclasslist classlist;  // The classes in the diagram
    char *outdir;  // Output directory
    int clobber;  // Overwrite files in directory
    namelist classes;  // Selection of classes to generate code for
    int mask;  // Flag that inverts the above selection
    char *license; // License file
};
typedef struct batch batch;

char *strtoupper(char *s);
char *strtolower(char *s);
char *strtoupperfirst(char *s);
namelist parse_class_names(const char *s);
int is_present(namelist list, const char *name);
namelist find_classes(umlclasslist current_class, batch *b);
umlclasslist find_by_name(umlclasslist list, const char * name);

int is_enum_stereo (char * stereo);

umlattrlist copy_attributes(umlattrlist src);

void * my_malloc( size_t size );

umlpackagelist make_package_list( umlpackage * package);

umlclasslist list_classes(umlclasslist current_class, batch *b);

extern char *file_ext;       /* Set by switch "-ext". Language specific
                                default applies when NULL.  */
extern char *body_file_ext;  /* Set by switch "-bext". Language specific
                                default applies when NULL.  */

extern int indentlevel;
void set_number_of_spaces_for_one_indentation(int n);  /* default: 2 spaces */
char *spc();
/* Returns a string consisting of (indentlevel *
   number_of_spaces_for_one_indentation) spaces.  */

/**
 * Output target files:
 * All backends would use `spec', but not all backends require `body'.
 * Hence not all backends need the {e,p}{body,both} functions below.
*/
extern FILE *spec, *body;

/* Emitters for file output  */
void emit  (char *msg, ...);  /* print verbatim to spec */
void ebody (char *msg, ...);  /* print verbatim to body (e stands for emit) */
void eboth (char *msg, ...);  /* print verbatim to both spec and body */
void print (char *msg, ...);  /* print with leading indentation to spec */
void pbody (char *msg, ...);  /* print with leading indentation to body */
void pboth (char *msg, ...);  /* print with leading indentation to both */

/**
 * open_outfile() returns NULL if the file exists and is not rewritten
 * due to a clobber prohibition. Does an exit(1) if serious problems happen.
*/
FILE * open_outfile (char *filename, batch *b);

// Added by RK 2003-02-20
#define HUGE_BUFFER 8192
#define LARGE_BUFFER 1024
#define BIG_BUFFER 255
#define SMALL_BUFFER 80

#define NEW_AUTO_INDENT 1
#ifdef NEW_AUTO_INDENT
#define d2c_fprintf _d2c_fprintf
#define d2c_fputs _d2c_fputs
#define d2c_fputc _d2c_fputc
#else
#define d2c_fprintf fprintf
#define d2c_fputs fputs
#define d2c_fputc fputc
#endif

void d2c_indent();
void d2c_outdent();
int _d2c_fputs(const char *s, FILE *f);
int _d2c_fputc(int c, FILE *f);
int _d2c_fprintf(FILE *f, char *fmt, ...);
//void d2c_set_braces(char *open, char *close);
void d2c_open_brace(FILE *outfile, char *suffix);
void d2c_close_brace(FILE *outfile, char *suffix);
void d2c_parse_impl(FILE *f, char *cmt_start, char *cmt_end);
void d2c_dump_impl(FILE *f, char *section, char *name);
void d2c_deprecate_impl(FILE *f, char *comment_start, char *comment_end);
char *d2c_operation_mangle_name(umlopnode *op);
int d2c_backup(char *filename);

#define TAG fprintf(stderr, "%s %d\n", __FILE__, __LINE__);
#define eq  !strcmp

struct param_list
{
  char *name;
  char *value;
  struct param_list *next;
};

typedef struct param_list param_list;

void param_list_destroy();
param_list * d2c_parameter_add(char *name, char *value);
param_list * d2c_parameter_set(char *name, char *value);
char * d2c_parameter_value(char *name);
param_list *d2c_parameter_find(char *name);

int indent_count;
int indent_open_brace_on_newline;
int generate_backup;

#endif
