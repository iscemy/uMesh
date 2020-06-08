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

#include <mqtt_client.h>

#include "servercomm.h"

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
    //ESP_ERROR_CHECK(esp_wifi_set_channel(CONFIG_CHANNEL, 0));
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(register_rx_tx(esp_wifi_80211_tx)));
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous_filter(&sniffer_filter));
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));
    vTaskDelete(NULL);
    return 0;
}

const static int CONNECTED_BIT = BIT0;
static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    /* For accessing reason codes in case of disconnection */
    system_event_info_t *info = &event->event_info;
    
    switch (event->event_id) {
        case SYSTEM_EVENT_STA_START:
            xEventGroupSetBits(wifi_event_group, START_BIT);
            esp_wifi_connect();
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);

            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            ESP_LOGE(TAG, "Disconnect reason : %d", info->disconnected.reason);
            if (info->disconnected.reason == WIFI_REASON_BASIC_RATE_NOT_SUPPORT) {
                /*Switch to 802.11 bgn mode */
                esp_wifi_set_protocol(ESP_IF_WIFI_STA, WIFI_PROTOCAL_11B | WIFI_PROTOCAL_11G | WIFI_PROTOCAL_11N);
            }
            esp_wifi_connect();
            xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
            break;
        default:
            break;
    }
    return ESP_OK;
}

static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    int msg_id, size;
    // your_context_t *context = event->context;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            char *ptr = (char*)get_all_data(&size);
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED %p", ptr);
            if(ptr != NULL){
                ESP_LOGI(TAG, "TRYING TO SEND");
                esp_mqtt_client_publish(client, "/topic/tasarimm", ptr, 0, 0, 0);
            }
            free(ptr);
            //msg_id = esp_mqtt_client_publish(client, "/topic/qos1", "data_3", 0, 1, 0);

            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
            ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            break;
    }
    return ESP_OK;
}

esp_mqtt_client_config_t mqtt_cfg = {
    .uri = "mqtt://test.mosquitto.org:1883",
    .event_handle = mqtt_event_handler,
    // .user_context = (void *)your_context
};

esp_mqtt_client_handle_t client;

void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = "mqtt://test.mosquitto.org:1883",
        .event_handle = mqtt_event_handler,
        // .user_context = (void *)your_context
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_start(client);
}

void mqtt_stop(){
    esp_mqtt_client_stop(client);
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

void init_wifi_for_mesh(){
        
    ESP_ERROR_CHECK(esp_wifi_stop());  
    ESP_ERROR_CHECK(esp_wifi_deinit())
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(0x0);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

}

void init_wifi_sta(){
    ESP_ERROR_CHECK(esp_wifi_set_promiscuous(false));  
    ESP_ERROR_CHECK(esp_wifi_stop());  
    ESP_ERROR_CHECK(esp_wifi_deinit()); 
   
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "ASUS",
            .password = "22200109",
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));

    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "Waiting for wifi");
    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
}


TaskHandle_t meshng_task_handle;

void start_mesh(struct meshng_parameters_t* parameters){
    ESP_LOGI(TAG,"MESHNG START: %ld",xTaskCreate(&start_meshng, "routing_task", 2048, parameters, 11, &meshng_task_handle));
}

void wifi_set_sta(){
    vTaskSuspend(meshng_task_handle);
    init_wifi_sta();
    
    
}

void wifi_set_mesh(){
    //mqtt_stop();
    init_wifi_for_mesh();     
    xTaskCreate(&sniffer_task, "sniffer_task", 2048, NULL, 10, NULL);
    vTaskDelay(200 / portTICK_PERIOD_MS);
    esp_wifi_set_channel(4,WIFI_SECOND_CHAN_NONE);
    vTaskResume(meshng_task_handle);
}

void init_hw(struct meshng_parameters_t* parameters)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    initialise_wifi();
    
    xTaskCreate(&sniffer_task, "sniffer_task", 2048, NULL, 10, NULL);

    vTaskDelay(200 / portTICK_PERIOD_MS);

    esp_wifi_set_channel(4,WIFI_SECOND_CHAN_NONE);
    mqtt_app_start();
    esp_wifi_set_max_tx_power(60);
#if DEV_TYPE == BEACON
    start_mesh(parameters);
#else
    start_mesh(parameters);
#endif


}
