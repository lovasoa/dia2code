/***************************************************************************
                                  main.c
                             -------------------
    begin                : Sun Oct 15 2000
    copyright            : (C) 2000-2001 by Javier O'Hara
    email                :  joh314@users.sourceforge.net
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
#include "code_generators.h"
#include "parse_diagram.h"

int process_initialization_file(char *filename, int exit_if_not_found);

#define DEFAULT_TARGET 0

#ifdef DSO
static void *
find_dia2code_module(const char *lang) {
    char *homedir;
    char *modulepath;
    char *modulename;
    /*  char *generatorname;*/
    void *handle;
    void (*generator)();

    homedir = getenv("HOME");
    if ( homedir )
        homedir = strdup(homedir);
    else
        homedir = strdup(".");

    modulename = (char*)malloc(strlen(DSO_PREFIX) + strlen(lang) + 1);
    sprintf(modulename, "%s%s", DSO_PREFIX, lang);

    modulepath = (char*)malloc(strlen(homedir) + strlen(modulename)
                               + strlen(MODULE_DIR) + strlen(DSO_SUFFIX) + 3);
    sprintf(modulepath, "%s/%s/%s%s", homedir, MODULE_DIR, modulename, DSO_SUFFIX);

    handle = dlopen(modulepath, RTLD_NOW | RTLD_GLOBAL);
    if ( !handle ) {
        fprintf(stderr, "can't find the module: %s\n", dlerror());
        exit(2);
    }
    printf("module name : %s\n", modulename);
    generator = dlsym(handle, modulename);

    if ( modulepath ) free(modulepath);
    if ( modulename ) free(modulename);
    if ( homedir ) free(homedir);

    return generator;
}
#endif /* DSO */

char *outdir = NULL;   /* Output directory */

int INDENT_CNT = 4; // This should be a parameter in the command line
int bOpenBraceOnNewline = 1; // This should also be a command-line parameter

