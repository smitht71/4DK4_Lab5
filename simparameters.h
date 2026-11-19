#ifndef _SIMPARAMETERS_H_
#define _SIMPARAMETERS_H_

#define SEND_WINDOW_SIZE_LIST 1, 3, 5, 7, 9, 11, 15, 19, 23, 27, 31, 35, 39 // Ws values to simulate
#define RECEIVE_WINDOW_SIZE 10 //Wr
#define NUM_ID_BITS 8 // m, 2^m must be at least the largest Ws plus Wr
#define DATA_TRANSMISSION_FAILURE_PROBABILITY_LIST 0.05 // pd values to simulate
#define ACK_TRANSMISSION_FAILURE_PROBABILITY_LIST 0.05 // pa values to simulate

#define DATA_TRANSMISSION_TIME 0.001 //seconds
#define ACK_TRANSMISSION_TIME 0.0 //seconds
#define DATA_PROPAGATION_DELAY 0.005 //seconds
#define TIMEOUT_INTERVAL_LIST 0.015 // timeout intervals to simulate, in seconds

#define NUMBER_BACKLOGGED_PACKETS 110000 // Max number of packets simulation can run
#define NUMBER_PACKETS_TO_SEND 10 // Number of packets to send in simulation

/* Comma separated list of random seeds to run. */
#define RANDOM_SEED_LIST 400443520, 400439508, 400435424

#endif /* simparameters.h */