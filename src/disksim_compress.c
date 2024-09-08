#include <stdio.h>
#include <stdlib.h>
#include "disksim_compress.h"

#ifdef ASSERT1
#undef ASSERT1
#endif

#ifdef ASSERT2
#undef ASSERT2
#endif

#ifdef ASSERT3
#undef ASSERT3
#endif

#ifdef ASSERT
#undef ASSERT
#endif

#define ASSERT0( fmt )   do { \
            printf("ASSERT: %s,%d info: %s\n", __FUNCTION__, __LINE__, fmt); \
            exit(1); \
    } while(0)

#define ASSERT1( cond )   do { \
        if( cond ){ \
            printf("ASSERT: %s,%d : %s\n", __FUNCTION__, __LINE__, #cond); \
            exit(1); \
        } \
    } while(0)

#define ASSERT2( cond, fmt )   do { \
        if( cond ){ \
            printf("ASSERT: %s,%d : %s : %s\n", __FUNCTION__, __LINE__, #cond, fmt); \
            exit(1); \
        } \
    } while(0)

#define ASSERT3( cond, fmt, ... )   do { \
        if( cond ){ \
            printf("ASSERT: %s,%d : %s : ", __FUNCTION__, __LINE__, #cond); \
            printf( fmt, __VA_ARGS__); \
            printf("\n"); \
            exit(1); \
        } \
    } while(0)

#define ASSERT ASSERT1

comp_node* global_comp = NULL;
FILE* comp_debug_file = NULL;
const int COMPRESS_RECORD_DEBUG = 0;
const int COMPRESS_PRINT_DEBUG = 0;
const int COMP_READ_COMPRESS = 1;
const int COMP_WRITE_COMPRESS = 0;
const int COMP_HEAD_SIZE = 48;
const int COMP_MIN_LEFT = 560;
const double COMP_READ_TIME = 0.0004;
const int COMP_TRACE_USE_DEFAULT_RATIO = 1;
double COMP_DEFAULT_RATIO = 0.00;

compress_struct* compress_global;

void compress_trace_set_default_ratio(char** s, int i)
{
    double j = -1;
    j = atof(s[i]);
    if(COMP_TRACE_USE_DEFAULT_RATIO == 1) {
        COMP_DEFAULT_RATIO = j;
    }
}

int compress_init()
{
    comp_debug_file = fopen( "./logcompress", "w");
    compress_global = (compress_struct* )calloc(1, sizeof(compress_struct) );
    compress_global->comp_stats_global = NULL;
    fprintf(disksim->exectrace, "head=%d readtime=%f ratio=%f\n", COMP_HEAD_SIZE, COMP_READ_TIME, COMP_DEFAULT_RATIO);
    fprintf(comp_debug_file, "head=%d readtime=%f ratio=%f\n", COMP_HEAD_SIZE, COMP_READ_TIME, COMP_DEFAULT_RATIO);
    printf("head=%d readtime=%f ratio=%f\n", COMP_HEAD_SIZE, COMP_READ_TIME, COMP_DEFAULT_RATIO);
    if(COMPRESS_PRINT_DEBUG){
        if( COMP_READ_COMPRESS ){
            printf("Enable READ compress!\n");
        }
        else{
            printf("Disable READ compress!\n");
        }
        if( COMP_WRITE_COMPRESS ){
            printf("Enable WRITE compress!\n");
        }
        else{
            printf("Disable WRITE compress!\n");
        }
    }
    return (0);
}

int compress_destroy()
{
    compress_print_stats();
    fclose(comp_debug_file);
    free(compress_global);
    return (0);
}

void compress_debug( const char* filename, const char* function, int line)
{
    printf( "%s:%d - %s : ERROR!\n", filename, line, function);
    exit(1);
}

int compress_print_0(char* msg)
{
    fprintf(comp_debug_file, "%s\n", msg);
    return (TRUE);
}

/* 
 * set the ratio for a region of continuous disk blocks. The vaild range is <start> to <start+end-1>
 * return value:  TRUE or FALSE
 */
int compress_set_ratio_ioreq( int start, int end, ioreq_event* curr, double ratio)
{
    if(disksim->synthgen) return (TRUE); 
    ASSERT1( disksim->traceformat != ASCII);
    ASSERT1( curr==NULL || end<=start || ratio <=0 || ratio >1);

    if(COMPRESS_RECORD_DEBUG)
        fprintf(comp_debug_file, "%f compress set ratio: %d - %d curr %p ratio %f\n",simtime, start, end, curr, ratio);
    comp_node* tmp = (comp_node*) malloc( sizeof(comp_node) );
    tmp->blkstart = start;
    tmp->blkend = end;
    tmp->ratio = ratio;
    tmp->ioreq = curr;
    tmp->next = NULL;
    if( global_comp == NULL ){
        global_comp = tmp;   
        return (TRUE);
    }
    comp_node* tmp_i = global_comp;
    while( tmp_i -> next){
        if(tmp_i->ioreq == curr){
            printf("compress set reatio %p :duplicate ioreq %p\n", tmp_i, curr );
            free(tmp);
            exit(1);
        }
        tmp_i = tmp_i ->next;
    }
    if(tmp_i->ioreq == curr){
        printf("compress set reatio %p :duplicate ioreq %p\n", tmp_i, curr );
        free(tmp);
        exit(1);
    }
    tmp_i ->next = tmp;
    return (TRUE);
}

