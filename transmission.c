#include <stdio.h>
#include <math.h>
#include "trace.h"
#include "transmission.h"

int min(int a, int b)
{
    return (a < b) ? a : b;
}

long int
schedule_reservation_send_event(Simulation_Run_Ptr simulation_run, Time event_time, int station_id)
{
    Event event;
    event.description = "Reservation Send Event";
    event.function = reservation_send_event;
    event.attachment.id = -1;
    event.attachment.station_id = station_id;

    // change to slotted R-ALOHA - THIS JUST MOVES TIME TO BEGINNING OF NEXT SLOT
    Time slot_time = ceil((event_time - GUARD_TIME) / RESERVATION_SLOT_DURATION) * RESERVATION_SLOT_DURATION + GUARD_TIME;
  
    if (slot_time < event_time)
        slot_time += RESERVATION_SLOT_DURATION;

    // If the next reservation slot is in the next packet duration, move to the next packet duration
    if(fmod(slot_time, TOTAL_PACKET_DURATION) > RESERVATION_DURATION) {
        slot_time += TOTAL_PACKET_DURATION - (fmod(slot_time, TOTAL_PACKET_DURATION)) + GUARD_TIME;
    }

    return simulation_run_schedule_event(simulation_run, event, slot_time);
}

void reservation_send_event(Simulation_Run_Ptr simulation_run, Packet packet)
{
    int station_id = packet.station_id;
    Simulation_Run_Data_Ptr data = (Simulation_Run_Data_Ptr) simulation_run_data(simulation_run);
    Channel_Ptr channel = data->channel;

    // Get index of next unsent packet in the send window
    int send_window_index = -1;
    for (int i = 0; i < data->stations[station_id].send_window_size; i++) {
        if (!data->stations[station_id].send_window[i].reservation_sent) {
            send_window_index = i;
            break;
        }
    }

    if (send_window_index == -1) {
        // No unsent packets in the send window, nothing to do
        return;
    }

    // Create packet with next id in send window
    Packet new_packet;
    new_packet.id = data->stations[station_id].send_window[send_window_index].id;
    new_packet.station_id = station_id;

    new_packet.status = TRANSMITTING;

    if(get_channel_state(channel) != IDLE) {
        /* The channel is now colliding. */
        set_channel_state(channel, COLLISION);
    } 
    else {
        /* The channel is successful, for now. */
        set_channel_state(channel, SUCCESS);
    }

    // Increment the transmission count for the packet
    increment_transmitting_stn_count(channel);
    data->stations[station_id].send_window[send_window_index].reservation_sent = true;

    schedule_reservation_receive_event(simulation_run, simulation_run_get_time(simulation_run) + RESERVATION_SLOT_NO_GUARD_DURATION, new_packet);
}

long int
schedule_reservation_receive_event(Simulation_Run_Ptr simulation_run, Time event_time,
               Packet packet)
{
    Event event;
    event.description = "Reservation Receive Event";
    event.function = reservation_receive_event;
    event.attachment = packet;

    return simulation_run_schedule_event(simulation_run, event, event_time);
}

void reservation_receive_event(Simulation_Run_Ptr simulation_run, Packet packet)
{
    Simulation_Run_Data_Ptr data = (Simulation_Run_Data_Ptr) simulation_run_data(simulation_run);
    Channel_Ptr channel = data->channel;
    Time backoff_duration;
    Window_Data_Ptr window_data = (Window_Data_Ptr) simulation_run->windowdata;
    
    /* This station has stopped transmitting. */
    decrement_transmitting_stn_count(channel);

    /* Check if the packet was successful. */
    if(get_channel_state(channel) == SUCCESS) {

        set_channel_state(channel, IDLE);

        data->stations[packet.station_id].sequential_collisions = 0;

        packet.status = WAITING;

        TRACE(printf("Packet %ld: Reservation successful at time %.3f\n", packet.id, simulation_run_get_time(simulation_run));)

        // Schedule the send event for the packet
        schedule_send_event(simulation_run, simulation_run_get_time(simulation_run) + RESERVATION_DURATION + GUARD_TIME, packet.station_id);
    } 
    else {

        /* The transmission was unsuccessful. Clean up the channel state,
        backoff, and try again. */
        packet.status = WAITING;

        data->stations[packet.station_id].sequential_collisions++;

        // Reset reservation sent
        for (int i = 0; i < RECEIVE_WINDOW_SIZE; i++) {
            if(window_data->receive_window[i].id == packet.id) {
                window_data->receive_window[i].reservation_sent = false;
                break;
            }
        }

        TRACE(printf("Packet %ld: Collision at time  %.3f\n", packet.id, simulation_run_get_time(simulation_run)););

        /* If the collision is over, free up the channel. */
        if(get_transmitting_stn_count(channel) == 0) {
            set_channel_state(channel, IDLE);
        }

        // For discrte slots - backoff duration will be multiple of slot duration
        double slot_index = floor(uniform_generator() * pow(2.0, min(data->stations[packet.station_id].sequential_collisions, 3)));
        backoff_duration = slot_index * RESERVATION_SLOT_DURATION;
    }

    // Schedule the next reservation attempt for immediately or after the backoff
    schedule_reservation_send_event(simulation_run, simulation_run_get_time(simulation_run) + backoff_duration, packet.station_id);
}


