####### kdevelop will overwrite this part!!! (begin)##########
noinst_LIBRARIES = libdia2code.a

libdia2code_a_SOURCES = decls.c includes.c generate_code_idl.c generate_code_php.c generate_code_shp.c generate_code_python.c generate_code_ada.c generate_code_sql.c scan_tree.c generate_code_c.c parse_diagram.c dia2code.c generate_code_java.c generate_code_cpp.c generate_code_csharp.c main.c 

SUBDIRS = docs 

EXTRA_DIST = main.c generate_code_cpp.c generate_code_java.c code_generators.h dia2code.h dia2code.c parse_diagram.c parse_diagram.h generate_code_c.c scan_tree.c generate_code_sql.c generate_code_ada.c generate_code_python.c Makefile.am generate_code_shp.c generate_code_php.c makefile.mingw generate_code_idl.c generate_code_csharp.c decls.c includes.c

####### kdevelop will overwrite this part!!! (end)############
bin_PROGRAMS = dia2code
INCLUDES = @XML_INCLUDES@
dia2code_SOURCES = dia2code.c main.c parse_diagram.c scan_tree.c generate_code_c.c \
generate_code_java.c generate_code_cpp.c generate_code_ada.c generate_code_sql.c \
generate_code_python.c generate_code_php.c generate_code_shp.c generate_code_idl.c \
generate_code_csharp.c decls.c includes.c



