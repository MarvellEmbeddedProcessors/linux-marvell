#ifndef PESQ_MAIN_H
#define PESQ_MAIN_H

#include <stdio.h>
#include <math.h>
#include "pesq.h"
#include "dsp.h"
#define ITU_RESULTS_FILE          "pesq_results.txt"

int pesq_main (int argc, const char *argv []);
void usage (void);
void pesq_measure (SIGNAL_INFO * ref_info, SIGNAL_INFO * deg_info,
    ERROR_INFO * err_info, long * Error_Flag, char ** Error_Type);


#endif