// Everything below deals with DATA rather than reservation

long int
schedule_send_event(Simulation_Run_Ptr simulation_run, Time event_time, int station_id)
{
    Event event;
    event.description = "Send Event";
    event.function = send_event;
    event.attachment.id = -1;
    event.attachment.station_id = station_id;

    return simulation_run_schedule_event(simulation_run, event, event_time);
}

void send_event(Simulation_Run_Ptr simulation_run, Packet packet)
{
    int station_id = packet.station_id;
    Simulation_Run_Data_Ptr data = (Simulation_Run_Data_Ptr) simulation_run_data(simulation_run);

    // Get index of next unsent packet in the send window
    int send_window_index = -1;
    for (int i = 0; i < data->stations[station_id].send_window_size; i++) {
        if (!data->stations[station_id].send_window[i].processed) {
            send_window_index = i;
            break;
        }
    }

    // Create packet with next id in send window
    Packet new_packet;
    new_packet.id = data->stations[station_id].send_window[send_window_index].id;
    new_packet.station_id = station_id;

    // Increment the transmission count for the packet
    data->stations[station_id].send_window[send_window_index].transmission_count++;

    // Send packet and mark as processed
    schedule_receive_event(simulation_run, simulation_run_get_time(simulation_run) + DATA_PROPAGATION_DELAY + DATA_TRANSMISSION_TIME, new_packet);
    schedule_timeout_event(simulation_run, simulation_run_get_time(simulation_run) + data->timeout_interval + DATA_TRANSMISSION_TIME, new_packet);
    data->stations[station_id].send_window[send_window_index].processed = true;

    TRACE(printf("Packet %ld: sent at time %.3f\n", new_packet.id, simulation_run_get_time(simulation_run));)

    bool open_send_slot = false;
    for (int i = 0; i < data->stations[station_id].send_window_size; i++) {
        if (!data->stations[station_id].send_window[i].processed) {
            open_send_slot = true;
            break;
        }
    }

    if(open_send_slot) {
        TRACE(printf("Scheduling next send event at time %.3f\n", simulation_run_get_time(simulation_run) + DATA_TRANSMISSION_TIME);)
        schedule_send_event(simulation_run, simulation_run_get_time(simulation_run) + DATA_TRANSMISSION_TIME, station_id);
    }
}

long int
schedule_receive_event(Simulation_Run_Ptr simulation_run, Time event_time,
               Packet packet)
{
    Event event;
    event.description = "Receive Event";
    event.function = receive_event;
    event.attachment = packet;

    return simulation_run_schedule_event(simulation_run, event, event_time);
}

