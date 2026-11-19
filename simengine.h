#ifndef _SIMLIB_H_
#define _SIMLIB_H_

/******************************************************************************/

#include <stdlib.h>
#include <stdbool.h>
#include "simparameters.h"

/******************************************************************************/

/*
 * Declare the objects below so that the typedef ordering does not
 * matter.
 */

struct _simulation_run_;
struct _clock_;
struct _event_;
struct _event_container_;
struct _eventlist_;
struct _rand_stream_;
struct _window_entry_;
struct _window_data_;
struct packet;

typedef struct _simulation_run_
{
  struct _eventlist_ * eventlist;
  struct _clock_ * clock;
  struct _window_data_ * windowdata;
  void * data;
} Simulation_Run, * Simulation_Run_Ptr;

typedef struct _clock_
{
  double time;
} Clock, * Clock_Ptr;

typedef struct packet
{
  int id;
} Packet;

typedef struct _event_
{
  const char * description;
  void (* function)(struct _simulation_run_*, Packet);
  Packet attachment;
} Event, * Event_Ptr;

typedef struct _event_container_
{
  struct _event_ event;
  double occurrence_time;
  void * data_ptr;
  long int event_id;
  struct _event_container_ * next_container;
  struct _event_container_ * previous_container;
} Event_Container, * Event_Container_Ptr;

typedef struct _eventlist_
{
  struct _event_container_ * front_ptr;
  struct _event_container_ * back_ptr;
  int size;
} Eventlist, * Eventlist_Ptr;

typedef struct _rand_stream_
{
  unsigned rand_max;
  unsigned seed;
  unsigned next;
} Rand_Stream, * Rand_Stream_Ptr;

typedef struct _window_entry_
{
  int id;
  bool processed;
  bool acked;
  int transmission_count;
} Window_Entry, * Window_Entry_Ptr;

typedef struct _window_data_
{
  Window_Entry * send_window;
  Window_Entry receive_window[RECEIVE_WINDOW_SIZE];
  int send_window_size;
} Window_Data, * Window_Data_Ptr;

Simulation_Run_Ptr simulation_run_new(int send_window_size);
double simulation_run_get_time(Simulation_Run_Ptr);
void * simulation_run_data(Simulation_Run_Ptr);
void simulation_run_set_data(Simulation_Run_Ptr, void *);
long int simulation_run_schedule_event(Simulation_Run_Ptr, Event, double);
void * simulation_run_deschedule_event(Simulation_Run_Ptr, long int);
void simulation_run_execute_event(Simulation_Run_Ptr);
void simulation_run_free_memory(Simulation_Run_Ptr);

Rand_Stream_Ptr rand_stream_new(unsigned);
void rand_stream_initialize(Rand_Stream_Ptr, unsigned);
unsigned rand_stream_get(Rand_Stream_Ptr);
double rand_stream_uniform_generator(Rand_Stream_Ptr);
double rand_stream_exponential_generator(Rand_Stream_Ptr, double);

void random_generator_initialize(unsigned);
double uniform_generator(void);
double exponential_generator(double);

void * xmalloc(unsigned);
void * xcalloc(unsigned, unsigned);
void xfree(void *);

#endif /* _SIMLIB_H_ */