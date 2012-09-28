#ifndef D2C_COMMENTHELPER_H
  #define D2C_COMMENTHELPER_H


void generate_class_comment( FILE *outfile, batch *b, umlclass *class );
void generate_operation_comment( FILE *outfile, batch *b, umloperation *ope );
void generate_attribute_comment( FILE *outfile, batch *b, umlattribute *attr );


#endif