/* Delete the comp struct for ioreq */
int compress_remove_ratio_ioreq( ioreq_event *curr)
{
    if(disksim->synthgen) return (TRUE); 
    ASSERT1( disksim->traceformat != ASCII);
    ASSERT2(global_comp == NULL, "compress delete error");

    comp_node* tmp_i = global_comp;
    comp_node* tmp_j = NULL;

    if(global_comp->ioreq == curr){
        tmp_j = global_comp;
        global_comp = global_comp->next;
        free(tmp_j);
        return (TRUE);
    }
    while( tmp_i -> next){
        if(tmp_i->next->ioreq == curr){
            tmp_j = tmp_i -> next;
            tmp_i ->next = tmp_j -> next;
            free(tmp_j);
            return (TRUE);
        }
        tmp_i = tmp_i ->next;
    }
    printf("compress delete error\n" );
    exit(1);
}

int compress_remove_all_ratio()
{
    ASSERT1( disksim->traceformat != ASCII);

    if(disksim->synthgen) return (TRUE); 
    comp_node * tmp = global_comp;
    comp_node * tmp_i = tmp;
    while(tmp) {
        tmp_i = tmp;
        //printf("free %p\n", tmp->ioreq);
        free(tmp_i);
        tmp = tmp->next;
    }
    return (TRUE);
}

/*
 * get ratio of specific blkno
 * return value: positive means success, which should be in the range of 0 to 1, and 1 is accepted, otherwise 0 is not.
 */
double compress_get_ratio(int blkno)
{
    ASSERT1( disksim->traceformat != ASCII);

    if(disksim->synthgen) return ( COMP_DEFAULT_RATIO ); 
    comp_node * tmp = global_comp;
    while(tmp) {
        if( blkno < tmp->blkend && blkno >= tmp->blkstart){
            return (tmp->ratio);
        }
        tmp = tmp->next;
    }
    return (-1);
}

/* init metadate for seg 
 * r_w should be READ or WRITE*/
int compress_seg_init( segment* seg,int n, int r_w)
{
    ASSERT1(seg== NULL || seg->comp_metadata!=NULL);

    seg_comp* p = seg->comp_metadata = (seg_comp*) calloc( 1, sizeof( seg_comp ) );
    p->no = n;
    p->access = (ioreq_event*) calloc( 1, sizeof(ioreq_event) );
    if( r_w==READ ){
        if( COMPRESS_PRINT_DEBUG ){
            printf("Init seg for READ, %s\n",
                COMP_READ_COMPRESS? "TRUE" : "FALSE" );
        }
        p->rw = READ;
        if( COMP_READ_COMPRESS ){
            p->effect = TRUE;
        }else{
            p->effect = FALSE;
        }
    }
    else if( r_w == WRITE ){
        if( COMPRESS_PRINT_DEBUG ){
            printf("Init seg for WRITE, %s\n",
                COMP_WRITE_COMPRESS? "TRUE" : "FALSE" );
        }
        p->rw = WRITE;
        if(COMP_WRITE_COMPRESS){
            p->effect = TRUE;
        }
        else{
            p->effect = FALSE;
        }
    }else{
        ASSERT0("arguement r_w error!");
    }
    p->phy_size = seg->size*512;
    p->phy_left = p->phy_size;
    p->first_addr = 0;
    p->sector_meta_array = NULL;
    p->startblkno = p->endblkno = -1;
    p->first_index=0;
    return (TRUE);
}

/* add sector to segment */
int compress_seg_add( segment* seg, int blkno, int head )
{
    /* sanity check */
    ASSERT1(seg== NULL || seg->comp_metadata==NULL);
    ASSERT1( head>512 || head<=0 );
    ASSERT1(seg->comp_metadata->phy_size<1024);  /* segment size can't be too short */
    ASSERT1(   seg->comp_metadata->sector_meta_array!=NULL && seg->comp_metadata->endblkno!=blkno );
    ASSERT1( seg->comp_metadata->effect==FALSE );
    
    if(COMPRESS_RECORD_DEBUG)
        fprintf( comp_debug_file, "%f compress add: seg %p blkno %d head %d\n",simtime, seg, blkno, head);
    seg_comp* comp = seg->comp_metadata;
    /* first secotr */
    if( comp->sector_meta_array == NULL ){
        comp->sector_meta_array = (array_int_32*) calloc( 1, sizeof(array_int_32) );
        memset(comp->sector_meta_array, 0, sizeof(array_int_32) );
    }
    /* search for the available array */
    array_int_32* tmp = comp->sector_meta_array;
    while( tmp->index == 32 ){
        if( tmp->next == NULL) {
            tmp->next = (array_int_32*) calloc( 1, sizeof(array_int_32) );
            tmp = tmp->next;
            memset(tmp, 0, sizeof(array_int_32) );
            break;
        }
        tmp = tmp->next;
    }
    /* first secotr */
    if( comp->phy_size == comp->phy_left){
        comp->startblkno = comp->endblkno = blkno;
    }

    if( comp->endblkno == blkno ){
        comp->endblkno = blkno;
        comp->endblkno++;
        tmp->array[ tmp->index ] = head;
        tmp->index++;
    }
    else{
        ASSERT1( comp->endblkno != blkno );
    }
    comp->phy_left -= (head+ COMP_HEAD_SIZE );
    while(comp->phy_left<0 ){
        /* sector size + sector head */
        comp->phy_left += ( comp->sector_meta_array->array[ comp->first_index]  + COMP_HEAD_SIZE );
        comp->first_addr += ( comp->sector_meta_array->array[comp->first_index] + COMP_HEAD_SIZE );
        comp->startblkno++ ;
        comp->first_index++;
        /* wrap  */
        if( comp->first_index == 32 ) {
            tmp = comp->sector_meta_array;
            ASSERT1( comp->sector_meta_array->next == NULL );
            comp->sector_meta_array = comp->sector_meta_array->next;
            free(tmp);
            comp->first_index = 0;
        }
        if(comp->first_addr > comp->phy_size ){
            comp->first_addr -= comp->phy_size;
        }
    }
    if(COMPRESS_RECORD_DEBUG)
        fprintf(comp_debug_file, "%f seg add blkno %d head %d , first_addr %d, first_index %d, phy_left%d, start %d, end%d \n",simtime, blkno, head, comp->first_addr, comp->first_index, comp->phy_left, comp->startblkno, comp->endblkno);
   // printf("<--seg add blkno %d head %d , first_addr %d, first_index %d, phy_left%d, start %d, end%d \n", blkno, head, comp->first_addr, comp->first_index, comp->phy_left, comp->startblkno, comp->endblkno);
    return ( TRUE );
}

