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
struct _packet_;
struct _fifoqueue_;
struct _queue_container_;
struct _server_;

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

typedef enum {WAITING, TRANSMITTING} Packet_Status;

typedef struct _packet_ 
{
  double arrive_time;
  double service_time;
  int station_id;
  int id;
  Packet_Status status;
} Packet, * Packet_Ptr;

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

typedef struct _window_entry_
{
  int id;
  bool processed;
  bool acked;
  int transmission_count;
  bool reservation_sent;
} Window_Entry, * Window_Entry_Ptr;

typedef struct _window_data_
{
  Window_Entry receive_window[RECEIVE_WINDOW_SIZE];
} Window_Data, * Window_Data_Ptr;

typedef struct _rand_stream_
{
  unsigned rand_max;
  unsigned seed;
  unsigned next;
} Rand_Stream, * Rand_Stream_Ptr;

/*
 * FIFO queue object keeps the queue size and contains pointers to containers
 * at the front and back of the queue. The queue container objects are kept on
 * a singly linked list. Each container has a content pointer that tracks the
 * object placed on the FIFO queue.
 */

struct _queue_container_;

typedef struct _fifoqueue_
{
  struct _queue_container_ * front_ptr;
  struct _queue_container_ * back_ptr;
  int size;
} Fifoqueue, * Fifoqueue_Ptr;

typedef struct _queue_container_
{
  void * content_ptr;
  struct _queue_container_ * next_ptr;
} Queue_Container, *Queue_Container_Ptr;


/*
 * Server object and definitions. 
 */

typedef enum{FREE, BUSY} Server_State;

typedef struct _server_
{
  Server_State state;
  void * customer_in_service;
} Server, * Server_Ptr;

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