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
#include <errno.h>

char * d2c_indentstring = "   ";
int d2c_indentposition = 0;

int indentlevel = 0;
static int number_of_spaces_for_one_indentation = 2;
static int DBG_LEVEL = 4;

void dia2code_initializations()
{
}

void debug_setlevel( int newlevel )
{
    DBG_LEVEL = newlevel;
}

/* 
 * a dummy logger / debugger function
 */
void debug( int level, char *fmt, ... )
{
    static char debug_buffer[HUGE_BUFFER];
    va_list argptr;
    int cnt;
    //printf( "debug call\n" );
    if( level != DBG_LEVEL ) 
        return;
    va_start(argptr, fmt);
    cnt = vsprintf(debug_buffer, fmt, argptr);
    va_end(argptr);
    fprintf( stderr, "DBG %d: %s\n", level, debug_buffer );
    fflush( stderr);
    //printf( "END debug call\n" );
}

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
    namelist list = NULL;

    cp = strdup(s);
    if (cp == NULL) {
        fprintf(stderr, "Out of memory\n");
        exit(1);
    }
    token = strtok (cp, delim);
    while ( token != NULL ) {
        namenode *tmp = NEW (namenode);
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

namelist parse_sql_options(const char *s) {
  /* AI: same thing for now but it could change in the future */
  return parse_class_names(s);
}

int is_present(namelist list, const char *name) {
    while (list != NULL) {
        int len;
        char* mask;
        if ( ! strcmp(list->name, name) ) {
            return 1;
        }
        len = strlen(list->name);
        if (len >= 2 && len <= strlen(name)
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
    /* safer zone */
    memset (tmp, 0, size);
    return tmp;
}


/*
    Builds a package list from the hierarchy of parents of package.
    The topmost package will be the first on the list and the initial
    package will be the last.
*/
umlpackagelist make_package_list(umlpackage * package){
    umlpackagelist dummylist, tmplist=NULL;

    while ( package != NULL ){
        dummylist = NEW (umlpackagenode);
        dummylist->next = tmplist;
        tmplist = dummylist;
        tmplist->key = package;
        package = package->parent;
    }
    return tmplist;
}

umlattrlist copy_attributes(umlattrlist src)
{
    umlattrlist cpy = NULL, start = NULL;

    while (src != NULL)
    {
        umlattrlist tmp = NEW (umlattrnode);
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


/**
 * create a directory hierarchy for the package name 
 * batch.outdir is taken as root directory 
 * works with java-like package naming convention 
 * the directory path is stored in pkg->directory
 * eg. org.foo.bar will create directory tree org/foo/bar
 * @param the current batch
 * @param the package pointer
 * @return the full directory path eg. "<outdir>/org/foo/bar"
 * 
 */
char *create_package_dir( const batch *batch, umlpackage *pkg )
{
    char *fulldirname, *dirname, fulldirnamedup[BIG_BUFFER];
    int ret;
    /* created directories permissions */
    mode_t dir_mask = S_IRUSR | S_IWUSR | S_IXUSR |S_IRGRP | S_IXGRP;
    if (pkg == NULL) {
        return NULL;
    }
    if (batch->buildtree == 0 || pkg->name == NULL) {
        pkg->directory = batch->outdir;
    } else {
        fulldirname = (char*)my_malloc(BIG_BUFFER);
        sprintf(fulldirname, "%s", batch->outdir);
        dirname = strdup(pkg->name);
        dirname = strtok( dirname, "." );
        while (dirname != NULL) {
            sprintf( fulldirnamedup, "%s/%s", fulldirname, dirname );
            sprintf( fulldirname, "%s", fulldirnamedup );
            /* TODO : should create only if not existent */
            ret = mkdir( fulldirname, dir_mask );
            dirname = strtok( NULL, "." );
        }
        /* set the package directory used later for source file creation */
        pkg->directory = fulldirname;
    }
    return pkg->directory;
}

void set_number_of_spaces_for_one_indentation(int n)
{
    number_of_spaces_for_one_indentation = n;
}

char *spc()
{
   static char spcbuf[BIG_BUFFER];
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
    char str[LARGE_BUFFER]; \
    va_start (vargu, first_arg); \
    vsnprintf (str, LARGE_BUFFER, first_arg, vargu); \
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
    static char outfilename[BIG_BUFFER];
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
    return (!strcasecmp(stereo, "enum") ||
            !strcasecmp (stereo, "enumeration") ||
            !strcmp (stereo, "CORBAEnum"));
}

int
is_struct_stereo (char *stereo)
{
    return (!strcasecmp(stereo, "struct") ||
            !strcasecmp (stereo, "structure") ||
            !strcmp (stereo, "CORBAStruct"));
}

int
is_typedef_stereo (char *stereo)
{
    return (!strcasecmp(stereo, "typedef") ||
            !strcmp (stereo, "CORBATypedef"));
}

int
is_const_stereo (char *stereo)
{
    return (!strcasecmp(stereo, "const") ||
            !strcasecmp (stereo, "constant") ||
            !strcmp (stereo, "CORBAConstant"));
}

/* Added by RK 2003-02-20
   This should become part of the uml_class object. */

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
        fprintf(f, "%s", esb->buf); /* We do not d2c_fprintf the buffer, cause it's read in indented. */
        esb = esb->next;
    }
}

endless_string * new_endless_string()
{
    endless_string *es = NEW (endless_string);
    es->start = NULL;
    es->end = NULL;
    return es;
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
    endless_string_buf *esb = NEW (endless_string_buf);
    esb->buf = strdup(s);
    esb->next = NULL;
    if (es->start == NULL)
        es->start = esb;
    if (es->end != NULL)
        es->end->next = esb;
    es->end = esb;
}

struct d2c_impl{
    char name[SMALL_BUFFER];
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
    d2c_impl *d2ci = NEW (d2c_impl);
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

#if defined (USE_DUMPIMPL)
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
#else
void d2c_dump_impl(FILE *f, char *section, char *name)
{
}
#endif

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
                /* We do not d2c_fprintf the buffer, cause it's read in indented. */
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

static char *sscanfmt(int size)
{
    static char buf[10];
    sprintf (buf, " %%%ds", size - 1);
    return buf;
}

void d2c_parse_impl(FILE *f, char *cmt_start, char *cmt_end)
{
#define IN_SOURCE 0
#define START_IMPL 1
#define IN_IMPL 2
#define END_IMPL 3

#define STATE_WARNING(s) fprintf(stderr, "Warning: line %ld %s, state=%d, %p, %s\n", line, (s), state, d2ci, ((d2ci != NULL) ? d2ci->name : "<no section>"));

    char s[HUGE_BUFFER];
    char s_comment[LARGE_BUFFER], s_dbl_hash[SMALL_BUFFER], s_implementation[BIG_BUFFER],
         s_preserve[SMALL_BUFFER], s_marker[SMALL_BUFFER], s_class[LARGE_BUFFER], s_name[LARGE_BUFFER];
    int state = IN_SOURCE;
    int count;
    d2c_impl *d2ci=NULL;
    endless_string *es=NULL;
    long line = 0;
    int preserve=0;
    char fmtbuf[SMALL_BUFFER];

    d2c_impl_list_destroy();

    while (fgets(s, HUGE_BUFFER - 1, f) != NULL)
    {
        line++;
        if (state == START_IMPL) state = IN_IMPL;
        if (state == END_IMPL) state = IN_SOURCE;
	fmtbuf[0] = '\0';
	strcat (fmtbuf, sscanfmt (sizeof(s_comment)));
	strcat (fmtbuf, sscanfmt (sizeof(s_dbl_hash)));
	strcat (fmtbuf, sscanfmt (sizeof(s_implementation)));
	strcat (fmtbuf, sscanfmt (sizeof(s_preserve)));
	strcat (fmtbuf, sscanfmt (sizeof(s_marker)));
	strcat (fmtbuf, sscanfmt (sizeof(s_class)));
	strcat (fmtbuf, sscanfmt (sizeof(s_name)));
        count = sscanf(s, fmtbuf,
                       s_comment, s_dbl_hash, s_implementation, s_preserve, s_marker, s_class, s_name);
        if (count == 7 &&
            (strncmp(s_comment, cmt_start, sizeof(s_comment)) == 0) &&
            (strncmp(s_dbl_hash, "##", sizeof(s_dbl_hash)) == 0) &&
            (strncmp(s_implementation, IMPLEMENTATION, sizeof(s_implementation)) == 0))
        {
            if (strncmp(s_marker, "start", sizeof(s_marker)) == 0)
            {
                if (state != IN_SOURCE)
                {
                    STATE_WARNING("found a nested implementation comment")
                }
                preserve = strcmp(s_preserve, "preserve") ? 0 : 1;
                state = START_IMPL;
                d2ci = d2c_impl_find_or_add(s_name);
                es = d2ci->impl;
            }
            else if (strncmp(s_marker, "end", sizeof(s_marker)) == 0)
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

/* This function takes a UML Operation and mangles it for implementation comments.
   Because it uses an internal buffer to store and return, repeated calls to this
   function will overwrite previous values.
*/
char *d2c_operation_mangle_name(umloperation *ope)
{
    static char d2c_mangle_name[LARGE_BUFFER];
    umlattrlist params = ope->parameters;
    char *p;

    sprintf(d2c_mangle_name, "%s@%s@@", ope->attr.name, ope->attr.type);
    while (params != NULL)
    {
        strcat(d2c_mangle_name, "@");
        strcat(d2c_mangle_name, params->key.type);
        params = params->next;
    }

    /* Convert whitespace to underbars */
    for (p = d2c_mangle_name; *p != '\0'; p++)
    {
        if (*p == ' ' || *p == '\t') *p = '_';
    }
    return d2c_mangle_name;
}

int d2c_backup(char *filename)
{
    /* This is not necessarily portable. (requires ability to just
     * tag-on four more characters - not DOS-friendly)
     * But I'll admit to being a bit out-of-the loop here.
     */
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

/* Todo on auto-indentation:
   1. Define meta-characters that are converted to braces
*/
int indent_count = 4;
int indent_open_brace_on_newline = 1;


typedef struct
{
    FILE *f;
    int indentation;
} D2C_INDENT_STRUCT;

D2C_INDENT_STRUCT d2c_files[32];
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
    /* int len = 0; */
    while (*buf != '\0')
    {
        if (fputc(*buf, f) == EOF)
            return EOF;
        /* len++; */
        buf++;
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

int d2c_directprintf(FILE *f, char *fmt, ...)
{
    va_list argptr;
    int cnt;

    d2c_indentate(f);
    va_start(argptr, fmt);
    cnt = vfprintf(f, fmt, argptr);
    va_end(argptr);
    return 0;
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
    param_list *entry = my_malloc (sizeof(d2c_parameters));
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


void d2c_indentate( FILE *f)
{
    int i;
    for ( i = 0; i < d2c_indentposition; i++) {
        fputs( d2c_indentstring, f);
    }
}


/*
 * increment the tab position 
 * tab position is used in d2c_fprintf and alike
 */
void d2c_shift_code()
{
    d2c_indentposition ++;   
}


/*
 * increment the tab position 
 * tab position is used in d2c_fprintf and alike
 */
void d2c_unshift_code()
{
    if( d2c_indentposition > 0 )
        d2c_indentposition --;
}




/* 
* find a diaoid token in a string
* the diaoid must be formatted as @diaoid <oid> where oid is a string without space
* @param the NULL terminated string buffer to look in 
* @param (out) a pointer located on the first oid charater - NULL is allowed if you dont need this pointer
* @return the diaoid found or NULL if none is found 
*/
char *find_diaoid( const char *buf, char **newpos  )
{
    static const char *oidtag = "@diaoid";
    char *cp, *ep; // current pos, diaoid ending position
    char *oidp=NULL;
    debug( DBG_CORE, "find_diaoid()" );
    if( buf == NULL ) {
        return NULL;
    }
    cp = strstr( buf, oidtag );
    if( cp == NULL )
        return NULL;
    cp += strlen(oidtag)+1;
    /* get the oid */
    ep = strpbrk( cp, " \t\n\r" );
    if( ep == NULL ) {
        oidp = strdup(cp);
    } else {
        oidp= (char*) strndup( cp, (size_t) ( ep-cp));
    }
    /* caller want the new position : we set it */
    if( newpos != NULL ) {
        (*newpos) = cp;
    }
    return oidp;
}


