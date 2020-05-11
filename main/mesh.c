//#include "mesh.h"
#include "communication.h"
#include "esp_log.h"
#include "mesh_defs.h"
#include "routing.h"
#include "rx_tx_registerer.h"
#include <string.h>
#define TAG "mesh.c"
#define MAX_QUEUE_SIZE 20

int head_u = 0;
int tail_u = 0;


general_payload_t* mesh_user_queue[MAX_QUEUE_SIZE];

void start_meshng(struct meshng_parameters_t *parameters){
    ESP_LOGI(TAG,"Start Meshng %d",parameters->with_rtseq );
	DEV_ID = parameters->addr;
	meshng_state = without_init;
    if(parameters->with_rtseq == 1){
	   start_routing_seq(parameters->type);
       meshng_state = routing_end;
    }else{
        meshng_state = no_routing;
    }
    
    ESP_LOGI(TAG,"\nnew state routing_end,%d\n", meshng_state);
	
	while(true){
		mesh_engine();
        vTaskDelay(100 / portTICK_PERIOD_MS);
	}
}


general_payload_t* get_user_queue(int *status){
    int ret = 0,tmp = 0;
    //ESP_LOGI(TAG,"get_user_queue status %d head_u:%d, tail_u:%d",ret, head_u,tail_u);
    if(head_u < tail_u){
    	tmp = head_u;
    	head_u++;
    	*status = 0;
    }else if(head_u == tail_u){
    	*status = -1;
    	//empty
    	if(head_u >= MAX_QUEUE_SIZE - 1){
    		tmp = head_u;
    		head_u = 0;
    		tail_u = 0;
    	}
    }
    //ESP_LOGI(TAG,"geT_mesh_user_queue status %d head_q:%d, tail_u:%d",ret, head_u,tail_u);
    return mesh_user_queue[tmp];
}


int put_user_queue(general_payload_t* packet){
    int ret = 0;
    if(tail_u >= MAX_QUEUE_SIZE){
        ret = -1;//queue full
    }else{
        mesh_user_queue[tail_u] = packet;
        tail_u++;
    }
    #ifdef DEBUG_BUILD
        ESP_LOGI(TAG,"put_user_queue status %d head_q:%d, tail_u:%d",ret, head_u,tail_u);
    #endif
    return ret;
}

int put_dpacket_uqueue(data_packet_t* packet){
	data_packet_t* local_dpacket = malloc(packet->data_packet.size + sizeof(general_payload_t));
	if(local_dpacket != NULL){
		memcpy(local_dpacket, packet, packet->data_packet.size  + sizeof(general_payload_t));
		put_user_queue(local_dpacket);
		return 0;
	}else{
		return -1;
		//not enough mem
	}
}