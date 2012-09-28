/***************************************************************************
                          comment_helper.c
                             -------------------
    begin                : Sun Mar 14 2004
    copyright            : (C) 2004 by Leo West
    email                : west_leo AT yahoo DOT com
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
* generate a comment block for a class
  *
 *
 */
void generate_class_comment( FILE *outfile, batch *b, umlclass *class )
{
    
    
}

/**
 * generate a comment block for an operation
 *
 */
void generate_operation_comment( FILE *outfile, batch *b, umloperation *ope )
{
    umlattrlist  tmpa;
    d2c_fprintf(outfile, "/**\n");
    d2c_fprintf(outfile, " * Operation %s\n", ope->attr.name );
    if( ope->attr.comment[0] != 0 ) {
        d2c_fprintf(outfile, " * %s\n", ope->attr.comment );
    }
    d2c_fprintf(outfile, " *\n");
     tmpa = ope->parameters;
    while (tmpa != NULL) {
        d2c_fprintf(outfile, " * @param %s - %s\n", tmpa->key.name, tmpa->key.comment );
        tmpa = tmpa->next;
    }
    if(strcmp(ope->attr.type, "void")) {
        d2c_fprintf(outfile, " * @return %s\n", ope->attr.type);
    }
    d2c_fprintf(outfile, " */\n");
}


/**
 * generate a comment block for an UML attribute
 *
 */
void generate_attribute_comment( FILE *outfile, batch *b, umlattribute *attr )
{
    d2c_fprintf(outfile, "/**\n");
    d2c_fprintf(outfile, " * %s\n", attr->comment );
    d2c_fprintf(outfile, " */\n");
}