/* forward the start blkno
 * if blkno>=endblkno, then all data in segment will be cleared,
 * and startblkno and endblkno will be valued to blkno
 */
int compress_seg_forward_startblkno( segment* seg, int blkno)
{
    ASSERT1( seg == NULL );
    ASSERT1( blkno<seg->startblkno );
    ASSERT1( seg->comp_metadata->effect==FALSE );
    if(COMPRESS_RECORD_DEBUG)
        fprintf( comp_debug_file, "%f compress forward: seg %p blkno %d \n",simtime, seg, blkno);

    seg_comp* comp = seg->comp_metadata;
    array_int_32* tmp = comp->sector_meta_array;
    if( blkno >= comp->endblkno ){
        compress_seg_reset( seg, blkno );
        return (TRUE);
    }
    while( comp->startblkno < blkno ){
        /* sector size + sector head */
        comp->phy_left += ( comp->sector_meta_array->array[ comp->first_index]  + COMP_HEAD_SIZE );
        comp->first_addr += ( comp->sector_meta_array->array[comp->first_index] + COMP_HEAD_SIZE );
        comp->startblkno++ ;
        comp->first_index++;
        /* wrap  */
        if( comp->first_index == 32 ) {
            tmp = comp->sector_meta_array;
            ASSERT1( comp->sector_meta_array->next == NULL );
            comp->sector_meta_array = comp->sector_meta_array->next;
            free(tmp);
            comp->first_index = 0;
        }
        if(comp->first_addr > comp->phy_size ){
            comp->first_addr -= comp->phy_size;
        }
    }
    return (TRUE);
}

/* remove all data for newer use
 * set startblkno and endblkno to blkno
 */
int compress_seg_reset( segment* seg, int blkno)
{
    ASSERT1( seg->comp_metadata->effect==FALSE );
    seg_comp* cmp = seg->comp_metadata;
    if(cmp == NULL ){
        return (FALSE);
    }
    if(COMPRESS_RECORD_DEBUG)
        fprintf( comp_debug_file, "%f compress reset: seg %p blkno %d \n",simtime, seg, blkno);
    cmp->startblkno = cmp->endblkno = blkno;
    cmp->phy_size = seg->size*512;
    cmp->phy_left = cmp->phy_size;
    cmp->first_addr = 0;
    cmp->first_index = 0;
    array_int_32* tmp_in_cmp = cmp->sector_meta_array;
    array_int_32* tmp_in_cmp1 = NULL;
    while(tmp_in_cmp && (tmp_in_cmp->next) ){
    	tmp_in_cmp1 = tmp_in_cmp->next;
        free(tmp_in_cmp);
        tmp_in_cmp = tmp_in_cmp1;
    }
    if(tmp_in_cmp){
        free(tmp_in_cmp);
    }
    cmp->sector_meta_array = NULL;
    return (TRUE);
}

/* Get the metadata of specific blkno of seg.
 * if not vaild head, return negative number;
 */
int compress_seg_get_metadata( segment* seg, int blkno)
{
    ASSERT1( seg== NULL );
    ASSERT1( seg->comp_metadata->effect==FALSE );
    ASSERT1( blkno<seg->comp_metadata->startblkno || blkno>=seg->comp_metadata->endblkno);

    seg_comp *cmp = seg->comp_metadata;
    array_int_32 *p = cmp->sector_meta_array;
    int distance = blkno - cmp->startblkno;
    while(distance + cmp->first_index >= 32 ){
        p = p->next;
        ASSERT1( p==NULL );
        distance -= 32;
    }
    return p->array[distance+cmp->first_index];
}

