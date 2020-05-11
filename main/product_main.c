#include "product_main.h"
#include "esp_log.h"
#include <esp_system.h>
#include <esp_sleep.h>
#include <string.h>


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "mesh.h"
#include "softuart.h"
#include "sensor.h"


#include <esp_heap_caps.h>

uint16_t *rtcMemory = (long *)0x60001200;

#include "rtc_storage.h"
#include "driver/hw_timer.h"
#include "routing.h"
#define TAG "product_main"

#define DEEP_SLEEP(sec) esp_deep_sleep(sec*1000*1000);
#define BEACON 0
#define SENSOR 1
#define ZIBIK 2
#define BEACON_FIXED_ADDR 0xbeef
#define DEV_TYPE SENSOR

extern void init_hw();
extern void* get_rssi_table(int *size);


struct meshng_parameters_t parameters;

enum sensor_data_types{
	sensor_data = 0,
	sensor_rssi_table
};

typedef struct {
	unsigned short addr;
	char packet_type;
	sensor_data_packet_t sensor_data;
}sensor_data_unit_t;

typedef struct {
	unsigned short addr;
	char packet_type;
	uint16_t size;
	char payload[0];
}sensor_data_unit_header_t;

void reset_chip(){

}


void hw_timer_callback(void *arg){
	//ESP_LOGI(TAG,"GOING FOR DEEP_SLEEP");
	DEEP_SLEEP(5);
}

void app_main(){
	int reset_cause = esp_reset_reason();
	unsigned char mac_addr[6];
	
	if(esp_efuse_mac_get_default(mac_addr) != 0){
		//shit happned
		return -1;
	}

	unsigned short device_addr = *((unsigned short*)&mac_addr[4]);//addr is least significant 2 bytes of mac addr
#if DEV_TYPE == BEACON
	parameters.addr = BEACON_FIXED_ADDR;
#else
    parameters.addr = device_addr;
#endif

    parameters.type = DEV_TYPE;
    ESP_LOGI(TAG,"reset reason: %d", reset_cause);
    if(reset_cause == ESP_RST_DEEPSLEEP){
    	if(DEV_TYPE == SENSOR){
    		hw_timer_init(hw_timer_callback, NULL);
			hw_timer_alarm_us(0x199999, false);
    	}
		parameters.with_rtseq = 0;
		ESP_LOGI(TAG,"next dev addr 0x%hx", *rtcMemory);
	}else{
		parameters.with_rtseq = 1;
	}
	ESP_LOGI("_________________________________________","\nrtseq = %d\n\n\n", parameters.with_rtseq);
	init_hw(&parameters);

	
	
    //start_meshng(&parameters);
    ESP_LOGI(TAG,"mac_addr %hx:%hx:%hx:%hx:%hx:%hx	dev_type %d, devaddr:%hx", mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5], DEV_TYPE, device_addr);

    uint8_t soft_uart_no = 0;
    //const char test[] = "AT";
	if(DEV_TYPE == BEACON){
		softuart_open(soft_uart_no, 9600, 14, 13); //initiliazation of soft uart for gps
		//softuart_puts(soft_uart_no,test);
		ESP_LOGI(TAG,"BEACON");

	}else if(DEV_TYPE == SENSOR){
		init_sensors();
		ESP_LOGI(TAG,"SENSOR");
	}
	unsigned char test_str[] = "test";
	

    //tast_ret = xTaskCreate(&start_meshng, "meshng task", 4096, &parameters, 22, NULL);
    //esp_deep_sleep(5*1000*1000);
	while((meshng_state!=routing_end)&&(meshng_state != no_routing)){
		vTaskDelay(200 / portTICK_PERIOD_MS);
	}

	sensor_data_unit_header_t* rssi_table_pkt = NULL;


