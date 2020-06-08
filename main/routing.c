#include "routing.h"
#include "mesh_io.h"
#include <string.h>
#include "esp_log.h"
#include "hash_map.h"
#define LISTEN_NUM_OF_PACKETS_ROUTING 50
#define REPEAT_NUM_OF_ROUTING_START_SEQ 1
#define MAX_NUM_OF_DISCARDED_PACKETS_IN_START_SEQ 200
#define MAX_NUM_ROUTING_PACKET 50
#define BEACON_START_PKT_CNT 10
#define TAG "routing.c"

//code is redundant and ugly fix it

routing_packet_info_t *routing_packet_array[LISTEN_NUM_OF_PACKETS_ROUTING];
int routing_packet_array_index = 0;


int send_routing_packets(){
	int ret = 0, len, size;
	char buf[256];
	routing_packet_t* routing_packet;
	general_payload_t * general_payload = (general_payload_t *) buf;

	general_payload->destination = BROADCAST_ADDR;
	general_payload->target = BROADCAST_ADDR;
	general_payload->sender = DEV_ID;
	general_payload->type = routing_broadcast;

	for (int index = 0; index < routing_packet_array_index; index++){
		routing_packet = routing_packet_array[index];
		len =  routing_packet->routing_packet_info_data.len*sizeof(uint16_t);
		size = sizeof(general_payload_t) + sizeof(routing_packet_t) + len;

		ESP_LOGI(TAG,"send_routing_packets %d index %d, len %d", routing_packet_array_index,index,size);
		if(size > 255){
			ESP_LOGI(TAG,"are u still alive");
			ret = -1;
			break;
		}
		memcpy(general_payload->data, routing_packet_array[index], sizeof(routing_packet_t) + len);
		
		ret = tx_data_blocking((unsigned char *)general_payload, size);
		if(ret != 0){
			break;
		}
	}
	ESP_LOGI(TAG,"send_routing_packets ret %d", ret);
	return ret;
}


//TODO: make this funtion more readable
routing_packet_info_t* init_routing_packets_table(uint16_t tx_from, uint16_t rx_by, int8_t rssi, char *data){
	routing_packet_t* new_routing_packet = malloc(sizeof(routing_packet_t) + 2 + ((routing_packet_info_data_t*)data)->len*2);
	int lindex = (((routing_packet_info_data_t*)data)->len)*2;
	ESP_LOGI(TAG,"@init_routing_packets_table :%d",sizeof(routing_packet_test) + 2 + (((routing_packet_info_data_t*)data)->len)*2);

	if(new_routing_packet != NULL){
		new_routing_packet->routing_packet_info.tx_from = tx_from;
		new_routing_packet->routing_packet_info.rx_by = rx_by;	
		new_routing_packet->routing_packet_info.rssi = rssi;	
		((routing_packet_info_data_t*)data)->len++;
		memcpy(&new_routing_packet->routing_packet_info_data, data, lindex + sizeof(routing_packet_info_data_t));
		(new_routing_packet->routing_packet_info_data.data)[lindex] = rx_by&0xff;
		(new_routing_packet->routing_packet_info_data.data)[lindex + 1] = rx_by>>8;
		return new_routing_packet;
	}else{
		return NULL;
	}

	return NULL;
}

int gather_routing_packets(){
	//tüm cihazlar routing durumuna geldiği zaman
	int get_status,failsafe = 0,recved_packets = 0, rloop = 0;
	data_unit* recvd_data;
	general_payload_t* recved_payload;
	routing_packet_info_t* rpi_s;
	routing_packet_t *routing_packet;
	uint16_t *routing_rec;
	while (recved_packets < LISTEN_NUM_OF_PACKETS_ROUTING){
		recvd_data = get_data(&get_status); // (a)
		recved_payload = (general_payload_t*)recvd_data->payload;
		vTaskDelay(30 / portTICK_PERIOD_MS); //it has to be more abstarct and not dependent to target chips sdk-11	
		//ESP_LOGI(TAG,"routing.c routing_broadcast @ gather_routing_packets %d", recved_payload->type);	
		if(get_status == 0){
			if((recved_payload->destination == 0xFFFF)&&(recved_payload->type == routing_broadcast)){

				rloop = 0;
				routing_packet = recved_payload->data;
				routing_rec = (routing_packet->routing_packet_info_data.data);
				for(int index = 0; index < routing_packet->routing_packet_info_data.len; index = index + 2){
					if(routing_packet->routing_packet_info_data.data[index] + (routing_packet->routing_packet_info_data.data[index + 1]<<8) == DEV_ID){	//for preventing loops in rtable
						rloop = 1;
						break;
					}
				}
				ESP_LOGI(TAG,"routing.c routing_broadcast @ gather_routing_packets failsafe %d  recved_packets %d rloop %d", failsafe, recved_packets, rloop);
				if(rloop == 0){
					ESP_LOGI("TAG","received routing packet first node %hx, len %d", routing_packet->routing_packet_info_data.data[0] + (routing_packet->routing_packet_info_data.data[1]<<8), routing_packet->routing_packet_info_data.len);
					rpi_s = init_routing_packets_table(recved_payload->sender, DEV_ID, recvd_data->rssi, &(routing_packet->routing_packet_info_data)); //allocating/copying routing info for table
					if(rpi_s != NULL){
						routing_packet_array[recved_packets] = rpi_s;
						recved_packets++;
					}else{
						//NOT ENOUGH MEM FOR ALLOCATION
					}
				}
			}
		}
		if(failsafe > MAX_NUM_ROUTING_PACKET){
			routing_packet_array_index = recved_packets;
			if(recved_packets > 0){
				return 0; //maybe problematic with (a)
			}else{
				return -1;
			}
		}
		failsafe++;
		routing_packet_array_index = recved_packets;
	}

	return 0;
}