int compress_seg_print_all_metadata( segment *seg)
{
    ASSERT1( seg->comp_metadata->effect==FALSE );
    if( seg==NULL || seg->comp_metadata == NULL ){
        return (FALSE);
    }
    seg_comp* cmp = seg->comp_metadata;
    array_int_32* array = cmp->sector_meta_array;
    int i = 0;  /* index of sector number */
    int i_array = cmp->first_index; /* index inside array */
    int len = cmp->endblkno - cmp->startblkno;
    while(i < len){
        ASSERT1(array == NULL);
        printf("blkno %d -- metadata %d\n", cmp->startblkno+i, array->array[i_array]);
        i++;
        i_array++;
        if(i_array == 32){
            i_array = 0;
            array = array->next;
        }
    }
    return (TRUE);
}

int compress_buffer_sync( segment *seg )
{
    ASSERT1( seg->startblkno > seg->comp_metadata->startblkno );
    ASSERT1( seg->comp_metadata->effect==FALSE );

    if( (seg->startblkno != seg->comp_metadata->startblkno) || (seg->endblkno != seg->comp_metadata->endblkno) ){
        if(COMPRESS_RECORD_DEBUG)
            fprintf( comp_debug_file, "%f compress sync: needed! seg %p seg %d - %d comp %d - %d\n",
                simtime, seg, seg->startblkno, seg->endblkno, seg->comp_metadata->startblkno, seg->comp_metadata->endblkno);
    }
    else{
        if(COMPRESS_RECORD_DEBUG)
            fprintf(comp_debug_file, "%f compress sync: not_need! %d - %d \n",simtime, seg->startblkno, seg->endblkno);
    }
    seg->startblkno = seg->comp_metadata->startblkno;
    seg->endblkno = seg->comp_metadata->endblkno;
    return (TRUE);
}

/* add sector to buffer , detect the ratio
 * seg->endblkno is not changed! only the compressed structure is changed!
 * */
int compress_buffer_add( segment *seg, int blkno)
{
    ASSERT1( seg==NULL );
    if(seg->comp_metadata->effect == FALSE ){
        seg->endblkno = blkno;
        return (TRUE);
    }
    // not read ahead
    if( !(seg->access->flags & BUFFER_BACKGROUND) ){
        compress_seg_add( seg, blkno, 512 );
        return(TRUE);
    }
    // read ahead
    if(COMPRESS_RECORD_DEBUG)
        fprintf( comp_debug_file, "compress buffer add: seg %p blkno %d \n", seg, blkno);
    double ratio = compress_get_ratio( blkno );
    if(ratio == -1) {
        ratio = COMP_DEFAULT_RATIO;
    }
    double comp_size = ratio * 512;
    comp_size = ( (int)comp_size < comp_size )? (int)comp_size+1 : comp_size;
    int count_full_old = seg->comp_metadata->endblkno - seg->comp_metadata->startblkno;
    compress_seg_add( seg, blkno, (int)comp_size );
    int count_full_new = seg->comp_metadata->endblkno - seg->comp_metadata->startblkno;
    if( count_full_old<=seg->size && count_full_new>seg->size){
        compress_seg_reach_full_record( seg );
    }
    return(TRUE);
}

/* destroy all struct  */
int compress_seg_destroy( segment *seg)
{
    seg_comp* cmp = seg->comp_metadata;
    if(cmp == NULL ){
        return (FALSE);
    }
    array_int_32* tmp_in_cmp = cmp->sector_meta_array;
    while( tmp_in_cmp && (tmp_in_cmp->next) ){
        free(tmp_in_cmp);
        tmp_in_cmp = tmp_in_cmp ->next;
    }
    if(tmp_in_cmp){
        free(tmp_in_cmp);
    }
    free( cmp->access );
    free(cmp);
    return (TRUE);
}

inline int compress_seg_check( segment* seg )
{
    if(    seg->startblkno != seg->comp_metadata->startblkno 
        || seg->endblkno != seg->comp_metadata->endblkno ){
        return (FALSE);
    }
    return(TRUE);
}

