/* sniffer example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
//#define configUSE_PREEMPTION 0
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include <driver/gpio.h>
#include <esp_spi_flash.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "rx_tx_registerer.h"
#include "esp_libc.h"

#include "mesh.h"
#include "softuart.h"
#include "sensor.h"
#include "product_main.h"

#define TAG "sniffer"

#define MAC_HEADER_LEN 24
#define SNIFFER_DATA_LEN 112
#define MAC_HDR_LEN_MAX 40

static EventGroupHandle_t wifi_event_group;

static const int START_BIT = BIT0;

static void sniffer_task(void* pvParameters)
{
    wifi_promiscuous_filter_t sniffer_filter = {0};
    sniffer_filter.filter_mask |= WIFI_PROMIS_FILTER_MASK_CTRL;
    sniffer_filter.filter_mask |= WIFI_PROMIS_FILTER_MASK_DATA;
    sniffer_filter.filter_mask |= WIFI_PROMIS_FILTER_MASK_MGMT;
#if CONFIG_FILTER_MASK_MGMT
    
#endif

#if CONFIG_FILTER_MASK_CTRL
    
#endif

#if CONFIG_FILTER_MASK_DATA

#endif

#if CONFIG_FILTER_MASK_DATA_FRAME_PAYLOAD
    /*Enable to receive the correct data frame payload*/
    extern esp_err_t esp_wifi_set_recv_data_frame_payload(bool enable_recv);
    ESP_ERROR_CHECK(esp_wifi_set_recv_data_frame_payload(true));
#endif

#if CONFIG_FILTER_MASK_MISC
    sniffer_filter.filter_mask |= WIFI_PROMIS_FILTER_MASK_MISC;
#endif

    if (sniffer_filter.filter_mask == 0) {
        ESP_LOGI(TAG, "Please add one filter at least!");
        vTaskDelete(NULL);
    }

    xEventGroupWaitBits(wifi_event_group, START_BIT,
                        false, true, portMAX_DELAY);
    ESP_ERROR_CHECK(esp_wifi_set_channel(CONFIG_CHANNEL, 0));
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(register_rx_tx(esp_wifi_80211_tx)));
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_filter(&sniffer_filter));
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));
    vTaskDelete(NULL);
}

static esp_err_t event_handler(void* ctx, system_event_t* event)
{
    switch (event->event_id) {
        case SYSTEM_EVENT_STA_START:
            xEventGroupSetBits(wifi_event_group, START_BIT);
            break;

        default:
            break;
    }

    return ESP_OK;
}

static void initialise_wifi(void)
{
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(0x0);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
}


void init_hw(struct meshng_parameters_t* parameters)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    initialise_wifi();
    xTaskCreate(&sniffer_task, "sniffer_task", 2048, NULL, 10, NULL);
    //xTaskCreate(&start_routing_seq, "routing_task", 2048, NULL, 9, NULL);
    //xTaskCreate(&, "print_task", 2048, NULL, 10, NULL);

  
    esp_wifi_set_channel(4,WIFI_SECOND_CHAN_NONE);
         
//   uint8_t test_str[] = "0EST_DATA_TO_BEACON 0x2222";
//   const char testasd[] = "assasdasd";

    esp_wifi_set_max_tx_power(-128);
    //start_routing_seq(0); //0b 1s
//    ESP_LOGI("sniffer","routing :%hx\n", get_next_node_addr_to_beacon());
   //struct meshng_parameters_t parameters;
   vTaskDelay(100 / portTICK_PERIOD_MS);
   //start_meshng(sta,0x2222);
   ESP_LOGI(TAG,"MESHNG START: %ld",xTaskCreate(&start_meshng, "routing_task", 4096, parameters, 11, NULL));
//   xTaskCreate(&start_meshng, "routing_task", 2048, &parameters, 9, NULL);
    
//	if(send_to_beacon(test_str, sizeof(test_str)) == 0){

//	}					   rx  tx


 //   softuart_open(0, 9600, 14, 13);
 //   init_sensors();
 //   sensor_data_packet_t sensor_datas;
 //   char flash_w_test = "123";


    //product_main_task();
    //while(1){
        //ESP_LOGI(TAG,"%d",esp_wifi_80211_tx(WIFI_IF_STA, beacon_raw, sizeof(beacon_raw), 1));
        //ESP_LOGI(TAG,"%d",esp_wifi_80211_tx(WIFI_IF_STA, frame_buffer+frame_buffer[4], 40, 1));
//	    for (int index = 0; index < 5; index++){

//		}
        



   
   // }

}
