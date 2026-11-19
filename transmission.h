#ifndef _PACKET_TRANSMISSION_H_
#define _PACKET_TRANSMISSION_H_

#include "main.h"

int min (int a, int b);

long int schedule_send_event(Simulation_Run_Ptr, Time);
void send_event(Simulation_Run_Ptr, Packet);

long int schedule_receive_event(Simulation_Run_Ptr, Time, Packet);
void receive_event(Simulation_Run_Ptr, Packet);

long int schedule_ack_event(Simulation_Run_Ptr, Time, Packet);
void ack_event(Simulation_Run_Ptr, Packet);

long int schedule_timeout_event(Simulation_Run_Ptr, Time, Packet);
void timeout_event(Simulation_Run_Ptr, Packet);

#endif /* packet_transmission.h */