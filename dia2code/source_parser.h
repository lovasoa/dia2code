#ifndef D2C_SOURCEPARSER_H
#define D2C_SOURCEPARSER_H

#include <unistd.h> 
#include <sys/types.h> 
#include <sys/stat.h>
#include <fcntl.h>

/*
 defines a section of source code between starting and ending positions
 as for now, two additional informations are attached to each block : an unique oid and a type 
*/
struct sourceblock {
    char * spos; /* starting position - file buffer offset */
    size_t len; /* length in char */
    int type;
    char *oid;
};

typedef struct sourceblock sourceblock;

/* 
 to link block togethers 
*/
struct _sourceblocknode {
    sourceblock *blk;
    struct _sourceblocknode *next;
};

typedef struct _sourceblocknode sourceblocknode;

/*  
  macro structure used for code preservation feature 
  buffer contains the complete source of the file
  blocks is a list of identified blocks (methods implementations for now) in this source
*/
struct _sourcecode
{
    sourceblocknode *blocks;
    char *buffer;
};
typedef struct _sourcecode sourcecode;


void source_preserve( batch *b, umlclass *class, const char *filename, sourcecode *source );

//sourceblock *block_find( sourceblocknode list, const char *oid );
sourceblock *sourceblock_find( sourceblocknode *list, const char *oid );

sourceblocknode *source_parse( const char *sourcebuffer );

sourceblock *sourceblock_new( char *spos, size_t len, char *oid, int type );

char *source_loadfromfile(const char *filename );

int find_more_str( char *buf, char *tofind[], sourceblock pblocks[] );

sourceblock *find_matching_char( const char *buf, char *oid, char openc, char closec, int maxcars );


#endif
