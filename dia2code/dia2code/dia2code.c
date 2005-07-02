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
#include "errno.h"

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

umlattrlist copy_attributes(umlattrlist src)
{
    umlattrlist cpy = NULL, start = NULL;

    while (src != NULL)
    {
        umlattrlist tmp = (umlattrlist) my_malloc(sizeof(umlattrnode));
        tmp->key = src->key;
        if (cpy == NULL) {
            cpy = tmp;
            start = tmp;
        } else {
            cpy->next = tmp;
            cpy = tmp;
        }
        src = src->next;
    }
    if (cpy != NULL)
        cpy->next = NULL;

    return start;
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

// Added by RK 2003-02-20
// This should become part of the uml_class object.

struct endless_string_buf
{
    char *buf;
    struct endless_string_buf *next;
};
typedef struct endless_string_buf endless_string_buf;

struct endless_string
{
    endless_string_buf *start;
    endless_string_buf *end;
};
typedef struct endless_string endless_string;

void dump_endless_string(FILE *f, endless_string *es)
{
    endless_string_buf *esb = es->start;
    while (esb != NULL)
    {
        fprintf(f, "%s", esb->buf); // We do not d2c_fprintf the buffer, cause it's read in indented.
        esb = esb->next;
    }
}

endless_string * new_endless_string()
{
    endless_string *es = my_malloc(sizeof(endless_string));
    es->start = NULL;
    es->end = NULL;
}

void destroy_endless_string(endless_string * es)
{
    endless_string_buf *esb = es->start;
    endless_string_buf *esb_next;
    while (esb != NULL)
    {
        if (esb->buf != NULL)
            free(esb->buf);
        esb_next = esb->next;
        free(esb);
        esb = esb_next;
    }
    free (es);
}

void append_endless_string(endless_string * es, char *s)
{
    endless_string_buf *esb = my_malloc(sizeof(endless_string_buf));
    esb->buf = strdup(s);
    esb->next = NULL;
    if (es->start == NULL)
        es->start = esb;
    if (es->end != NULL)
        es->end->next = esb;
    es->end = esb;
}

struct d2c_impl{
    char name[128];
    endless_string *impl;
    int impl_len;
    int in_source;
    int in_class;
    struct d2c_impl *next;
};

typedef struct d2c_impl d2c_impl;
d2c_impl *d2c_impl_list = NULL;

void d2c_impl_list_destroy()
{
    d2c_impl *p = d2c_impl_list;
    while (p != NULL)
    {
        destroy_endless_string(p->impl);
        d2c_impl_list = p;
        p = p->next;
        free(d2c_impl_list);
    }
    d2c_impl_list = NULL;
}

d2c_impl * d2c_impl_add(char *name)
{
    d2c_impl *d2ci;
    d2ci = malloc(sizeof(d2c_impl));
    strcpy(d2ci->name, name);
    d2ci->impl = new_endless_string();
    d2ci->impl_len = 0;
    d2ci->in_source = 0;
    d2ci->in_class = 0;
    d2ci->next = d2c_impl_list;
    d2c_impl_list = d2ci;

    return d2ci;
}

d2c_impl* d2c_impl_find(char *name)
{
    d2c_impl *p = d2c_impl_list;
    while (p != NULL)
    {
        if (strcmp(p->name, name) == 0)
            return p;
        p = p->next;
    }
    return NULL;
}

d2c_impl *d2c_impl_find_or_add(char *name)
{
    d2c_impl *p = d2c_impl_find(name);
    if (p == NULL)
        p = d2c_impl_add(name);

    return p;
}

#define IMPLEMENTATION "Implementation"

void d2c_impl_comment(FILE *f, char *nm, char *range, int preserve, char *comment_start, char *comment_end)
{
    d2c_fprintf(f, "%s ## %s %spreserve %s class %s %s\n",
                   comment_start, IMPLEMENTATION, preserve?"":"no", range, nm, comment_end);
}

void d2c_dump_impl(FILE *f, char *section, char *name)
{
    char nm[LARGE_BUFFER];
    d2c_impl *d2ci;

    sprintf(nm, "%s.%s", section, name);
    d2ci = d2c_impl_find_or_add(nm);

    d2ci->in_class = 1;

    d2c_impl_comment(f, nm, "start", 1, "//", "");
    dump_endless_string(f, d2ci->impl);
    d2c_impl_comment(f, nm, "end", 1, "//", "");
}

void d2c_deprecate_impl(FILE *f, char *comment_start, char *comment_end)
{
    d2c_impl *p = d2c_impl_list;
    int cnt = 0;
    endless_string_buf *esb;

    while (p != NULL)
    {
        if (p->in_class == 0 && p->in_source == 1)
        {
            fprintf(stderr, "Deprecating implementation %s\n", p->name);

            if (cnt == 0)
            {
                d2c_fprintf(f, "\n%s WARNING: %s\n", comment_start, comment_end);
                d2c_fprintf(f, "%s The following code blocks will be deleted the next time code is generated. %s\n\n", comment_start, comment_end);
            }
            d2c_impl_comment(f, p->name, "start", 0, comment_start, comment_end);
            esb = p->impl->start;
            while (esb != NULL)
            {
                 // We do not d2c_fprintf the buffer, cause it's read in indented.
                fprintf(f, "// %s", esb->buf);
                esb = esb->next;
            }

            d2c_impl_comment(f, p->name, "end", 0, comment_start, comment_end);
            cnt++;
        }
        p = p->next;
    }
    if (cnt > 0)
        fprintf(stderr, "Warning: %d implementation blocks have been deprecated; examime source files.\n", cnt);
}

void d2c_parse_impl(FILE *f, char *cmt_start, char *cmt_end)
{
#define IN_SOURCE 0
#define START_IMPL 1
#define IN_IMPL 2
#define END_IMPL 3

#define STATE_WARNING(s) fprintf(stderr, "Warning: line %ld %s, state=%d, %p, %s\n", line, (s), state, d2ci, ((d2ci != NULL) ? d2ci->name : "<no section>"));

    char s[HUGE_BUFFER];
    char s_comment[SMALL_BUFFER], s_dbl_hash[SMALL_BUFFER], s_implementation[SMALL_BUFFER],
         s_preserve[SMALL_BUFFER], s_marker[SMALL_BUFFER], s_class[LARGE_BUFFER], s_name[LARGE_BUFFER];
    int state = IN_SOURCE;
    int count;
    d2c_impl *d2ci;
    endless_string *es;
    long line = 0;
    int preserve;
    d2c_impl_list_destroy();

    while (fgets(s, HUGE_BUFFER - 1, f) != NULL)
    {
        line++;
        if (state == START_IMPL) state = IN_IMPL;
        if (state == END_IMPL) state = IN_SOURCE;
        count = sscanf(s, " %80s %80s %80s %80s %80s %8192s %8192s",
                       s_comment, s_dbl_hash, s_implementation, s_preserve, s_marker, s_class, s_name);
        if (count == 7 &&
            (strncmp(s_comment, cmt_start, SMALL_BUFFER) == 0) &&
            (strncmp(s_dbl_hash, "##", SMALL_BUFFER) == 0) &&
            (strncmp(s_implementation, IMPLEMENTATION, SMALL_BUFFER) == 0))
        {
            if (strncmp(s_marker, "start", SMALL_BUFFER) == 0)
            {
                if (state != IN_SOURCE)
                {
                    STATE_WARNING("found a nested implementation comment")
                }
                preserve = strcmp(s_preserve, "preserve")?0:1;
                state = START_IMPL;
                d2ci = d2c_impl_find_or_add(s_name);
                es = d2ci->impl;
            }
            else if (strncmp(s_marker, "end", SMALL_BUFFER) == 0)
            {
                if (state != IN_IMPL)
                {
                    STATE_WARNING("found an end implemenataion comment without matching start")
                }
                if (d2ci && strcmp(d2ci->name, s_name) != 0)
                {
                    STATE_WARNING("end implementation comment does not match start implementation comment")
                }
                state = END_IMPL;
                d2ci = NULL;
                es = NULL;
            }
            else
            {
                STATE_WARNING("unrecognized state marker")
            }
        }

        if (state == IN_IMPL && preserve)
        {
            append_endless_string(es, s);
            d2ci->in_source= 1;
        }
    }
    if (state != IN_SOURCE && state != END_IMPL)
        STATE_WARNING("found start implementation comment without end comment")
}

// This function takes a UML Operation and mangles it for implementation comments.
// Because it uses an internal buffer to store and return, repeated calls to this
// function will overwrite previous values.
//
char *d2c_operation_mangle_name(umlopnode *op)
{
    static char d2c_mangle_name[LARGE_BUFFER];
    umlattrlist params = op->key.parameters;
    char *p;

    sprintf(d2c_mangle_name, "%s@%s@@", op->key.attr.name, op->key.attr.type);
    while (params != NULL)
    {
        strcat(d2c_mangle_name, "@");
        strcat(d2c_mangle_name, params->key.type);
        params = params->next;
    }

    // Convert whitespace to underbars
    for (p = d2c_mangle_name; *p != '\0'; p++)
    {
        if (*p == ' ' || *p == '\t') *p = '_';
    }
    return d2c_mangle_name;
}

int d2c_backup(char *filename)
{
    // This is not necessarily portable. (requires ability to just
    // tag-on four more characters - not DOS-friendly)
    // But I'll admit to being a bit out-of-the loop here.
    //
    char *backup_filename = my_malloc(strlen(filename) + 4);
    strcpy(backup_filename, filename);
    strcat(backup_filename, ".bak");

	if (generate_backup)
	{
		if (remove(backup_filename))
		{
			if (errno != ENOENT)
			{
				fprintf(stderr, "Error %d while trying to delete file %s\n", errno, backup_filename);
				return -1;
			}
		}
		if (rename(filename, backup_filename))
		{
			if (errno != ENOENT)
			{
				fprintf(stderr, "Error %d while trying to rename %s to %s\n", errno, filename, backup_filename);
				return -1;
			}
		}
	}
    return 0;
}

// Todo on auto-indentation:
// 1. Define meta-characters that are converted to braces

int indent_count = 4;
int indent_open_brace_on_newline = 1;


typedef struct
{
    FILE *f;
    int indentation;
} D2C_INDENT_STRUCT;

D2C_INDENT_STRUCT d2c_files[10];
int d2c_num_files = 0;

int d2c_indent_offset(FILE *f)
{
    int i;
    for (i = 0; i < d2c_num_files; i++)
    {
        if (d2c_files[i].f == f)
        {
            return i;
        }
    }
    d2c_files[i].f = f;
    d2c_files[i].indentation = 0;
    d2c_num_files++;
    return i;
}

void d2c_indent(FILE *f)
{
    d2c_files[d2c_indent_offset(f)].indentation++;
}

void d2c_outdent(FILE *f)
{
    d2c_files[d2c_indent_offset(f)].indentation--;
}

int d2c_fprint_indent(FILE *f)
{
    int i;
    int indentation;

    indentation = d2c_files[d2c_indent_offset(f)].indentation;

    for (i = 0; i < indentation * indent_count; i++)
        fputc(' ', f);
    return i;
}

char d2c_io_lchar = 0;

int _d2c_fputc(int c, FILE *f)
{
	int indent_cnt = 0;
    int rc;

    if (d2c_io_lchar == '\n' && c != '\n')
        indent_cnt = d2c_fprint_indent(f);
    d2c_io_lchar = c;
    rc = fputc(c, f);
    if (rc == EOF)
    	return rc;
    else
    	return indent_cnt + 1;
}

int _d2c_fputs(const char *s, FILE *f)
{
    const char *buf = s;
    //int len = 0;
    while (*buf != '\0')
    {
        if (fputc(*buf, f) == EOF)
            return EOF;
        //len++;
    }
    return 1;
}

char d2c_fprintf_buf[HUGE_BUFFER * 2];

int _d2c_fprintf(FILE *f, char *fmt, ...)
{
    va_list argptr;
    int cnt;
    int extern_cnt;
    int i;

    va_start(argptr, fmt);
    cnt = vsprintf(d2c_fprintf_buf, fmt, argptr);
    va_end(argptr);

    extern_cnt = cnt;
    if (cnt != EOF)
    {
        for (i = 0; i < cnt; i++)
        {
            extern_cnt += _d2c_fputc(d2c_fprintf_buf[i], f);
        }
    }

    return extern_cnt;
}

void d2c_open_brace(FILE *f, char *suffix)
{
    if (indent_open_brace_on_newline)
    {
        d2c_fputc('\n', f);
    }
    else
    {
        d2c_fputc(' ', f);
    }
    d2c_fprintf(f, "{%s\n", suffix);
    d2c_indent(f);
}

void d2c_close_brace(FILE *f, char *suffix)
{
    d2c_outdent(f);
    d2c_fprintf(f, "}%s\n", suffix);
}

param_list *d2c_parameters = NULL;

void param_list_destroy()
{
    param_list *p = d2c_parameters;
    while (p != NULL)
    {
        free(p->name);
        free(p->value);
        d2c_parameters = p;
        p = p->next;
        free(d2c_parameters);
    }
    d2c_parameters = NULL;
}

param_list * d2c_parameter_add(char *name, char *value)
{
    param_list *entry;
    entry = malloc(sizeof(d2c_parameters));
    entry->name = strdup(name);
    entry->value = value ? strdup(value) : NULL;
    entry->next = d2c_parameters;
    d2c_parameters = entry;

    return entry;
}

param_list * d2c_parameter_set(char *name, char *value)
{
    param_list *entry = d2c_parameter_find(name);
    if (entry == NULL)
    	entry=d2c_parameter_add(name, value);
    else
	{
    	free(entry->value);
    	entry->value = value ? strdup(value) : NULL;
	}

    return entry;
}

char * d2c_parameter_value(char *name)
{
	param_list *entry = d2c_parameter_find(name);
	if (entry != NULL)
		return entry->value;
	return NULL;
}

param_list *d2c_parameter_find(char *name)
{
    param_list *p = d2c_parameters;
    while (p != NULL)
    {
        if (strcmp(p->name, name) == 0)
            return p;
        p = p->next;
    }
    return NULL;
}
