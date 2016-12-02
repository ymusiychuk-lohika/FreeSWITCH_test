/****************************************************************************
 * vid_delay_analyzer.h
 *
 * Copyright (C) Bluejeans Network, Inc  All Rights Reserved
 *
 * Author: Brian Ashford
 * Date:   07/25/2013
 *****************************************************************************/

#ifndef VIDDELAYANALYZER_H_
#define VIDDELAYANALYZER_H_

#include <stdint.h>
#include <pj/types.h>
#include <pj/pool.h>
#include <pj/file_io.h>

#include <pjmedia/delayanalyzer.h>

/* for logging purposes */
typedef struct _vid_delay_logfile_info {
    pj_oshandle_t	    logfile_handle;
    pj_bool_t		    logfile_open;
    pj_bool_t		    logfile_open_tried;
    double              delay_stats_loss;
    pj_uint32_t         delay_stats_bytes;
    pj_uint64_t         delay_stats_last_time;
    pj_uint32_t         delay_stats_bw;
} vid_delay_logfile_info;

void vid_delay_logfile_open(pj_pool_t* pool, struct _vid_delay_logfile_info* info, 
    pj_str_t* logpath, pj_str_t* guid);
void vid_delay_logfile_close(vid_delay_logfile_info* info);
void vid_delay_logfile_dump(void* analyzer, vid_delay_logfile_info* info);
void vid_delay_logfile_set_stats(vid_delay_logfile_info* info, 
    void* rtcp, unsigned len);

#endif /* VIDDELAYANALYZER_H_ */