int compress_seg_print_stats( segment* seg )
{
    ASSERT1( seg== NULL || seg->comp_metadata==NULL );
    if( seg->comp_metadata->effect==FALSE ){
        int blocks = seg->endblkno - seg->startblkno;
        fprintf(comp_debug_file, "SEG %d [%d - %d] %p compress disable! | size %d blocks %d | next %p prev %p reqlist %p | outbcount %d min %d max %d",
            seg->comp_metadata->no, seg->startblkno, seg->endblkno, seg, seg->size, blocks, seg->next, seg->prev, seg->diskreqlist, seg->outbcount, seg->minreadaheadblkno, seg->maxreadaheadblkno );
    }
    else{
        int start = seg->comp_metadata->startblkno;
        int end = seg->comp_metadata->endblkno;
        int p_left = seg->comp_metadata->phy_left;
        int p_size = seg->comp_metadata->phy_size;
        int blocks = end - start;
        float ratio = ((float)( p_size - p_left)) / (blocks*512) ;
        fprintf(comp_debug_file, "SEG %d [%d - %d] outbcount %d | p_left %d ratio %f | next %p prev %p reqlist %p | min %d max %d blocks %d size%d %p : time %f ",
            seg->comp_metadata->no, start, end, seg->outbcount, p_left, ratio, seg->next, seg->prev, seg->diskreqlist, seg->minreadaheadblkno, seg->maxreadaheadblkno, blocks, seg->size, seg, seg->time );
        int compare = blocks - seg->size;
        if(compare>0){
            fprintf(comp_debug_file, "MORE: %d ", compare);
        }
        if(compare<0){
            fprintf(comp_debug_file, "LESS: %d ", compare);
        }
        if(compare==0){
            fprintf(comp_debug_file, "FULL: %d ", compare);
        }
    }
    switch(seg->state){
        case BUFFER_EMPTY:
            fprintf(comp_debug_file, "| state %s ", "BUFFER_EMPTY");
            break;
        case BUFFER_CLEAN:
            fprintf(comp_debug_file, "| state %s ", "BUFFER_CLEAN");
            break;
        case BUFFER_DIRTY:
            fprintf(comp_debug_file, "| state %s ", "BUFFER_DIRTY");
            break;
        case BUFFER_READING:
            fprintf(comp_debug_file, "| state %s ", "BUFFER_READING");
            break;
        case BUFFER_WRITING:
            fprintf(comp_debug_file, "| state %s ", "BUFFER_WRITING");
            break;
        default:
            ASSERT0("seg->state error!");
    }
    switch(seg->outstate){
        case BUFFER_IDLE:
            fprintf(comp_debug_file, "| outstate %s ", "BUFFER_IDLE");
            break;
        case BUFFER_PREEMPT:
            fprintf(comp_debug_file, "| outstate %s ", "BUFFER_PREEMPT");
            break;
        case BUFFER_CONTACTING:
            fprintf(comp_debug_file, "| outstate %s ", "BUFFER_CONTACTING");
            break;
        case BUFFER_TRANSFERING:
            fprintf(comp_debug_file, "| outstate %s ", "BUFFER_TRANSFERING");
            break;
        default:
            ASSERT0("seg->outstate error!");
    }
    switch (seg->comp_metadata->rw ){
        case READ:
            fprintf(comp_debug_file, "| %s ", "READ");
            break;
        case WRITE:
            fprintf(comp_debug_file, "| %s ", "WRITE");
            break;
    }
    switch (seg->comp_metadata->effect){
        case TRUE:
            fprintf(comp_debug_file, "TRUE");
            break;
        case FALSE:
            fprintf(comp_debug_file, "FALSE");
            break;
    }
    fprintf(comp_debug_file, "\n");
    return (TRUE);
}

int compress_seg_print_stats_extra( segment* seg, char* msg)
{
    fprintf(comp_debug_file, "%s ", msg);
    compress_seg_print_stats(seg);
    return (TRUE);
}

int compress_seg_print_all_stats(disk* currdisk)
{
    ASSERT1(currdisk==NULL);
    segment* tmpseg = currdisk->seglist;
    fprintf(comp_debug_file, "           -----compress_seg_print_all_stats-----\n");
    while( tmpseg ) {
        compress_seg_print_stats(tmpseg);
        tmpseg = tmpseg->next;
    }
    fprintf(comp_debug_file, "           --------------------------------------\n");
    return (TRUE);
}

/* get the total size of blocks from [start] to [end]
 * block number [end] is not calculated 
 * the size comprise head size and data size specified by head
 * */
int compress_seg_get_size(segment* seg, int start, int end )
{
    ASSERT1(end<=start);
    ASSERT1( seg== NULL || seg->comp_metadata==NULL );
    ASSERT1( start<seg->comp_metadata->startblkno || start>=seg->comp_metadata->endblkno);
    ASSERT1( end>seg->comp_metadata->endblkno );

    int count = 0;
    int i;
    for(i=start; i<end; i++){
        count += compress_seg_get_metadata(seg, i) + COMP_HEAD_SIZE;
    }
    return (count);
}

inline int compress_seg_get_phy_left(segment* seg)
{
    ASSERT1( seg== NULL || seg->comp_metadata==NULL );
    return (seg->comp_metadata->phy_left);
}

inline int compress_seg_left_enough(segment* seg)
{
    ASSERT1( seg== NULL || seg->comp_metadata==NULL );
    if(seg->comp_metadata->phy_left >= COMP_MIN_LEFT){
        return ( TRUE );
    } else{
        return ( FALSE );
    }
}

inline int compress_seg_get_sector_count(segment* seg)
{
    ASSERT1( seg== NULL || seg->comp_metadata==NULL );
    return (seg->comp_metadata->endblkno - seg->comp_metadata->startblkno);
}

inline int compress_seg_get_endblkno( segment* seg )
{
    ASSERT1( seg== NULL || seg->comp_metadata==NULL );
    return seg->comp_metadata->endblkno;
}

inline int compress_seg_get_startblkno( segment* seg )
{
    ASSERT1( seg== NULL || seg->comp_metadata==NULL );
    return seg->comp_metadata->startblkno;
}

inline int compress_seg_set_outblkno(segment* seg, int blkno )
{
    ASSERT1( seg== NULL || seg->comp_metadata==NULL );
    ASSERT1( blkno<seg->comp_metadata->startblkno || blkno>=seg->comp_metadata->endblkno);
    
    seg->comp_metadata->outblkno = blkno;
    return (TRUE);
}

inline int compress_seg_get_outblkno(segment* seg )
{
    ASSERT1( seg== NULL || seg->comp_metadata==NULL );
    
    return seg->comp_metadata->outblkno ;
}