int main(int argc, char **argv) {
    int i;
    char *license = NULL;  /* License file */
    int clobber = 1;   /*  Overwrite files while generating code*/
    char *infile = NULL;    /* The input file */
    namelist classestogenerate = NULL;
    int classmask = 0, parameter = 0;
    batch *thisbatch;
    int iniParameterProcessed;
    char inifile[HUGE_BUFFER];

    void (*generator)(batch *);
    void (*generators[NO_GENERATORS])(batch *);

    char * notice = "\
dia2code version 0.8.1, Copyright (C) 2000-2001 Javier O'Hara\n\
Dia2Code comes with ABSOLUTELY NO WARRANTY\n\
This is free software, and you are welcome to redistribute it\n\
under certain conditions; read the COPYING file for details.\n";

    char *help = "[-h|--help] [-d <dir>] [-nc] [-cl <classlist>]\n\
       [-t (ada|c|cpp|idl|java|php|python|shp|sql|csharp)] [-v]\n\
       [-l <license file>] [-ini <initialization file>]<diagramfile>";

    char *bighelp = "\
    -h --help            Print this help and exit\n\
    -t <target>          Selects the output language. <target> can be\n\
                         one of: ada,c,cpp,idl,java,php,python,shp,sql or csharp. \n\
                         Default is C++\n\
    -d <dir>             Output generated files to <dir>, default is \".\" \n\
    -l <license>         License file to prepend to generated files.\n\
    -nc                  Do not overwrite files that already exist\n\
    -cl <classlist>      Generate code only for the classes specified in\n\
                         the comma-separated <classlist>. \n\
                         E.g: Base,Derived.\n\
    -v                   Invert the class list selection.  When used \n\
                         without -cl prevents any file from being created\n\
    -ext <extension>     Use <extension> as the file extension.\n\
                         Here are the defaults:\n\
                         ada:\"ads\", c:\"h\", cpp:\"h\", idl:\"idl\",\n\
                         java:\"java\", php:\"php\", python:\"py\", csharp:\"cs\".\n\
                         Not applicable to shp, sql.\n\
    -bext <extension>    Use <extension> as the body (implementation) file\n\
                         extension. Currently only applies only to ada.\n\
                         Here are the defaults:\n\
                         ada:\"adb\"\n\
    -ini <file>          Can be used instead of command-line parameters\n\
    <diagramfile>        The Dia file that holds the diagram to be read\n\n\
    Note: parameters can be specified in any order.";

    generator = NULL;
    generators[0] = generate_code_cpp;
    generators[1] = generate_code_java;
    generators[2] = generate_code_c;
    generators[3] = generate_code_sql;
    generators[4] = generate_code_ada;
    generators[5] = generate_code_python;
    generators[6] = generate_code_php;
    generators[7] = generate_code_shp;
    generators[8] = generate_code_idl;
    generators[9] = generate_code_csharp;


    if (argc < 2) {
        fprintf(stderr, "%s\nUsage: %s %s\n", notice, argv[0], help);
        exit(2);
    }

    iniParameterProcessed = 0;

    /* Argument parsing: rewritten from scratch */
    for (i = 1; i < argc; i++) {
        switch ( parameter ) {
        case 0:
            if ( ! strcmp (argv[i], "-t") ) {
                parameter = 1;
            } else if ( ! strcmp (argv[i], "-d") ) {
                parameter = 2;
            } else if ( ! strcmp (argv[i], "-nc") ) {
                clobber = 0;
            } else if ( ! strcmp (argv[i], "-cl") ) {
                parameter = 3;
            } else if ( ! strcmp (argv[i], "-l") ) {
                parameter = 4;
            } else if ( ! strcmp (argv[i], "-ext") ) {
                parameter = 5;
            } else if ( ! strcmp (argv[i], "-bext") ) {
                parameter = 6;
            } else if ( ! strcmp (argv[i], "-ini") ) {
                parameter = 7;
            } else if ( ! strcmp (argv[i], "-v") ) {
                classmask = 1 - classmask;
            } else if ( ! strcmp("-h", argv[i]) || ! strcmp("--help", argv[i]) ) {
                printf("%s\nUsage: %s %s\n\n%s\n", notice, argv[0], help, bighelp);
                exit(0);
            } else {
                infile = argv[i];
            }
            break;
        case 1:   /* Which code generator */
            parameter = 0;
            if ( ! strcmp (argv[i], "c") ) {
                generator = generators[2];
            } else if ( ! strcmp (argv[i], "cpp") ) {
                generator = generators[0];
            } else if ( ! strcmp (argv[i], "java") ) {
                generator = generators[1];
            } else if ( ! strcmp (argv[i], "sql") ) {
                generator = generators[3];
            } else if ( ! strcmp (argv[i], "ada") ) {
                generator = generators[4];
            } else if ( ! strcmp (argv[i], "python") ) {
                generator = generators[5];
            } else if ( ! strcmp (argv[i], "php") ) {
                generator = generators[6];
            } else if ( ! strcmp (argv[i], "shp") ) {
                generator = generators[7];
            } else if ( ! strcmp (argv[i], "idl") ) {
                generator = generators[8];
            } else if ( ! strcmp (argv[i], "csharp") ) {
                generator = generators[9];
            } else {
#ifdef DSO
                generator = find_dia2code_module(argv[i]);
                if ( ! generator ) {
                    fprintf(stderr, "can't find the generator: %s\n", dlerror());
                    parameter = -1;   /* error */
                }
#else
parameter = -1;   /* error */
#endif
            }
            break;
        case 2:   /* Which output directory */
            outdir = argv[i];
            parameter = 0;
            break;
        case 3:   /* Which classes to consider */
            classestogenerate = parse_class_names(argv[i]);
            classmask = 1 - classmask;
            parameter = 0;
            break;
        case 4:   /* Which license file */
            license = argv[i];
            parameter = 0;
            break;
        case 5:   /* Which file extension */
            file_ext = argv[i];
            parameter = 0;
            break;
        case 6:   /* Which implementation file extension */
            body_file_ext = argv[i];
            parameter = 0;
            break;
        case 7:   /* Use initialization file */
            process_initialization_file(argv[i], 1);
            iniParameterProcessed = 1;
            parameter = 0;
            break;
        }
    }
    /* parameter != 0 means the command line was invalid */

    if ( parameter != 0 || infile == NULL ) {
        printf("%s\nUsage: %s %s\n\n%s\n", notice, argv[0], help, bighelp);
        exit(2);
    }

    if (iniParameterProcessed == 0)
    {
        if (!process_initialization_file("dia2code.ini", 0))
        {
#ifdef WIN32
            if (getenv("HOME") == NULL)
                strcpy(inifile, "c:");
            else
                strcpy(inifile, getenv("HOME"));
            strcat(inifile, "\\dia2code\\dia2code.ini");
#else
            strcpy(inifile, "~/.dia2code/dia2code.ini");
#endif
            process_initialization_file(inifile, 0);           
        }
    }



    thisbatch = (batch*)my_malloc(sizeof(batch));

    LIBXML_TEST_VERSION;
    xmlKeepBlanksDefault(0);

    /* We build the class list from the dia file here */
    thisbatch->classlist = parse_diagram(infile);

    thisbatch->outdir = outdir;
    thisbatch->license = license;
    thisbatch->clobber = clobber;
    thisbatch->classes = classestogenerate;
    thisbatch->mask = classmask;

    /* Code generation */
    if ( !generator ) {
        generator = generators[DEFAULT_TARGET];
    };
    (*generator)(thisbatch);

    param_list_destroy();
    return 0;
}

