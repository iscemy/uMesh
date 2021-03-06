#include <stdint.h>
#include <stdlib.h>
#pragma once
#include "mesh_defs.h"

//payload is sended data from transmitter, other fields are useful datas collected from lower layers

typedef struct {
    int8_t rssi;
    uint32_t sys_time;
    uint16_t len;
    uint8_t payload[0];
}data_unit;

typedef struct {
	uint16_t len;
	uint16_t addrs[0];
}rtablec_packet;

int post_data(uint8_t *buf,uint16_t len, int8_t rssi, uint32_t sys_time);

data_unit* get_data(int* status);

int tx_data_blocking(general_payload_t *buf_payload, unsigned short len);