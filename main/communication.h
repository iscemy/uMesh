#include "mesh_defs.h"
#include "mesh_io.h"
#include "routing.h"

typedef struct __attribute__((packed))
{
	uint16_t size;
	uint8_t data[0];
}data_packet_bottom_t;


typedef struct __attribute__((packed))
{
	general_payload_t header;
	data_packet_bottom_t data_packet;
}data_packet_t;
