#ifndef DISKSIM_COMPRESS_H
#define DISKSIM_COMPRESS_H

#include "disksim_global.h"
#include "disksim_disk.h"

typedef struct seg segment;
typedef struct disk disk; 
typedef struct diskreq_t diskreq;

/* used for record ratio for ioreq */
typedef struct comp_n{
    ioreq_event* ioreq;
    int blkstart;
    int blkend;      /* blkstart <= vaild range <blkend*/
    double ratio;
    struct comp_n* next;
} comp_node;

typedef struct array_int_node_32{
    int index; /* point to the vaild position, 32 is out of range */
    int array[32]; /* store head of sector consecutively*/ 
    struct array_int_node_32 *next;
} array_int_32;

/* new array is linked to the end of sector_meta_array
 * the first_index point to the first useable data of the first array.
 * If first_index is 32, the first array is exhausted, and will be free to give way to next array by which linked. first_index will be set to 0 correspondingly.
 * */
typedef struct seg_comp_data{
    int no;
    int effect;        /* TRUE or FALSE */
    int startblkno;    /* should be equal to the value of same member of segment */
    int endblkno; /* same to above */
    int phy_size;
    int phy_left;
    int first_addr;    /* the physical address first sector in segment */
    int first_index;  /* the first usable member of array */
    ioreq_event *access;   /* the copy of tracked request */
    int outblkno;          /* the first blkno when disk transfer out blocks */
    int rw;                /* can be used by write? or only read? */
                           /* value: READ / WRITE */
    array_int_32 *sector_meta_array;   /*stores the metadata, linke one by one*/
} seg_comp;

typedef struct comp_stats_data{
    ioreq_event *ioreq;
//    diskreq *disk_req;
//    ioreq_event *access;
    segment *seg;
    int seg_start;
    int seg_end;
    int hittype;
    int blkno;
    int bcount;
    double time;
    double active_time;
    double complete_time;
    double complete_active;      /* from coompete to active */
    double complete_come;    /* from come time to completion */
    double seek_overhead;
    struct comp_stats_data *next;
} comp_stats;

typedef struct {
    int comp_hits;
    int comp_full;
    comp_stats *comp_stats_global;
} compress_struct;

int compress_init();
int compress_destroy();
void compress_debug( const char* filename,const char* function, int line);
int compress_print_0(char* msg);
int compress_set_ratio_ioreq( int start, int end, ioreq_event* curr, double ratio);
int compress_remove_ratio_ioreq( ioreq_event *curr);
int compress_remove_all_ratio();
double compress_get_ratio(int blkno);
int compress_seg_init( segment* seg,int n, int r_w);
int compress_seg_add( segment* seg, int blkno, int head );
int compress_seg_forward_startblkno( segment* seg, int blkno);
int compress_seg_reset( segment* seg, int blkno);
int compress_seg_get_metadata( segment* seg, int blkno);
int compress_seg_print_all_metadata( segment *seg);
int compress_buffer_sync( segment *seg );
int compress_buffer_add( segment *seg, int blkno);
int compress_seg_destroy( segment *seg);
int compress_seg_check( segment* seg );
int compress_seg_print_stats( segment* seg );
int compress_seg_print_stats_extra( segment* seg, char* msg);
int compress_seg_print_all_stats(disk* currdisk);
int compress_seg_get_size(segment* seg, int start, int end );
int compress_seg_get_phy_left(segment* seg);
int compress_seg_left_enough(segment* seg);
int compress_seg_get_sector_count(segment* seg);
int compress_seg_get_endblkno( segment* seg );
int compress_seg_get_startblkno( segment* seg );
int compress_seg_set_outblkno(segment* seg, int blkno );
int compress_seg_get_outblkno(segment* seg );
int compress_seg_judge_comperssion( segment* seg);
int compress_seg_set_enable( segment* seg, int flag );
int compress_mechanism_is_enable();
int compress_seg_is_enable( segment* seg );
int compress_seg_for_write( segment* seg);
int compress_seg_get_max_readahead( disk* currdisk, segment* seg, ioreq_event* curr);
int compress_print_diskreq_info( diskreq* tmpreq);
int compress_seg_reach_full_record( segment* seg);
int compress_seg_hit_full_check(segment* seg, ioreq_event *curr, int hittype);
int compress_print_diskreq_info_extra( diskreq* tmpreq, char* msg);
int compress_print_disk_info(disk* currdisk);
void compress_print_ioreq_info_extra( ioreq_event* curr, char* s);
void compress_print_ioreq_info( ioreq_event* curr);
int compress_stats_add_ioreq( ioreq_event *ioreq );
void compress_stats_set_activetime( ioreq_event *ioreq, double t);
void compress_stats_set_completetime( ioreq_event *ioreq, double t);
void compress_stats_set_seekoverhead( ioreq_event *ioreq, double t);
int compress_stats_remove_ioreq( ioreq_event *ioreq);
void compress_stats_print_ioreq( ioreq_event* ioreq);
void compress_print_stats();
#endif
