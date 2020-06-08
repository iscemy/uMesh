#include "mesh_defs.h"
void* register_rx_tx(void *tx);
int tx_data_blocking(general_payload_t *buf_payload, unsigned short len);
int tx_data_blocking_woseq(general_payload_t *buf_payload, unsigned short len);

