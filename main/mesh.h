#include "mesh_defs.h"
/*

*/

general_payload_t* get_user_queue(int *status); //for beacon
int send_to_beacon(uint8_t* data, int len);		//for sta

int send_broadcast_data_packet(uint8_t* data, int len);

void* register_rx_tx(void *tx);

void start_meshng(struct meshng_parameters_t *parameters);

