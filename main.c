#include <stdio.h>
#include "simengine.h"
#include "simparameters.h"
#include "transmission.h"
#include "trace.h"
#include "main.h"


int
main(void)
{
  unsigned random_seed;
  int SEND_WINDOW_SIZES[] = {SEND_WINDOW_SIZE_LIST};
  double DATA_TRANSMISSION_FAILURE_PROBABILITIES[] = {
    DATA_TRANSMISSION_FAILURE_PROBABILITY_LIST
  };
  double ACK_TRANSMISSION_FAILURE_PROBABILITIES[] = {
    ACK_TRANSMISSION_FAILURE_PROBABILITY_LIST
  };
  double TIMEOUT_INTERVALS[] = {TIMEOUT_INTERVAL_LIST};
  unsigned RANDOM_SEEDS[] = {RANDOM_SEED_LIST, 0};
  int window_count = sizeof(SEND_WINDOW_SIZES) / sizeof(SEND_WINDOW_SIZES[0]);
  int probability_count = sizeof(DATA_TRANSMISSION_FAILURE_PROBABILITIES) /
                         sizeof(DATA_TRANSMISSION_FAILURE_PROBABILITIES[0]);
  int ack_probability_count =
    sizeof(ACK_TRANSMISSION_FAILURE_PROBABILITIES) /
    sizeof(ACK_TRANSMISSION_FAILURE_PROBABILITIES[0]);
  int timeout_interval_count =
    sizeof(TIMEOUT_INTERVALS) / sizeof(TIMEOUT_INTERVALS[0]);
  int seed_count = 0;
  int window_index, probability_index, ack_probability_index;
  int timeout_interval_index, seed_index, i;
  Simulation_Run_Ptr simulation_run;
  Simulation_Run_Data data;
  double elapsed_time, elapsed_time_sum, average_elapsed_time;
  double efficiency_sum, average_efficiency;
  double sends_per_success_sum, average_sends_per_success;
  double *seed_elapsed_times, *seed_efficiencies, *seed_sends_per_success;
  FILE *csv_file;

  while (RANDOM_SEEDS[seed_count] != 0) {
    seed_count++;
  }

  if (seed_count == 0) {
    fprintf(stderr, "No random seeds configured.\n");
    return 1;
  }

  for (window_index = 0; window_index < window_count; window_index++) {
    if (SEND_WINDOW_SIZES[window_index] <= 0 ||
        (unsigned long) SEND_WINDOW_SIZES[window_index] + RECEIVE_WINDOW_SIZE >
          (1UL << NUM_ID_BITS)) {
      fprintf(stderr, "Invalid send window size %d: it must be positive and "
              "satisfy Ws + Wr <= 2^m.\n", SEND_WINDOW_SIZES[window_index]);
      return 1;
    }
  }

  for (probability_index = 0; probability_index < probability_count;
       probability_index++) {
    if (DATA_TRANSMISSION_FAILURE_PROBABILITIES[probability_index] < 0.0 ||
        DATA_TRANSMISSION_FAILURE_PROBABILITIES[probability_index] > 1.0) {
      fprintf(stderr, "Invalid data transmission failure probability %.6f: "
              "probabilities must be between 0 and 1.\n",
              DATA_TRANSMISSION_FAILURE_PROBABILITIES[probability_index]);
      return 1;
    }
  }

  for (ack_probability_index = 0;
       ack_probability_index < ack_probability_count;
       ack_probability_index++) {
    if (ACK_TRANSMISSION_FAILURE_PROBABILITIES[ack_probability_index] < 0.0 ||
        ACK_TRANSMISSION_FAILURE_PROBABILITIES[ack_probability_index] > 1.0) {
      fprintf(stderr, "Invalid ACK transmission failure probability %.6f: "
              "probabilities must be between 0 and 1.\n",
              ACK_TRANSMISSION_FAILURE_PROBABILITIES[ack_probability_index]);
      return 1;
    }
  }

  for (timeout_interval_index = 0;
       timeout_interval_index < timeout_interval_count;
       timeout_interval_index++) {
    if (TIMEOUT_INTERVALS[timeout_interval_index] <= 0.0) {
      fprintf(stderr, "Invalid timeout interval %.6f: timeout intervals "
              "must be greater than zero.\n",
              TIMEOUT_INTERVALS[timeout_interval_index]);
      return 1;
    }
  }

  seed_elapsed_times = (double *) malloc(sizeof(*seed_elapsed_times) *
                                         seed_count);
  seed_efficiencies = (double *) malloc(sizeof(*seed_efficiencies) *
                                         seed_count);
  seed_sends_per_success = (double *) malloc(
    sizeof(*seed_sends_per_success) * seed_count);
  if (seed_elapsed_times == NULL || seed_efficiencies == NULL ||
      seed_sends_per_success == NULL) {
    perror("Unable to allocate simulation results");
    free(seed_elapsed_times);
    free(seed_efficiencies);
    free(seed_sends_per_success);
    return 1;
  }

  csv_file = fopen("efficiency.csv", "w");
  if (csv_file == NULL) {
    perror("Unable to open efficiency.csv");
    free(seed_elapsed_times);
    free(seed_efficiencies);
    free(seed_sends_per_success);
    return 1;
  }

  fprintf(csv_file, "SEND_WINDOW_SIZE,DATA_TRANSMISSION_FAILURE_PROBABILITY,"
          "ACK_TRANSMISSION_FAILURE_PROBABILITY,TIMEOUT_INTERVAL");
  for (seed_index = 0; seed_index < seed_count; seed_index++) {
    fprintf(csv_file, ",%u", RANDOM_SEEDS[seed_index]);
  }
  fprintf(csv_file, ",AVERAGE_EFFICIENCY_PERCENT");
  for (seed_index = 0; seed_index < seed_count; seed_index++) {
    fprintf(csv_file, ",%u_AVERAGE_SENDS_PER_SUCCESS",
            RANDOM_SEEDS[seed_index]);
  }
  fprintf(csv_file, ",AVERAGE_SENDS_PER_SUCCESS");
  for (seed_index = 0; seed_index < seed_count; seed_index++) {
    fprintf(csv_file, ",%u_SIMULATED_ELAPSED_TIME_SECONDS",
            RANDOM_SEEDS[seed_index]);
  }
  fprintf(csv_file, ",AVERAGE_SIMULATED_ELAPSED_TIME_SECONDS\n");
  if (ferror(csv_file)) {
    fprintf(stderr, "Unable to write the efficiency.csv header.\n");
    fclose(csv_file);
    free(seed_elapsed_times);
    free(seed_efficiencies);
    free(seed_sends_per_success);
    return 1;
  }

  for (window_index = 0; window_index < window_count; window_index++) {
    int send_window_size = SEND_WINDOW_SIZES[window_index];

    for (probability_index = 0; probability_index < probability_count;
         probability_index++) {
      double failure_probability =
        DATA_TRANSMISSION_FAILURE_PROBABILITIES[probability_index];

      for (ack_probability_index = 0;
           ack_probability_index < ack_probability_count;
           ack_probability_index++) {
      double ack_failure_probability =
        ACK_TRANSMISSION_FAILURE_PROBABILITIES[ack_probability_index];

      for (timeout_interval_index = 0;
           timeout_interval_index < timeout_interval_count;
           timeout_interval_index++) {
      double timeout_interval = TIMEOUT_INTERVALS[timeout_interval_index];

      elapsed_time_sum = 0.0;
      efficiency_sum = 0.0;
      sends_per_success_sum = 0.0;
      for (seed_index = 0; seed_index < seed_count; seed_index++) {
        random_seed = RANDOM_SEEDS[seed_index];
        random_generator_initialize(random_seed);

        simulation_run = simulation_run_new(send_window_size);
        simulation_run_set_data(simulation_run, &data);

        data.packets_processed = 0;
        data.random_seed = random_seed;
        data.total_sends = 0;
        data.last_packet_id = -1;
        data.data_transmission_failure_probability = failure_probability;
        data.ack_transmission_failure_probability = ack_failure_probability;
        data.timeout_interval = timeout_interval;

        schedule_send_event(simulation_run,
                            simulation_run_get_time(simulation_run));

        while (data.packets_processed < NUMBER_PACKETS_TO_SEND) {
          simulation_run_execute_event(simulation_run);
        }

        if (data.last_packet_id == -1) {
          printf("ERROR: last packet id not set in the simulation run with "
                 "random seed %u\n", random_seed);
        } else {
          for (i = 0; i < send_window_size; i++) {
            data.total_sends +=
              simulation_run->windowdata->send_window[i].transmission_count;
            if (simulation_run->windowdata->send_window[i].id ==
                data.last_packet_id) {
              break;
            }
          }
        }

        elapsed_time = simulation_run_get_time(simulation_run);
        seed_elapsed_times[seed_index] = elapsed_time;
        elapsed_time_sum += elapsed_time;
        data.efficiency = elapsed_time > 0.0
          ? ((double) ((data.packets_processed * DATA_TRANSMISSION_TIME) +
                       DATA_PROPAGATION_DELAY) / elapsed_time) * 100.0
          : 0.0;
        seed_efficiencies[seed_index] = data.efficiency;
        efficiency_sum += data.efficiency;
        seed_sends_per_success[seed_index] = data.packets_processed > 0
          ? (double) data.total_sends / data.packets_processed
          : 0.0;
        sends_per_success_sum += seed_sends_per_success[seed_index];

        printf("Ws=%d pd=%.3f pa=%.3f timeout=%.6f seed=%u: "
               "completed at %.3f s, packets=%ld, "
               "average sends per success=%.3f, efficiency=%.2f%%\n",
               send_window_size, failure_probability, ack_failure_probability,
               timeout_interval, random_seed,
               elapsed_time, data.packets_processed,
               seed_sends_per_success[seed_index],
               data.efficiency);

        simulation_run_free_memory(simulation_run);
      }

      average_efficiency = efficiency_sum / seed_count;
      average_elapsed_time = elapsed_time_sum / seed_count;
      average_sends_per_success = sends_per_success_sum / seed_count;
      fprintf(csv_file, "%d,%.6f,%.6f,%.6f", send_window_size,
              failure_probability, ack_failure_probability, timeout_interval);
      for (seed_index = 0; seed_index < seed_count; seed_index++) {
        fprintf(csv_file, ",%.6f", seed_efficiencies[seed_index]);
      }
      fprintf(csv_file, ",%.6f", average_efficiency);
      for (seed_index = 0; seed_index < seed_count; seed_index++) {
        fprintf(csv_file, ",%.6f", seed_sends_per_success[seed_index]);
      }
      fprintf(csv_file, ",%.6f", average_sends_per_success);
      for (seed_index = 0; seed_index < seed_count; seed_index++) {
        fprintf(csv_file, ",%.6f", seed_elapsed_times[seed_index]);
      }
      fprintf(csv_file, ",%.6f\n", average_elapsed_time);
      if (ferror(csv_file)) {
        fprintf(stderr, "Unable to write the efficiency row for "
                "SEND_WINDOW_SIZE=%d, data failure probability %.6f, and "
                "ACK failure probability %.6f, and timeout interval %.6f.\n",
                send_window_size, failure_probability, ack_failure_probability,
                timeout_interval);
        fclose(csv_file);
        free(seed_elapsed_times);
        free(seed_efficiencies);
        free(seed_sends_per_success);
        return 1;
      }

      printf("Average efficiency for SEND_WINDOW_SIZE=%d and "
             "DATA_TRANSMISSION_FAILURE_PROBABILITY=%.3f and "
             "ACK_TRANSMISSION_FAILURE_PROBABILITY=%.3f and "
             "TIMEOUT_INTERVAL=%.6f across %d seeds: "
             "%.2f%%\n\n",
             send_window_size, failure_probability, ack_failure_probability,
             timeout_interval, seed_count,
             average_efficiency);
      printf("Average sends per success for SEND_WINDOW_SIZE=%d and "
             "DATA_TRANSMISSION_FAILURE_PROBABILITY=%.3f and "
             "ACK_TRANSMISSION_FAILURE_PROBABILITY=%.3f and "
             "TIMEOUT_INTERVAL=%.6f across %d seeds: "
             "%.3f\n\n",
             send_window_size, failure_probability, ack_failure_probability,
             timeout_interval, seed_count,
             average_sends_per_success);
      }
      }
    }
  }

  free(seed_elapsed_times);
  free(seed_efficiencies);
  free(seed_sends_per_success);
  if (fclose(csv_file) != 0) {
    perror("Unable to finish writing efficiency.csv");
    return 1;
  }

  printf("Wrote per-seed and average efficiencies to efficiency.csv\n");
  return 0;
}