void receive_event(Simulation_Run_Ptr simulation_run, Packet packet)
{
    Simulation_Run_Data_Ptr data = (Simulation_Run_Data_Ptr) simulation_run_data(simulation_run);
    if(uniform_generator() < data->data_transmission_failure_probability) {
        TRACE(printf("Packet %ld: Packet lost during transmission at time %.3f\n", packet.id, simulation_run_get_time(simulation_run));)
        return;
    }

    Window_Data_Ptr window_data = (Window_Data_Ptr) simulation_run->windowdata;

    bool packet_in_receive_window = false;
    // Mark the packet as processed in the receive window
    for (int i = 0; i < RECEIVE_WINDOW_SIZE; i++) {
        if(window_data->receive_window[i].id == packet.id) {
            if(!window_data->receive_window[i].processed) {
                window_data->receive_window[i].processed = true;
                window_data->receive_window[i].acked = true;
                packet_in_receive_window = true;
            }
            break;
        }
    }

    int max_id = (1 << NUM_ID_BITS) - 1;

    // Schedule ack event if the packet is in the receive window or if it is within the send window size of the first packet in the receive window
    bool schedule_ack = packet_in_receive_window || ((((window_data->receive_window[0].id - packet.id) + max_id) % max_id) <= data->stations[packet.station_id].send_window_size);

    // Check if the first packet in the receive window has been acknowledged, and if so, shift the window to the left
    while(window_data->receive_window[0].acked) {

        // Shift the receive window to the right
        for (int i = 0; i < RECEIVE_WINDOW_SIZE - 1; i++) {
            window_data->receive_window[i] = window_data->receive_window[i + 1];
        }
        // Reset the last entry in the receive window
        if(RECEIVE_WINDOW_SIZE > 1) {
            window_data->receive_window[RECEIVE_WINDOW_SIZE - 1].id = window_data->receive_window[RECEIVE_WINDOW_SIZE - 2].id + 1;
        } else {
            window_data->receive_window[RECEIVE_WINDOW_SIZE - 1].id++;
        }
        if(window_data->receive_window[RECEIVE_WINDOW_SIZE - 1].id > (1 << NUM_ID_BITS) - 1) {
            window_data->receive_window[RECEIVE_WINDOW_SIZE - 1].id = 0;
        }
        window_data->receive_window[RECEIVE_WINDOW_SIZE - 1].processed = false;
        window_data->receive_window[RECEIVE_WINDOW_SIZE - 1].acked = false;
        window_data->receive_window[RECEIVE_WINDOW_SIZE - 1].reservation_sent = false;
    }

    // Increment the number of packets processed
    if(packet_in_receive_window) {
        data->packets_processed++;

        data->stations[packet.station_id].last_packet_id = packet.id;
    }

    // Schedule an ack event for the received packetS
    if(schedule_ack) {
        schedule_ack_event(simulation_run, simulation_run_get_time(simulation_run) + ACK_TRANSMISSION_TIME + DATA_PROPAGATION_DELAY, packet);
        if(packet_in_receive_window) {
            TRACE(printf("Packet %d: received at time %.3f, total packets processed: %ld\n", packet.id, simulation_run_get_time(simulation_run), data->packets_processed);)
        } else {
            TRACE(printf("Packet %d: received at time %.3f, but it is a duplicate\n", packet.id, simulation_run_get_time(simulation_run));)
        }
       // TRACE(printf("Packet %d: received at time %.3f, total packets processed: %ld\n", packet.id, simulation_run_get_time(simulation_run), data->packets_processed);)
    }
    else {
        TRACE(printf("Packet %d: received at time %.3f, but it is not in the receive window or is a duplicate\n", packet.id, simulation_run_get_time(simulation_run));)
        TRACE(printf("Receive window: ");)
        for (int i = 0; i < RECEIVE_WINDOW_SIZE; i++) {
            TRACE(printf("%d ", window_data->receive_window[i].id);)
        }
        TRACE(printf("\n");)
    }
}

long int
schedule_ack_event(Simulation_Run_Ptr simulation_run, Time event_time,
               Packet packet)
{
    Event event;
    event.description = "Ack Event";
    event.function = ack_event;
    event.attachment = packet;

    return simulation_run_schedule_event(simulation_run, event, event_time);
}