/* [flag] should be READ or WRITE */
int compress_seg_judge_comperssion( segment* seg)
{
    ASSERT1( seg== NULL || seg->comp_metadata==NULL );
    int rw = seg->comp_metadata->rw;
    if( rw == READ && COMP_READ_COMPRESS ){
        compress_seg_set_enable( seg, TRUE);
        return TRUE;
    }
    else if( rw == READ && ! COMP_READ_COMPRESS ){
        compress_seg_set_enable( seg, FALSE);
        return FALSE;
    }
    else if( rw == WRITE && COMP_WRITE_COMPRESS){
        compress_seg_set_enable( seg, TRUE);
        return TRUE;
    }
    else if( rw == WRITE && ! COMP_WRITE_COMPRESS){
        compress_seg_set_enable( seg, FALSE);
        return FALSE;
    }
    else{
        ASSERT0("Compress judge error!");
    }
}

/* [flag] should be TRUE or FALSE */
int compress_seg_set_enable( segment* seg, int flag )
{
    ASSERT1( seg== NULL || seg->comp_metadata==NULL );
    if( flag == TRUE ){
        if(COMPRESS_RECORD_DEBUG)
            fprintf( comp_debug_file, "compress set: seg %p TRUE\n", seg);
        seg->comp_metadata->effect = TRUE;
        return (TRUE);
    }
    else if( flag == FALSE){
        if(COMPRESS_RECORD_DEBUG)
            fprintf( comp_debug_file, "compress set: seg %p FALSE\n", seg);
        seg->comp_metadata->effect = FALSE;
        return (TRUE);
    }
    else{
        ASSERT2( 1, "flag should be TRUE or FALSE");
    }
    return (FALSE);
}

inline int compress_mechanism_is_enable()
{
    if( COMP_READ_COMPRESS || COMP_WRITE_COMPRESS ) return TRUE;
    return FALSE;
}

inline int compress_seg_is_enable( segment* seg )
{
    ASSERT1( seg== NULL || seg->comp_metadata==NULL );
    ASSERT1(seg->comp_metadata->effect!=TRUE && seg->comp_metadata->effect!=FALSE);

    return (seg->comp_metadata->effect);
}

int compress_seg_for_write( segment* seg)
{
    ASSERT1( seg== NULL || seg->comp_metadata==NULL );

    if(seg->comp_metadata->rw == WRITE ){
        return TRUE;
    }
    else if(seg->comp_metadata->rw == READ){
        return FALSE;
    }
    else {
        ASSERT0( "rw should be WRITE or READ!");
    }
}

/*
 * copy the content pointed by [curr] to comp->access
 * the max_readahead block number should be dynamically changed during compression
 */
int compress_seg_get_max_readahead( disk* currdisk, segment* seg, ioreq_event* curr)
{
    ASSERT1( seg == NULL || seg->comp_metadata==NULL);
    ASSERT1( curr==NULL && seg->comp_metadata->access==NULL );
    seg_comp* comp = seg->comp_metadata; 
    ioreq_event* tmp;

    tmp = comp->access;
    if(curr!=NULL) {
        memcpy(comp->access, curr, sizeof(ioreq_event));
    }
    else{
        tmp = comp->access;
    }
    int readahead = disk_buffer_get_max_readahead(currdisk,seg,tmp);
    if(COMPRESS_RECORD_DEBUG)
        fprintf(comp_debug_file, "compress get maxreadhead %f seg %p curr %p max %d | nex max %d\n", simtime, seg, curr, seg->maxreadaheadblkno, readahead );
    return readahead;
}

/* return wrong value...!!! */
double compress_seg_get_read_time( int devno, segment* seg, int blkno)
{
    ASSERT1( seg== NULL || seg->comp_metadata==NULL );
    ASSERT1( seg->comp_metadata->effect==FALSE );
    ASSERT1( blkno<seg->comp_metadata->startblkno || blkno>=seg->comp_metadata->endblkno);

    double disktime;
    if( compress_seg_get_metadata(seg, blkno) < 512 ){
        disktime = ( COMP_READ_TIME > (getdisk(devno))->blktranstime )
            ? COMP_READ_TIME
            : (getdisk(devno))->blktranstime ;
    }else{
        disktime = (getdisk(devno))->blktranstime ;
    }
    return disktime;
}

