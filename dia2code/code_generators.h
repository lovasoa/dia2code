/***************************************************************************
                         code_generators.h  -  Header for the code generating functions
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

#define NO_GENERATORS 13

void generate_code_c(batch *b);
void generate_code_cpp(batch *b);
void generate_code_java(batch *b);
void generate_code_sql(batch *b);
void generate_code_ada(batch *b);
void generate_code_python(batch *b);
void generate_code_php(batch *b);
void generate_code_shp(batch *b);
void generate_code_idl(batch *b);
void generate_code_csharp(batch *b);
void generate_code_php_five(batch *b);
void generate_code_ruby(batch *b);
void generate_code_as3(batch *b);

void inherit_attributes(umlclasslist, umlattrlist);