typedef struct ini_parse_command
{
    char *name;
    int type;
    void *ref;
} ini_parse_command;

#define PARSE_TYPE_FUNCTION 0
#define PARSE_TYPE_INT 1
#define PARSE_TYPE_STRCPY 2
#define PARSE_TYPE_STRDUP 3
#define PARSE_TYPE_YESNO 4
#define PARSE_TYPE_TRUEFALSE 5

ini_parse_command ini_parse_commands[] =
{
    {"file.outdir", PARSE_TYPE_STRDUP, &outdir},
    {"indent.brace.newline", PARSE_TYPE_YESNO, &indent_open_brace_on_newline},
    {"indent.size", PARSE_TYPE_INT, &indent_count},
    {"generate.backup", PARSE_TYPE_YESNO, &generate_backup},
    {NULL, -1, NULL}
};

void parse_command(char *name, char *value)
{
    int i = 0;
    void (*method)(char *, char *);
    char **css;

    while (1)
    {
        ini_parse_command *cmd = &ini_parse_commands[i];
        if(cmd->name == NULL)
            break;
        if (!strcmp(cmd->name, name) == 0)
        {
            i++;
            continue;
        }
        switch(cmd->type)
        {
        case PARSE_TYPE_FUNCTION:
            method = cmd->ref;
            (*method)(name, value);
            break;

        case PARSE_TYPE_INT:
            *(int *)(cmd->ref) = atoi(value);
            break;

        case PARSE_TYPE_STRCPY:
            strcpy(value, (char *)cmd->ref);
            break;

        case PARSE_TYPE_STRDUP:
            css = (char **)cmd->ref;
            if (*css)
                free(*css);
               *css = strdup(value);
            break;

        case PARSE_TYPE_YESNO:
            switch(tolower(value[0]))
            {
            case 'y': *(int *)(cmd->ref) = 1; break;
            case 'n': *(int *)(cmd->ref) = 0; break;
            default:
                fprintf(stderr, "Invalid yes/no value for %s(%s)\n", name, value);
            }
            break;

        case PARSE_TYPE_TRUEFALSE:
            switch(tolower(value[0]))
            {
            case 't': *(int *)(cmd->ref) = 1; break;
            case 'f': *(int *)(cmd->ref) = 0; break;
            default:
                fprintf(stderr, "Invalid true/false value for %s(%s)\n", name, value);
            }
            break;


        default:
            break;
        }
        return;
    }
}

int process_initialization_file(char *filename, int exit_if_not_found)
{
    FILE *f = fopen(filename, "r");
    int line = 0;
    int slen;
    char s[HUGE_BUFFER];
    
    if (f == NULL)
    if (exit_if_not_found)
    {
        fprintf(stderr, "Could not open initialization file %s\n", filename);
        exit(-1);
    }
    else
        return 0;

    while (fgets(s, HUGE_BUFFER - 1, f) != NULL)
    {
        char *name = s;
        char *param = strchr(s, '=');

        line++;
        if (s[0] == '#')
            continue;

        if (param == NULL)
        {
            fprintf(stderr, "Invalid parameter entry in %s:%d\n", filename, line);
        }
        else
        {
            *(param++) = '\0';
            slen = strlen(param) - 1;
            while (param[slen] == '\n' || param[slen] == '\r')
            {
                param[slen--] = '\0';
            }
        }
        d2c_parameter_set(name, param);
        parse_command(name, param);
    }
    fclose(f);
    
    return 1;
}