int compress_print_diskreq_info( diskreq* tmpreq)
{
    ASSERT1(tmpreq==NULL);
    fprintf(comp_debug_file, "diskreq stats: %p : outblkno %d inblkno %d ioreqlist %p | seg %p seg_next %p bus_next %p | watermark %d  overhead_done %f | flags: ",
        tmpreq, tmpreq->outblkno, tmpreq->inblkno, tmpreq->ioreqlist, tmpreq->seg, tmpreq->seg_next, tmpreq->bus_next, tmpreq->watermark, tmpreq->overhead_done);
    if(tmpreq->flags & SEG_OWNED) {
        fprintf(comp_debug_file, "SEG_OWNED ");
    }
    if(tmpreq->flags & HDA_OWNED) {
        fprintf(comp_debug_file, "HDA_OWNED ");
    }
    if(tmpreq->flags & EXTRA_WRITE_DISCONNECT) {
        fprintf(comp_debug_file, "EXTRA_WRITE_DISCONNECT ");
    }
    if(tmpreq->flags & COMPLETION_SENT) {
        fprintf(comp_debug_file, "COMPLETION_SENT ");
    }
    if(tmpreq->flags & COMPLETION_RECEIVED) {
        fprintf(comp_debug_file, "COMPLETION_RECEIVED ");
    }
    if(tmpreq->flags & FINAL_WRITE_RECONNECTION_1) {
        fprintf(comp_debug_file, "FINAL_WRITE_RECONNECTION_1 ");
    }
    if(tmpreq->flags & FINAL_WRITE_RECONNECTION_2) {
        fprintf(comp_debug_file, "FINAL_WRITE_RECONNECTION_2 ");
    }
    fprintf(comp_debug_file, " | hittype: ");
    switch(tmpreq->hittype){
        case BUFFER_COLLISION:
            fprintf(comp_debug_file, "BUFFER_COLLISION ");
            break;
        case BUFFER_NOMATCH:
            fprintf(comp_debug_file, "BUFFER_NOMATCH ");
            break;
        case BUFFER_WHOLE:
            fprintf(comp_debug_file, "BUFFER_WHOLE ");
            break;
        case BUFFER_PARTIAL:
            fprintf(comp_debug_file, "BUFFER_PARTIAL ");
            break;
        case BUFFER_PREPEND:
            fprintf(comp_debug_file, "BUFFER_PREEMPT ");
            break;
        case BUFFER_APPEND:
            fprintf(comp_debug_file, "BUFFER_APPEND ");
            break;
        default:
            ASSERT0("hittype error!");
    }
    fprintf(comp_debug_file, "\n");
    char* s = "    ";
    compress_print_ioreq_info_extra(tmpreq->ioreqlist, s);
    return (TRUE);
}

/* if the seg is compressed to store more blocks than it can, record it.
 * Don't frequently call this function for same segment. 
 * You should call this just at the moment the seg reaches full.
 * */
int compress_seg_reach_full_record( segment* seg)
{
    ASSERT1(seg==NULL || seg->comp_metadata==NULL);
    ASSERT1( seg->comp_metadata->effect==FALSE );

    seg_comp* comp = seg->comp_metadata;
    if(comp->endblkno <= comp->startblkno+seg->size){
        return (FALSE);
    }
    compress_global->comp_full++;
    int blocks = comp->endblkno - comp->startblkno;
    if(COMPRESS_RECORD_DEBUG)
        fprintf(comp_debug_file, "seg reach full: seg %p | start %d end %d blocks %d| total: %d\n",
            seg, comp->startblkno, comp->endblkno, blocks, compress_global->comp_full);
    return (TRUE);
}

/* Currently, only support BUFFER_WHOLE and BUFFER_PARTIAL for read
 * Only count the occurrence where the part exceed the the original size of segment is hit.
 */
int compress_seg_hit_full_check(segment* seg, ioreq_event *curr, int hittype)
{
    ASSERT1( seg == NULL || curr==NULL);
    ASSERT1( seg->comp_metadata->effect==FALSE );

    if(hittype!=BUFFER_WHOLE && hittype!=BUFFER_PARTIAL ){
        return (FALSE);
    }
    if(    curr->blkno >= (seg->startblkno+seg->size) 
        && curr->blkno <=seg->endblkno
        && seg->endblkno > seg->startblkno+seg->size)
    {
        compress_global->comp_hits++;
        int blocks = seg->endblkno - seg->startblkno;
        if(COMPRESS_RECORD_DEBUG)
            fprintf(comp_debug_file, "%f io hit full: seg %p ioreq %p blkno %d | seg start %d end %d blocks %d | total: %d \n",
                simtime, seg, curr, curr->blkno, seg->startblkno, seg->endblkno, blocks, compress_global->comp_hits);
        return (TRUE);
    }
    return (FALSE);
}

int compress_print_diskreq_info_extra( diskreq* tmpreq, char* msg)
{
    fprintf(comp_debug_file, "%s ", msg);
    compress_print_diskreq_info(tmpreq);
    return (TRUE);
}

int compress_print_disk_info(disk* currdisk)
{
    ASSERT1(currdisk==NULL);
    fprintf(comp_debug_file, "disk stats: %p : effectivebus %p currentbus %p effectivehda %p currenthda %p pendxfer %p | outstate: ",
        currdisk, currdisk->effectivebus, currdisk->currentbus, currdisk->effectivehda, currdisk->currenthda, currdisk->pendxfer);
    switch(currdisk->outstate ){
        case DISK_IDLE:
            fprintf(comp_debug_file, "DISK_IDLE ");
            break;
        case DISK_TRANSFERING:
            fprintf(comp_debug_file, "DISK_TRANSFERING ");
            break;
        case DISK_WAIT_FOR_CONTROLLER:
            fprintf(comp_debug_file, "DISK_WAIT_FOR_CONTROLLER ");
            break;
    }
    fprintf(comp_debug_file, "\n");
    return (TRUE);
}

void compress_print_ioreq_info_extra( ioreq_event* curr, char* s)
{
    ioreq_event* tmp = curr;
    while(tmp!=NULL){
        fprintf(comp_debug_file, "%s%f ioreq %p:type %d blkno %d bcount %d cause %d %s | next %p\n",
            s, tmp->time, tmp, tmp->type, tmp->blkno, tmp->bcount, tmp->cause, (curr->flags & BUFFER_BACKGROUND)?"BACKGROUND":"FORE", tmp->next);
        tmp = tmp->next;
    }
}

