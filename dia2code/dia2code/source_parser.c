/**
    DIA2CODE 
    source_parser.c code-preservation functions
    provides some functions to parse source code and extract method implementations
    @debug DBG 6  (public functions ) or 7 (low level calls)
    @author Leo West west_leoATyahooDOTcom
    
**/

#include "dia2code.h"
#include "source_parser.h"

/*  block with heterogen or unknown content */
#define SP_AT_MISC  1
/* block containing an atribute definition */
#define SP_AT_IMPL   2
/* block containing a method implementation */
#define SP_OP_IMPL   3

/*
 * test a file existence and extract the source and the blocks
 * source is allocated and initialised with the source code buffer and the blocks markers
 */
void source_preserve( batch *b, umlclass *class, const char *filename, sourcecode *source )
{
    char *diaoid = NULL;
    umloplist umlo;
    sourceblock *srcblock = NULL;
    debug( 4, "preserve_source(filename=%s)", filename);
    source = (sourcecode*) my_malloc( sizeof(sourcecode));
    source->buffer = NULL;
    source->blocks = NULL;
    
    //open the file in read only 
    FILE * rofile = fopen(filename, "r");
    if( ! rofile ) {
        debug( DBG_SOURCE, "no existing file %s for class %s", filename, class->name );
        return;
    }
    /* from here, the file exists, we transfer the content in a buffer and parse the source */
    source->buffer = source_loadfromfile( filename );
    if( source->buffer == NULL ) {
        debug( 4, "warning: NULL sourcebuffer from file" );
    } else {
        source->blocks  = source_parse( source->buffer );
        /* copy source blocks found to method->implementation */
        umlo = class->operations;
        while( umlo != NULL ) {
            /* is there a diaoid hidden in the operation comment ? */
            if( (diaoid=find_diaoid(umlo->key.attr.comment,NULL)) != NULL) {
                debug( DBG_SOURCE,"diaoid:%s found in comment for method %s", diaoid, umlo->key.attr.name );
                /* now try to find the implementation block in the sourcebuffer */
                srcblock = sourceblock_find( source->blocks, diaoid );
                // srcblock->spos poitns the implementation of lengtjh srcblock->len
                if( srcblock != NULL ) {
                    umlo->key.implementation = (char*) strndup( srcblock->spos, srcblock->len );
                }
            } else {
                debug( DBG_SOURCE, "diaoid %s not found in source", diaoid );
            }
            umlo = umlo->next;
        } // while
    }
    fclose(rofile);
}

/*
 * constructor for a new sourceblock structure address
 * @return an initialized sourceblock address
 */
sourceblock *sourceblock_new( char *spos, size_t len, char *oid, int type )
{
    debug( DBG_GENCODE, "sourceblock_new( spos=%p, len=%ld, oid=%s, type=%d )", spos,len,oid,type );
    // TODO : safety checks
    sourceblock *blk;
    blk = (sourceblock*) malloc( sizeof(sourceblock));
    blk->spos = spos;
    blk->len = len;
    blk->type = type;
    blk->oid = (char*) strndup( oid, 30 );
    return blk;
}

/*
* add a sourceblock at the head of the chained list
* @param the node is added at end of the list
*@return the newly allocated node
 */ 
sourceblocknode *sourceblocklist_add( sourceblocknode *list, const sourceblock *blk )
{
    debug( DBG_GENCODE, "sourceblocklist_add( list=%p, blk=%p )", list,blk );
    sourceblocknode *new =  (sourceblocknode*) malloc( sizeof(sourceblocknode));
    new->blk = (sourceblock*) blk;
    /* if the list is null, no need to link  */
    if( list == NULL ) {
        new->next = NULL;
    } else {
        new->next = list;
    }
    return new;
}

sourceblock *sourceblock_find( sourceblocknode *list, const char *oid )
{
    sourceblock *blk = NULL;
    sourceblocknode *pt=list;
    if( list == NULL ) {
        debug( DBG_GENCODE, "list should not be null in sourceblock_find" );
        return NULL;
    }
    if( oid == NULL ) {
        debug(  DBG_GENCODE,"oid should not be null in sourceblock_find\n" );
        return NULL;
    }
    while( pt != NULL ) {
        
        if( (pt->blk->oid != NULL) && (strcmp( pt->blk->oid, oid ) == 0) ) {
            return pt->blk;
        }
        pt = pt->next;
    }
    return NULL;
}

