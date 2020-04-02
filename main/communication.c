#include "communication.h"

#include "routing.h"
#include "esp_log.h"
#include "mesh_defs.h"
#include "mesh_io.h"



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

int packet_classifier(general_payload_t* packet){
	uint16_t new_target_node = 0;
	if(packet->destination == BEACON_ADDR){
		if(packet->target == DEV_ID){
			if(packet->type == data){		//only route data packets 
				new_target_node = get_next_node_addr_to_beacon();
				if(new_target_node != 0){	//is routing table exists ?
					packet->target = new_target_node;
					if(put_meshng_work_queue(packet) != 0){
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

int mesh_engine(){
	/*
		it has to checks state before proceeding
		two diffrent packet sources:
			1- from other nodes
			2- itself
		this two sources must stored in memory in organized manner (like a queue)
	*/
	

	return 0;

}

int send_to_beacon(char* data, int len){

	//this function will add packets to mesh_engines work queue
	return 0;
}