int send_routing_start_pkts(){
	int ret;
	general_payload_t* routing_start_frame = malloc(sizeof(general_payload_t));
	if(routing_start_frame != NULL){
		routing_start_frame->destination = BROADCAST_ADDR;
		routing_start_frame->sender = DEV_ID;
		routing_start_frame->type = routing_seq_start;
		ret = tx_data_blocking((uint8_t*)routing_start_frame, sizeof(general_payload_t));
		free(routing_start_frame);	//it may cause race conditions
									//send function will be better blocking until copies and not depended to passed address
									//altough function named as blocking but its not

	}else{
		ret = -1;
	}
	return ret;
}

int repeatw_routing_start_packets(uint8_t device_type){
	//this function may cause data losses
	//cagirildigi zaman queue de olan tüm paketler kaybolacak.
	int get_status = -1, start_seq_cnt = 0, ret = -1;
	int8_t rssi = -23;
	general_payload_t* recved_payload;
	data_unit* recved_data = NULL;
	flush_map();
	if(device_type == beacon){
		for (int i = 0; i < BEACON_START_PKT_CNT; i++){
			while (send_routing_start_pkts() != 0){
				vTaskDelay(10/portTICK_PERIOD_MS);
			}
		} 
		
		return 0;
	}
	for(int s = 0; s < MAX_NUM_OF_DISCARDED_PACKETS_IN_START_SEQ; s++){
		recved_data = get_data(&get_status);
		vTaskDelay(10 / portTICK_PERIOD_MS); //it has to be more abstarct and not dependent to target chips sdk
		if (get_status == 0){
			recved_payload = (general_payload_t*)recved_data->payload;
			ESP_LOGI(TAG,"@repeatw_routing_start_packets destination:%d %d %d rssi:%d",recved_payload->destination,((data_unit*) recved_data)->payload[1],
				((data_unit*) recved_data)->payload[2], recved_data->rssi);
			rssi = recved_data->rssi;
			if((recved_payload->destination == 0xFFFF)&&(recved_payload->type == routing_seq_start)){

				if(find_entry(recved_payload->sender) == NULL){
					insert_entry(recved_payload->sender,1,(void*)&rssi);
				}
				ESP_LOGI(TAG,"routing_seq_start packet received Num:%d", start_seq_cnt);
				send_routing_start_pkts();
				if (start_seq_cnt < REPEAT_NUM_OF_ROUTING_START_SEQ){
					start_seq_cnt++;
				}else{
					ret = 0;
				}
			}
			
		}
	}
	return ret;
}

void *rssi_table;
int rssi_table_size = 0;