void compress_print_ioreq_info( ioreq_event* curr)
{
    char* s = "";
    compress_print_ioreq_info_extra( curr, s);
}


/* ***********stats setting function****************** */

int compress_stats_add_ioreq( ioreq_event *ioreq )
{
    ASSERT1( ioreq==NULL );
    ASSERT1( outios==NULL );
    
    if(COMPRESS_PRINT_DEBUG) printf("compress stats add %p blkno %d\n", ioreq, ioreq->blkno);
    comp_stats* temp = compress_global->comp_stats_global;
    comp_stats *temp1 = malloc( sizeof(comp_stats) );
    memset( temp1, 0, sizeof(comp_stats));
    temp1 ->ioreq = ioreq;
    temp1 ->time = ioreq->time;
    temp1 ->blkno = ioreq->blkno;
    temp1 ->bcount = ioreq->bcount;
    temp1->next = NULL;
    if(compress_global->comp_stats_global == NULL ){
        compress_global->comp_stats_global = temp1;
        return TRUE;
    }
    while( temp->next ){
        if( temp->ioreq == ioreq ){
            printf("compress add stats %p :duplicate ioreq %p\n", temp, ioreq );
            exit(1);
        }
        temp = temp->next;
    }
    if( temp->ioreq == ioreq ){
        printf("compress add stats %p :duplicate ioreq %p\n", temp, ioreq );
        exit(1);
    }
    temp->next = temp1;
    return TRUE;
}


void compress_stats_set_activetime( ioreq_event *ioreq, double t)
{
    ASSERT1( ioreq==NULL );
    ASSERT1( outios==NULL );

    comp_stats *temp = compress_global->comp_stats_global;
    while( temp->ioreq != ioreq ){
        temp = temp->next;
        if(temp==NULL){
            printf("compress stats set activetime error\n");
            exit(1);
        }
    }
    if(temp->active_time ==0 ){
        temp->seg = ( (diskreq*) (ioreq->ioreq_hold_diskreq))->seg;
        temp->seg_start = temp->seg->startblkno;
        temp->seg_end = temp->seg->endblkno;
        temp->hittype = ( (diskreq*) (ioreq->ioreq_hold_diskreq))->hittype;
        temp->active_time = t;
    }
}

void compress_stats_set_completetime( ioreq_event *ioreq, double t)
{
    ASSERT1( ioreq==NULL );
    ASSERT1( outios==NULL );

    comp_stats *temp = compress_global->comp_stats_global;
    while( temp->ioreq != ioreq ){
        temp = temp->next;
        if(temp==NULL){
            printf("compress stats set completetime error\n");
            exit(1);
        }
    }
    temp->complete_time = t;
    temp->complete_active = t - temp->active_time;
    temp->complete_come = t - temp->time;
}

void compress_stats_set_seekoverhead( ioreq_event *ioreq, double t)
{
    ASSERT1( ioreq==NULL );
    ASSERT1( outios==NULL );

    comp_stats *temp = compress_global->comp_stats_global;
    while( temp->ioreq != ioreq ){
        temp = temp->next;
        if(temp==NULL){
            printf("compress stats set completetime error\n");
            exit(1);
        }
    }
    temp->seek_overhead = t;
}

int compress_stats_remove_ioreq( ioreq_event *ioreq)
{
    ASSERT1( ioreq==NULL );
    ASSERT1( outios==NULL );

    comp_stats *temp1 = compress_global->comp_stats_global;
    comp_stats *temp = compress_global->comp_stats_global;
    while( temp->ioreq != ioreq ){
        temp = temp->next;
        if(temp==NULL){
            printf("compress stats set completetime error\n");
            exit(1);
        }
    }
    if(temp == temp1){
        compress_global->comp_stats_global = temp->next;
        free(temp);
        return TRUE;
    }
    while( temp1->next != temp && temp1->next != NULL ) temp1 = temp1->next;
    ASSERT2(temp1->next==NULL, "stats remove not found");
    temp1->next = temp->next;
    free(temp);
    return TRUE;
}

void compress_stats_print_ioreq( ioreq_event* ioreq)
{
    ASSERT1( ioreq==NULL );
    ASSERT1( outios==NULL );

    comp_stats *temp = compress_global->comp_stats_global;
    while( temp->ioreq != ioreq ){
        temp = temp->next;
        if(temp==NULL){
            printf("compress stats set completetime error\n");
            exit(1);
        }
    }
    printf("%f %d %d[%d %d]%dhit=%d | activetime %f seek %f\n",
        temp->complete_time, temp->blkno, temp->bcount, temp->seg_start, temp->seg_end, temp->seg->comp_metadata->no,
        temp->hittype, temp->active_time, temp->seek_overhead);
    disksim_exectrace("%f %d %d[%d %d]%dhit=%d | from_come %f from_active %f activetime %f| seek %f\n",
        temp->complete_time, temp->blkno, temp->bcount, temp->seg_start, temp->seg_end, temp->seg->comp_metadata->no, temp->hittype,
        temp->complete_come, temp->complete_active, temp->active_time, temp->seek_overhead);
}

void compress_print_stats()
{
    printf("======Compress Stats======\n");
    printf("Compress reaches full: %d\n", compress_global->comp_full);
    printf("Compress hits full: %d\n", compress_global->comp_hits);
    printf("==========================\n");
}
