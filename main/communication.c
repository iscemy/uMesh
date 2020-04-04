#include "communication.h"

#include "routing.h"
#include "esp_log.h"
#include "mesh_defs.h"
#include "mesh_io.h"
#include <string.h>


#define TAG "communication.c"
#define MAX_QUEUE_SIZE 10

int head = 0;
int tail = 0;

general_payload_t* mesh_ng_queue[MAX_QUEUE_SIZE];



int put_meshng_work_queue(general_payload_t* packet){
    int ret = 0;
    if(tail >= MAX_QUEUE_SIZE){
        ret = -1;//queue full
    }else{
        if(packet != NULL){
            mesh_ng_queue[tail] = packet;
            tail++;
        }else{
            ret = -3;//not enough mem
        }
    }
    #ifdef DEBUG_BUILD
        ESP_LOGI(TAG,"put_meshng_work_queue status %d head:%d, tail:%d",ret, head,tail);
    #endif
    return ret;
}

general_payload_t* get_meshng_work_queue(int *status){
    int ret = 0;
    if((head < tail)||((tail == MAX_QUEUE_SIZE - 1)&&(head == MAX_QUEUE_SIZE - 1))){
        if((tail == MAX_QUEUE_SIZE - 1)&&(head == MAX_QUEUE_SIZE - 2)){
            head = 0;
            tail = 0;
        }else{
            head++;
        }
        ESP_LOGI(TAG,"get_meshng_work_queue status %d head:%d, tail:%d", ret, head, tail);

    }else{
        //ESP_LOGI(TAG,"queue empty");
        ret = -1;//queue empty
    }
    #ifdef DEBUG_BUILD
        //ESP_LOGI(TAG,"swap %d", ((data_unit*)swap)->payload[7]);
    #endif
    *status = ret;
   
    return mesh_ng_queue[head];
}


int put_dpacket_wqueue(data_packet_t* packet){
	data_packet_t* local_dpacket = malloc(packet->data_packet.size + sizeof(general_payload_t));
	if(local_dpacket != NULL){
		memcpy(local_dpacket, packet, packet->data_packet.size  + sizeof(general_payload_t));
		put_meshng_work_queue(local_dpacket);
		return 0;
	}else{
		return -1;
		//not enough mem
	}

}

int packet_classifier(general_payload_t* packet){
	uint16_t new_target_node = 0;
	if(packet->destination == BEACON_ADDR){
		if(packet->target == DEV_ID){
			if(packet->type == data){		//only route data packets 
				new_target_node = get_next_node_addr_to_beacon();
				if(new_target_node != 0){	//is routing table exists ?
					packet->target = new_target_node;
					packet->sender = DEV_ID;
					if(put_dpacket_wqueue(packet) != 0){
						//error handling ng work queue
					}
				}else{
					//routing table does not exists
				}

			}
		}
	}else if(packet->destination == DEV_ID){
		//p2p comm
	}
	return 0;
}


int init_put_data_for_work_queue(uint8_t *data, int len, int target, int destination) {
	general_payload_t* packet = malloc(len + sizeof(general_payload_t));
	if(packet != NULL){
		packet->destination = destination;
		packet->target = target;
		packet->type = 0;
		packet->sender = DEV_ID;
		memcpy(packet->data, data, len);
		put_meshng_work_queue(packet);
		return 0;
	} 
	return -1;
}


int mesh_engine(){
	/*
		it has to checks state before proceeding
		two diffrent packet sources:
			1- from other nodes
			2- itself
		this two sources must stored in memory in organized manner (like a queue)
	*/
	int status = 0;
	data_unit *packet_w_info;
	general_payload_t *received_packet;
	data_packet_t* packet_from_work_queue; //only data packets routed

	if(status == 0){
		packet_w_info = get_data(&status);
		received_packet = (general_payload_t*)packet_w_info->payload;
		packet_classifier(received_packet);//received packet classified and if its a data packet, addressed to beacon 
											//added to work queue
	}else{
		//cannot receive any data from mesh_io queue
	}
	
	packet_from_work_queue = get_meshng_work_queue(&status);
	if(status == 0){
		while (tx_data_blocking((unsigned char *)packet_from_work_queue, packet_from_work_queue->data_packet.size + sizeof(data_packet_t)) != 0){
			//implement a failsafe
		}
		free(packet_from_work_queue);
	}else if(status == -1){
		//empty work queue 
	}else{
		//queue error further checks should implemented
	}

	return 0;

}



int send_to_beacon(uint8_t* data, int len){

	//this function will add packets to mesh_engines work queue
	init_put_data_for_work_queue(data, len, BEACON_ADDR, get_next_node_addr_to_beacon());
	return 0;
}