void ack_event(Simulation_Run_Ptr simulation_run, Packet packet)
{
    Simulation_Run_Data_Ptr data = (Simulation_Run_Data_Ptr) simulation_run_data(simulation_run);
    if(uniform_generator() < data->ack_transmission_failure_probability) {
        TRACE(printf("Packet %d: Ack lost during transmission at time %.3f\n", packet.id, simulation_run_get_time(simulation_run));)
        return;
    }

    // Check if there is an open slot in the send window
    bool open_send_slot_beginning = false;
    int packet_index = -1;
    for (int i = 0; i < data->stations[packet.station_id].send_window_size; i++) {
        if (!data->stations[packet.station_id].send_window[i].processed) {
            open_send_slot_beginning = true;
        }
        if(data->stations[packet.station_id].send_window[i].id == packet.id) {
            packet_index = i;
        }
    }

    if(packet_index != -1) {
        if(packet_index == 0 || RECEIVE_WINDOW_SIZE > 1) {
            data->stations[packet.station_id].send_window[packet_index].acked = true;
            TRACE(printf("Packet %d: Ack received at time %.3f\n", packet.id, simulation_run_get_time(simulation_run));)
        } else {
            TRACE(printf("Packet %d: Ack received at time %.3f, but it was out of order in SW or GBN mode\n", packet.id, simulation_run_get_time(simulation_run));)
        }
    } else {
        TRACE(printf("Packet %d: ERROR: Ack received for unknown packet at time %.3f\n", packet.id, simulation_run_get_time(simulation_run));)
    }

    while(data->stations[packet.station_id].send_window[0].acked) {

        data->total_sends += data->stations[packet.station_id].send_window[0].transmission_count;

        // Shift the send window to the right
        for (int i = 0; i < data->stations[packet.station_id].send_window_size - 1; i++) {
            data->stations[packet.station_id].send_window[i] = data->stations[packet.station_id].send_window[i + 1];
        }
        // Reset the last entry in the send window
        if(data->stations[packet.station_id].send_window_size > 1) {
            data->stations[packet.station_id].send_window[data->stations[packet.station_id].send_window_size - 1].id = data->stations[packet.station_id].send_window[data->stations[packet.station_id].send_window_size - 2].id + 1;
        } else {
            data->stations[packet.station_id].send_window[data->stations[packet.station_id].send_window_size - 1].id++;
        }
        if(data->stations[packet.station_id].send_window[data->stations[packet.station_id].send_window_size - 1].id > (1 << NUM_ID_BITS) - 1) {
            data->stations[packet.station_id].send_window[data->stations[packet.station_id].send_window_size - 1].id = 0;
        }
        data->stations[packet.station_id].send_window[data->stations[packet.station_id].send_window_size - 1].processed = false;
        data->stations[packet.station_id].send_window[data->stations[packet.station_id].send_window_size - 1].acked = false;
        data->stations[packet.station_id].send_window[data->stations[packet.station_id].send_window_size - 1].reservation_sent = false;
        data->stations[packet.station_id].send_window[data->stations[packet.station_id].send_window_size - 1].transmission_count = 0;
    }

    // Check if ack frees up an empty slot in the send window
    bool open_send_slot_end = false;
    for (int i = 0; i < data->stations[packet.station_id].send_window_size; i++) {
        if (!data->stations[packet.station_id].send_window[i].processed) {
            open_send_slot_end = true;
            break;
        }
    }

    // Schedule send event if there is an open slot in the send window and no other send events are scheduled
    if(open_send_slot_end && !open_send_slot_beginning) {
        TRACE(printf("Scheduling next send event at current time %.3f\n", simulation_run_get_time(simulation_run));)
        schedule_send_event(simulation_run, simulation_run_get_time(simulation_run), packet.station_id);
    }
}


long int
schedule_timeout_event(Simulation_Run_Ptr simulation_run, Time event_time,
               Packet packet)
{
    Event event;
    event.description = "Timeout Event";
    event.function = timeout_event;
    event.attachment = packet;

    return simulation_run_schedule_event(simulation_run, event, event_time);
}

void timeout_event(Simulation_Run_Ptr simulation_run, Packet packet)
{
    Simulation_Run_Data_Ptr data = (Simulation_Run_Data_Ptr) simulation_run_data(simulation_run);

    int packet_index = -1;
    // Get index of the packet with this id
    for (int i = 0; i < data->stations[packet.station_id].send_window_size; i++) {
        if(data->stations[packet.station_id].send_window[i].id == packet.id) {
            packet_index = i;
            break;
        }
    }

    if(packet_index == -1) {
        TRACE(printf("Packet %d:Timeout event for unknown packet at time %.3f\n", packet.id, simulation_run_get_time(simulation_run));)
        return;
    }

    if(data->stations[packet.station_id].send_window[packet_index].acked) {
        TRACE(printf("Packet %d: Timeout event for packet at time %.3f, but packet already acknowledged\n", packet.id, simulation_run_get_time(simulation_run));)
        return;
    }

    // Check if there is an open slot in the send window
    bool open_send_slot_beginning = false;
    for (int i = 0; i < data->stations[packet.station_id].send_window_size; i++) {
        if (!data->stations[packet.station_id].send_window[i].processed) {
            open_send_slot_beginning = true;
            break;
        }
    }

    // Mark the packet as unprocessed and unreserved so it can be resent
    data->stations[packet.station_id].send_window[packet_index].processed = false;
    data->stations[packet.station_id].send_window[packet_index].reservation_sent = false;
    TRACE(printf("Packet %d: Timeout event for packet at time %.3f, resending\n", packet.id, simulation_run_get_time(simulation_run));)

    // Check if ack frees up an empty slot in the send window
    bool open_send_slot_end = false;
    for (int i = 0; i < data->stations[packet.station_id].send_window_size; i++) {
        if (!data->stations[packet.station_id].send_window[i].processed) {
            open_send_slot_end = true;
            break;
        }
    }

    // Schedule send event if there is an open slot in the send window and no other send events are scheduled
    if(open_send_slot_end && !open_send_slot_beginning) {
        TRACE(printf("Scheduling next send event at current time %.3f\n", simulation_run_get_time(simulation_run));)
        schedule_send_event(simulation_run, simulation_run_get_time(simulation_run), packet.station_id);
    }
}