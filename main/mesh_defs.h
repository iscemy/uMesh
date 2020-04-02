 #pragma once
#include <stdint.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#define DEV_ID 0x0001 //TODO: mac'in son 2 byte'ı yada hashlenebilir, otomatik yap
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

typedef struct __attribute__((packed))
{
	uint16_t destination; 	//desired destination
	uint16_t sender;
	uint16_t target;		//
	uint8_t type;
	uint16_t data[0];
}general_payload_t;

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