sourceblocknode *source_parse( const char *sourcebuffer )
{
    char *tokens[] = { "*/", "{",'\0' }; // we mut find this successfully these tags to isolate the funtion/method source
    sourceblocknode *list=NULL;
    sourceblock *srcbk;
    char *currentoid=NULL;
    char **newpos= (char**) malloc( sizeof(char*) );
    sourceblock tokenblocks[3];
    char  *pos = (char*) sourcebuffer; /* moving pointer */
    
    debug( DBG_GENCODE, "source_parse( sourcebuffer=%p )\n", sourcebuffer );
    
    while( (currentoid = find_diaoid( pos, newpos)) != NULL ) 
    {
        debug( 7, "source_parse : oid=%s  buf/newpos : %p/%p \n", currentoid, sourcebuffer, *newpos);
        int numfound = find_more_str( *newpos, tokens, tokenblocks );
        debug( 7, "found %d items from 2\n", numfound );
        /* +1 for the ending bracket that is oitherwise not included */
        if( numfound != 2 ) {
            debug( 7, "not all 2 tokens found" );
        }
       else {
            /*  find matching brace from char spos*/
            srcbk = find_matching_char( tokenblocks[1].spos, currentoid , '{', '}', 5000 );
            if( srcbk != NULL ) {
                debug( DBG_GENCODE, "srcbk spos=%p len=%ld \n",  srcbk->spos, srcbk->len );
                /* this to remove the { and } from the block */
                srcbk->spos ++; srcbk->len -= 2;
                char *source = (char*) strndup( srcbk->spos, srcbk->len ); /* only used for debug message */
                debug( 7, "METHOD SOURCE-----------------------------------------\n%s\n------------------------------------\n", source );
                free(source);
                list = sourceblocklist_add( list, srcbk );                
            } else {
                debug(7, "METHOD SOURCE NOT FOUND\n" );
            }
        }
        pos = *newpos;
    }
    return list;
}



/*
 * transfer a file content into a char buffer
 * in case of failure, return NULL
 * @param the filename
 * @return the allocated buffer as a char * 
 *
 */
char *source_loadfromfile(const char *filename )
{
    struct stat info;
    int fi; // file descriptor
    size_t sz;
    char *buf = NULL;
    
    if( stat( filename, &info ) ) {
        return NULL;
    }

    sz = info.st_size;
    fi = open(filename, O_RDONLY, 0777);
    if( ! fi  ) {
       return NULL;
    }
    buf = my_malloc(sizeof(char)*sz + 2 );
    read( fi, buf, sz );
    close(fi);
    buf[sz]= '\0';
    return buf;
}




int find_more_str( char *buf, char *tofind[], sourceblock pblocks[] )
{
    char *tok = NULL;
    int i=0;
    char *ep, *np = buf;
    
    while( tofind[i] != NULL ) {
        tok = tofind[i];
        ep = strstr( np, tok );
        if( ep == NULL ) {
            return i;
        }
        else {
            pblocks[i].spos = ep;
            if( i > 0 ) {
                pblocks[i-1].len = ep-np;
            }
            // printf( "--->found %s<--------\n", (char*)strndup(ep,40));
            np=ep;
        }
        i++;
    }
    return i; // number of stuff found
}

/**
 * given an opening char openc, find the matching closing char closec in a text buffer
 * support nested blocks 
 * @param the text buffer
 * @param opening char
 * @param closing char
 * @param  a limiter where parsing should stop anyway
 * @return a sourceblock . if none found, result.len = 0
 * @example find_matching_char( cppsource, '{', '}', 40000 );
 * TODO : ignore some section such as comment blocks, strings
 * @return the found position as a char pointer, NULL if not found
 */
sourceblock *find_matching_char( const char *buf, char *oid, char openc, char closec, int maxcars )
{
    char *cp, *startpos;
    int len=0, depth = 0;
    sourceblock *srcbk=NULL; // the block returned
    cp = (char*) buf;
    debug( DBG_GENCODE, "find_matching_char. openc=%c closec=%c oid=%s maxcars=%d", openc, closec, oid, maxcars );
    while( cp != NULL ) {
        
        if( *cp == openc ) {
            debug( 7, "openc found" );
            if( depth == 0 ) {
                startpos=cp;
            }
            depth ++;
        }else if( *cp == closec ) {
            debug( 7, "closec found " );
            depth --;
            /* we found the closing char */
            if( depth == 0 ) {
                size_t bklen = cp - buf +1;
                srcbk = sourceblock_new( startpos, bklen, oid,  SP_OP_IMPL );
                debug( 7, "sourceblock returned: spos=%p len=%ld oid=%s", srcbk->spos, srcbk->len, srcbk->oid );
                break;
            }
        }
        if( len > maxcars ) {
            break;
        }
        len ++;
        cp ++;
    }
    return srcbk;
}

