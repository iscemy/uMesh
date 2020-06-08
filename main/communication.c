#include "communication.h"
#include "mesh_defs.h"
#include "routing.h"
#include "esp_log.h"
#include "mesh_defs.h"
#include "mesh_io.h"
#include <string.h>
#include "hash_map.h"

#define TAG "communication.c"
#define MAX_QUEUE_SIZE 10
#define BROADCAST_REPEAT_N 1

int head_q = 0;
int tail_q = 0;

general_payload_t* mesh_ng_queue[MAX_QUEUE_SIZE];

int put_user_queue(general_payload_t* packet);


int put_meshng_work_queue(general_payload_t* packet);

int init_put_for_work_queue(data_packet_bottom_t *data, int len, int target, int destination) {
	general_payload_t* packet = malloc(len + sizeof(general_payload_t));
	ESP_LOGI(TAG,"local packet addr: %p", packet);
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
uint8_t seq_id = 1;
int init_put_for_work_queue_wseqid(data_packet_bottom_t *data, int len, int target, int destination) {//only for broadcast_command packets  this function can be extended further
	general_payload_t* packet = malloc(len + sizeof(general_payload_t));
	ESP_LOGI(TAG,"local packet addr: %p", packet);
	if(packet != NULL){
		packet->destination = destination;
		packet->target = target;
		packet->type = broadcast_commands;
		packet->sender = DEV_ID;        
        packet->seq_id = seq_id;
        seq_id++;
        
		memcpy(packet->data, data, len);
		put_meshng_work_queue(packet);
        ESP_LOGI(TAG,"\n seq_id %d",packet->seq_id);
		return 0;
	} 
	return -1;
}

int init_put_for_work_queue_wdseqid(data_packet_bottom_t *data, int len, int target, int destination, uint8_t seq_id_) {
	general_payload_t* packet = malloc(len + sizeof(general_payload_t));
	ESP_LOGI(TAG,"local packet addr: %p", packet);
	if(packet != NULL){
		packet->destination = destination;
		packet->target = target;
		packet->type = broadcast_commands;
		packet->sender = DEV_ID;        
        packet->seq_id = seq_id_;
        
		memcpy(packet->data, data, len);
		put_meshng_work_queue(packet);
        ESP_LOGI(TAG,"\n seq_id %d",packet->seq_id);
		return 0;
	} 
	return -1;
}

int send_broadcast_data_packet(uint8_t* data, int len){

	//this function will add packets to mesh_engines work queue
	data_packet_bottom_t* data_packet = malloc(sizeof(data_packet_bottom_t) + len);
	if (data_packet != NULL){
		if((meshng_state == routing_end)||(meshng_state == no_routing)){
			data_packet->size = len;
			memcpy(data_packet->data, data, data_packet->size);
			ESP_LOGI(TAG,"Burdamiyiz packet size: %hu", data_packet->size);
			init_put_for_work_queue_wseqid(data_packet,sizeof(data_packet_bottom_t) + len, BROADCAST_ADDR, BROADCAST_ADDR);
			free(data_packet);
		}else{
            free(data_packet);
			return -2;//routing not inited yet
		}
	}else{
		return -1;//not enough mem
	}
	return 0;
}

int send_broadcast_data_packet_woseqid(uint8_t* data, int len, uint8_t seq_id){
    
	//this function will add packets to mesh_engines work queue
	data_packet_bottom_t* data_packet = malloc(sizeof(data_packet_bottom_t) + len);
    ESP_LOGI(TAG,"send_broadcast_data_packet_woseqid %p", data_packet);
	if (data_packet != NULL){
		if((meshng_state == routing_end)||(meshng_state == no_routing)){
			data_packet->size = len;
			memcpy(data_packet->data, data, data_packet->size);
			ESP_LOGI(TAG,"Burdamiyiz packet size: %hu", data_packet->size);
			init_put_for_work_queue_wdseqid(data_packet,sizeof(data_packet_bottom_t) + len, BROADCAST_ADDR, BROADCAST_ADDR, seq_id);
			free(data_packet);
		}else{
            free(data_packet);
			return -2;//routing not inited yet
		}
	}else{
		return -1;//not enough mem
	}
	return 0;
}

int put_meshng_work_queue(general_payload_t* packet){
    int ret = 0;
    if(tail_q >= MAX_QUEUE_SIZE){
        ret = -1;//queue full
    }else{
        mesh_ng_queue[tail_q] = packet;
        tail_q++;
    }
    #ifdef DEBUG_BUILD
        ESP_LOGI(TAG,"put_meshng_work_queue status %d head_q:%d, tail_q:%d",ret, head_q,tail_q);
    #endif
    return ret;
}

general_payload_t* get_meshng_work_queue(int *status){
    int ret = 0,tmp = 0;
    //ESP_LOGI(TAG,"get_meshng_work_queue status %d head_q:%d, tail_q:%d",ret, head_q,tail_q);
    if(head_q < tail_q){
    	tmp = head_q;
    	head_q++;
    	*status = 0;
    }else if(head_q == tail_q){
    	*status = -1;
    	//empty
    	if(head_q >= MAX_QUEUE_SIZE - 1){
    		tmp = head_q;
    		head_q = 0;
    		tail_q = 0;
    	}
    }
   
    return mesh_ng_queue[tmp];
}


int put_dpacket_wqueue(data_packet_t* packet){
	data_packet_t* local_dpacket = malloc(packet->data_packet.size + sizeof(data_packet_t));
	if(local_dpacket != NULL){
		memcpy(local_dpacket, packet, packet->data_packet.size  + sizeof(data_packet_t));
	    ESP_LOGI("put dpacket wqueue dump",":");
	    for(int i = 0; i < packet->data_packet.size  + sizeof(data_packet_t); i++){
	        printf("%hx ",*((uint8_t*)packet + i));
	    }
		put_meshng_work_queue(local_dpacket);
		return 0;
	}else{
		return -1;
		//not enough mem
	}
}

int packet_classifier(general_payload_t* packet){
	uint16_t new_target_node = 0;
	data_packet_t *data_packet;
	ESP_LOGI(TAG,"packet_classifier dest:%hx target:%hx", packet->destination, packet->target);
	if((packet->destination == BEACON_ADDR)&&(packet->target == DEV_ID)&&(packet->target != packet->destination)){
		//ESP_LOGI(TAG,"route");//string ise
		if(packet->type == data){		//only route data packets 
			new_target_node = get_next_node_addr_to_beacon();
			if(new_target_node != 0){	//is routing table exists ?
				packet->target = new_target_node;
				packet->sender = DEV_ID;
				ESP_LOGI(TAG,"packet_classifier route to beacon dest:%hx target:%hx", packet->destination, packet->target);//string ise
				if(put_dpacket_wqueue(packet) != 0){
					//error handling ng work queue
				}
			}else{
				//routing table does not exists
			}
		}
	}else if((packet->destination == BEACON_ADDR)&&(packet->target == DEV_ID)){
		//ESP_LOGI(TAG,"to me 	");//string ise
		if(DEV_ID == BEACON_ADDR){
			//this device is beacon and packet addressed to beacon
			if(packet->type == data){
				data_packet = (data_packet_t*)packet;
				ESP_LOGI(TAG,"BEACON RECVED DATA: %s", data_packet->data_packet.data);//string ise
				put_user_queue(packet);
			}
		}else{
			//p2p comm
		}
	}else if((packet->destination == BROADCAST_ADDR)&&(packet->target == BROADCAST_ADDR)&&((packet->type == data)||(packet->type == broadcast_commands))){
		//these packets should repeated n times
        ESP_LOGI(TAG,"broadcast %d",packet->seq_id);
		int *times_repeated = find_entry(packet->seq_id);
		int times_r;
        ESP_LOGI(TAG,"broadcast %d %p",packet->seq_id, times_repeated);
		if(times_repeated == NULL){
			int times = 0;
            insert_entry(packet->seq_id, sizeof(times), &times);
            put_user_queue(packet);
            send_broadcast_data_packet_woseqid(((data_packet_bottom_t*)packet->data)->data, ((data_packet_bottom_t*)packet->data)->size, packet->seq_id);
		}else{
			times_r = *times_repeated + 1;
			if(times_r < BROADCAST_REPEAT_N){
				delete_entry(packet->seq_id);
				insert_entry(packet->seq_id, sizeof(times_r), &times_r);
                send_broadcast_data_packet_woseqid(((data_packet_bottom_t*)packet->data)->data, ((data_packet_bottom_t*)packet->data)->size, packet->seq_id);
			}else{
                delete_entry(packet->seq_id);
            }
		}
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
	int status = 0;
	data_unit *packet_w_info;
	general_payload_t *received_packet;
	data_packet_t* packet_from_work_queue; //only data packets routed

	packet_w_info = get_data(&status);//polling from recved packet queue
	if(status == 0){
		received_packet = (general_payload_t*)packet_w_info->payload;
		packet_classifier(received_packet);//received packet classified and if its a data packet, addressed to beacon 
											//added to work queue
	}else{
		//cannot receive any data from mesh_io queue
	}
	
	packet_from_work_queue = get_meshng_work_queue(&status);//polling from work queue
	
	if(status == 0){
		ESP_LOGI(TAG,"@mesh_enigne packet_from_work_queue: %p seq_id %hx, size:%d", packet_from_work_queue,packet_from_work_queue->header.seq_id , packet_from_work_queue->data_packet.size + sizeof(data_packet_t));
        
		while (tx_data_blocking((unsigned char *)packet_from_work_queue, packet_from_work_queue->data_packet.size + sizeof(data_packet_t)) != 0){
			//implement a failsafe
		}
		free(packet_from_work_queue);
	}else if(status == -1){
		return -1;
	}else{
		//queue error further checks should be implemented
	}

	return 0;

}



int send_to_beacon(uint8_t* data, int len){

	//this function will add packets to mesh_engines work queue
	data_packet_bottom_t* data_packet = malloc(sizeof(data_packet_bottom_t) + len);
	if (data_packet != NULL){
		if((meshng_state == routing_end)||(meshng_state == no_routing)){
			data_packet->size = len;
			memcpy(data_packet->data, data, data_packet->size);
			ESP_LOGI(TAG,"sen Burdamiyiz packet size: %hu %hx", data_packet->size,get_next_node_addr_to_beacon());
			init_put_for_work_queue(data_packet,sizeof(data_packet_bottom_t) + len, get_next_node_addr_to_beacon(), BEACON_ADDR);
			free(data_packet);
		}else{
            free(data_packet);
			return -2;//routing not inited yet
		}
	}else{
		return -1;//not enough mem
	}
	return 0;
}