/*
	char stupity_test[] = "asdasd";
	data_packet_t *recved_packet_;
	int stat;
	while(1){
		recved_packet_ = (data_packet_t*) get_user_queue(&stat);
		if(stat == 0){
			*(recved_packet_->data_packet.data + recved_packet_->data_packet.size - 1) = 0;
			ESP_LOGI(TAG,"\n%s\n", recved_packet_->data_packet.data);
			
			//send_to_beacon((uint8_t*)stupity_test, sizeof(stupity_test));
		} 
		vTaskDelay(20 / portTICK_PERIOD_MS);
		//send_to_beacon(stupity_test, sizeof(stupity_test);
	}
*/
#if DEV_TYPE == BEACON
	int status = 0;
	rssi_table_element_t *rssi_table_element = NULL;
	data_packet_t* recved_packet;
	while(1){
		recved_packet = (data_packet_t*) get_user_queue(&status);
		if(status == 0){
			sensor_data_unit_t *recved_sensor_data = recved_packet->data_packet.data;
			ESP_LOGI(TAG,"PACKET AT USER type:%d %hx", recved_sensor_data->packet_type, recved_packet->header.destination);
			if(recved_sensor_data->packet_type == sensor_data){
				ESP_LOGI(TAG,"PACKET AT USER type:%d", recved_sensor_data->packet_type);
				ESP_LOGI(TAG,"from %hx BEACON_RECVED_DATA temp:%d hum:%d inf1:%hx inf2:%hx",recved_sensor_data->addr,recved_sensor_data->sensor_data.temp,
							 recved_sensor_data->sensor_data.humidty, recved_sensor_data->sensor_data.infrared_1, recved_sensor_data->sensor_data.infrared_2);
			}else if(recved_sensor_data->packet_type == sensor_rssi_table){
				sensor_data_unit_header_t *rssi_table = recved_packet->data_packet.data;
				ESP_LOGI(TAG,"\nGOT RSSI_TABLE_PACKET,rssi_table->siz:%d %hx", rssi_table->size, rssi_table->addr);

				for(int index_r = 0; index_r < rssi_table->size; index_r += sizeof(rssi_table_element_t)){
					rssi_table_element = rssi_table->payload + index_r;
					ESP_LOGI(TAG,"ADDR: %hx RSSI:%d", rssi_table_element->addr, rssi_table_element->rssi);
				}
				
			}
		}
		vTaskDelay(10 / portTICK_PERIOD_MS);
	}

#else
/* Sensor */
	int sizeof_rssi_table;
	char *rssi_table_;
	vTaskDelay(10000 / portTICK_PERIOD_MS);
	if(parameters.with_rtseq == 1){
		hw_timer_init(hw_timer_callback, NULL);
		hw_timer_alarm_us(0x199999, false);
		rssi_table_ = get_rssi_table(&sizeof_rssi_table);
		rssi_table_pkt = malloc(sizeof_rssi_table + sizeof(sensor_data_unit_header_t));
		rssi_table_pkt->addr = device_addr;
		rssi_table_pkt->packet_type = sensor_rssi_table;
		ESP_LOGI(TAG,"Trying to s %p %hx %d %hx %d", rssi_table_, *((uint16_t*)rssi_table_),*((int8_t*)rssi_table_ + 2),*((uint16_t*)rssi_table_ + 4),*((int8_t*)rssi_table_+6));
		rssi_table_pkt->size = sizeof_rssi_table;
		memcpy(rssi_table_pkt->payload, rssi_table_, sizeof_rssi_table);
		ESP_LOGI(TAG,"Trying to send rssi_table pkt:%p tablesize:%d, totalpktsize:%d", rssi_table_pkt, sizeof_rssi_table,sizeof_rssi_table + sizeof(sensor_data_unit_header_t));
		send_to_beacon(rssi_table_pkt,sizeof_rssi_table + sizeof(sensor_data_unit_header_t));			
	}
	sensor_data_unit_t *sensor_data_unit = (sensor_data_unit_t*) malloc(sizeof(sensor_data_unit_t));
	sensor_data_unit->addr = device_addr;
	sensor_data_unit->packet_type = sensor_data;
	ESP_LOGI(TAG,"SENSOR SENDING DATA TO BEAOCN, %d, remainig heaps size %d", sizeof(sensor_data_packet_t),heap_caps_get_free_size(MALLOC_CAP_8BIT));

	get_sensor_data(&sensor_data_unit->sensor_data);
	ESP_LOGI(TAG,"remainig heaps size %d", heap_caps_get_free_size(MALLOC_CAP_8BIT));
	send_to_beacon(sensor_data_unit, sizeof(sensor_data_unit_t));
	vTaskDelay(1000 / portTICK_PERIOD_MS);
	free(sensor_data_unit);
	if(rssi_table_pkt) free(rssi_table_pkt);
#endif


	return 0;
}
