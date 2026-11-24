#ifndef _SIMPARAMETERS_H_
#define _SIMPARAMETERS_H_

#define SEND_WINDOW_SIZE_LIST 11 // Ws values to simulate
#define RECEIVE_WINDOW_SIZE 13 //Wr
#define NUMBER_STATIONS 2
#define NUM_ID_BITS 8 // m, 2^m must be at least the largest Ws plus Wr
#define DATA_TRANSMISSION_FAILURE_PROBABILITY_LIST 0.05 // pd values to simulate
#define ACK_TRANSMISSION_FAILURE_PROBABILITY_LIST 0.05 // pa values to simulate

#define DATA_TRANSMISSION_TIME 0.002 //seconds
#define ACK_TRANSMISSION_TIME 0.001 //seconds
#define DATA_PROPAGATION_DELAY 0.005 //seconds
#define TIMEOUT_INTERVAL_LIST 0.02 // timeout intervals to simulate, in seconds

#define GUARD_TIME 0.0001 // Guard time for slot timing, in seconds
#define RESERVATION_SLOT_DURATION 0.0005 // Duration of a time slot, in seconds
#define RESERVATION_SLOT_NO_GUARD_DURATION (RESERVATION_SLOT_DURATION - 2 * GUARD_TIME) // Duration of a time slot without guard time, in seconds

#define NUMBER_BACKLOGGED_PACKETS 110000 // Max number of packets simulation can run
#define NUMBER_PACKETS_TO_SEND 10000 // Number of packets to send in simulation

#define TOTAL_PACKET_DURATION (DATA_TRANSMISSION_TIME * 8 + RESERVATION_SLOT_DURATION * 8)
#define RESERVATION_DURATION (RESERVATION_SLOT_DURATION * 8)

/* Comma separated list of random seeds to run. */
#define RANDOM_SEED_LIST 400443520, 400439508, 400435424

#endif /* simparameters.h */