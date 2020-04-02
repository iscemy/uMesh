#include "mesh_defs.h"
#pragma once
typedef struct __attribute__((packed))
{
	int8_t rssi;
	uint16_t rx_by;
	uint16_t tx_from;
	uint8_t data[0];
}routing_packet_info_t;

typedef struct __attribute__((packed))
{
	uint16_t len;
	uint8_t data[0]; //fix size causes aligment issues 
}routing_packet_info_data_t;

typedef struct __attribute__((packed))
{
	routing_packet_info_t routing_packet_info;
	routing_packet_info_data_t routing_packet_info_data;
}routing_packet_t;

typedef struct __attribute__((packed))
{
	routing_packet_info_t routing_packet_info;
	routing_packet_info_data_t routing_packet_info_data;
}routing_packet_test;

int send_routing_packets();
int send_routing_start_pkts();
int start_routing_seq(uint8_t device_type);
uint16_t get_next_node_addr_to_beacon();