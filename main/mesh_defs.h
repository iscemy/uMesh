 #pragma once
#include <stdint.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
//#define DEV_ID 0x2222//TODO: mac'in son 2 byte'ı yada hashlenebilir, otomatik yap
uint16_t DEV_ID;
int meshng_state;
#define BEACON_ADDR 0xbeef
#define BROADCAST_ADDR 0xFFFF

/*
TODO:Proper error check/debug 
*/
#define DEBUG_BUILD
/*
enum mesh_status {
	MESH_ERROR,
	MESH_OK
};
*/

typedef struct 
{
	uint16_t destination; 	//desired destination
	uint16_t sender;
	uint16_t target;		//
	uint8_t type;
	uint8_t seq_id;
	uint16_t data[0];
}general_payload_t;

struct meshng_parameters_t{
	uint16_t addr;
	int type;
	char with_rtseq;
};

enum packet_types{
	data = 0,
	routing_seq_start,
	routing_broadcast
	//
};

enum node_types{
	beacon = 0,
	sta
};

enum state_machine_states{
	without_init = 0,
	routing,
	routing_end,
	no_routing
};

typedef struct 
{
	uint16_t size;
	uint8_t data[0];
}data_packet_bottom_t;


typedef struct 
{
	general_payload_t header;
	data_packet_bottom_t data_packet;
}data_packet_t;


