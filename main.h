#ifndef _MAIN_H_
#define _MAIN_H_

#include "simengine.h"
#include "simparameters.h"
#include "channel.h"

typedef double Time;

typedef struct _station_
{
  int id;
  int last_packet_id;
  Window_Entry * send_window;
  int send_window_size;
  long int packet_count;
  double accumulated_delay;
  double mean_delay;
} Station, * Station_Ptr;

/**********************************************************************/

typedef struct _simulation_run_data_
{
  Station_Ptr stations;
  Channel_Ptr channel;
  Fifoqueue_Ptr successfully_reserved_queue;
  long int packets_processed;
  unsigned random_seed;
  long int total_sends;
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