int start_routing_seq(uint8_t device_type){
	ESP_LOGI(TAG,"start_routing_seq");
	struct entry *rssi_element;
	while (repeatw_routing_start_packets(device_type) != 0){

	}

	if(device_type == sta){
		ESP_LOGI(TAG,"received routing_start_pkts");

		while (gather_routing_packets() != 0){

		}

		ESP_LOGI(TAG,"received routing_packets");
		#ifdef DEBUG_BUILD
			routing_packet_t *routing_packet;
			int len;
			for(int i = 0; i < routing_packet_array_index; i++){
				routing_packet = routing_packet_array[i];
				len = routing_packet->routing_packet_info_data.len*2;
				printf("len: %d \n", routing_packet->routing_packet_info_data.len);
				while (len > 0){
					printf("%x", routing_packet->routing_packet_info_data.data[len - 2]);
					printf("%x", routing_packet->routing_packet_info_data.data[len - 1]);
					printf(" ");
					len = len - 2;
				}
				printf("\n");
			}
			
		#endif
		vTaskDelay(30 / portTICK_PERIOD_MS);
		if (send_routing_packets() == 0){
			ESP_LOGI(TAG,"station sended first routing_packet");
		}else{

		}
	vTaskDelay(200/portTICK_PERIOD_MS);


	}else if(device_type == beacon){
		uint32_t* beacon_rtable_data = malloc(4);
		void *ret;
		routing_packet_info_data_t *rtable1 = (routing_packet_info_data_t*)beacon_rtable_data;
		rtable1->len = 0;
		ret = init_routing_packets_table(0, 0xbeef, 0, rtable1); //initilization a routing table for first rtable tx

		if(ret == NULL){

			return -1;
		}else{
			routing_packet_array[0] = ret;
			routing_packet_array_index = 1;
			ESP_LOGI(TAG,"added first rtable element to table,%d", ((routing_packet_t*)ret)->routing_packet_info_data.data[1]);
		}

		vTaskDelay(2000/portTICK_PERIOD_MS);

		for(int ind = 0; ind < 8; ind++){
			ESP_LOGI(TAG,"SENDING ROUTING PACKET");
			while (send_routing_packets() != 0);
			vTaskDelay(200/portTICK_PERIOD_MS);
			ESP_LOGI(TAG,"SENDED ROUTING PACKET\n");
		}

	}
		
		int rssi_element_size = get_num_of_enrties(), t_index = 0;
		rssi_table_element_t *rssi_table_element;

		ESP_LOGI(TAG,"total entry size: %d", rssi_element_size);
		rssi_table = malloc(rssi_element_size*sizeof(rssi_table_element_t));
		rssi_element = get_an_entry();

		while(rssi_element != NULL){
			rssi_table_element = rssi_table + sizeof(rssi_table_element)*t_index; 

			rssi_table_element->addr = rssi_element->key;
			rssi_table_element->rssi = (int8_t)*rssi_element->data;
			ESP_LOGI(TAG,"+%d to %p ADDR: %hx RSSI:%d", sizeof(rssi_table_element_t), rssi_table_element, rssi_table_element->addr, rssi_table_element->rssi);

			rssi_element = get_an_entry();
			t_index++;
		}
		rssi_table_size = t_index*sizeof(rssi_table_element_t);
		ESP_LOGI(TAG,"RSSI_TABLE: %d %p", rssi_table_size, rssi_table);
		ESP_LOGI(TAG,"TRSSITABLEEEs %p %hx %d %hx %d", rssi_table, ((rssi_table_element_t *)rssi_table)->addr,((rssi_table_element_t *)rssi_table)->rssi,
				((rssi_table_element_t *)rssi_table + 4)->addr,((rssi_table_element_t *)rssi_table + 4)->rssi);

	return 0;
}

void* get_rssi_table(int *size){
	*size = rssi_table_size;
	return rssi_table;
}

uint16_t get_next_node_addr(uint16_t destination){
	//for p2p comm, many aspects not yet implemented

	return 0;
}

uint16_t get_next_node_addr_to_beacon(){
	//an algortihm based upon rssi of total routing path can be implemented.
	/* dirty dram flush solve*/
	if(meshng_state == no_routing){
		ESP_LOGI(TAG,"get_next_node_addr no_routing %hx",*((uint16_t*)0x60001200));
		return *((uint16_t*)0x60001200);
	}else{
		int len;
		uint16_t *ret;
		if(routing_packet_array_index > 0){
			routing_packet_t *routing_packet = routing_packet_array[routing_packet_array_index - 1];
			len = routing_packet->routing_packet_info_data.len;
			ret = routing_packet->routing_packet_info_data.data;
			if(len > 1){
				*((uint16_t*)0x60001200) = (routing_packet->routing_packet_info_data.data[(2*(len) - 3)]<<8) + (routing_packet->routing_packet_info_data.data[(2*(len) - 4)]);
				*((uint16_t*)0x60001204) = len;
				return (routing_packet->routing_packet_info_data.data[(2*(len) - 3)]<<8) + (routing_packet->routing_packet_info_data.data[(2*(len) - 4)]);
			}
			//ESP_LOGI(TAG,"@get_next_node_addr_to_beacon addr:%hx, num:%d",ret[0],routing_packet_array_index);
		}
	}
	return 0;
}


