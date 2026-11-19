#ifndef _MAIN_H_
#define _MAIN_H_

#include "simengine.h"
#include "simparameters.h"

typedef double Time;

typedef struct _simulation_run_data_
{
  long int packets_processed;
  unsigned random_seed;
  long int total_sends;
  int last_packet_id;
  double efficiency;
  double data_transmission_failure_probability;
  double ack_transmission_failure_probability;
  double timeout_interval;
} Simulation_Run_Data, * Simulation_Run_Data_Ptr;

/**********************************************************************/

/*
 * Function prototypes
 */

int
main(void);

/**********************************************************************/

#endif /* main.h */