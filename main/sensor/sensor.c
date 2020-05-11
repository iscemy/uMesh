#include "dht.h"
#include "sensor.h"
#include "driver/gpio.h"
#include "driver/adc.h"

#define ADC_SAMPLE_CNT 5
#define INF_SENSOR_SEL_PIN (14)
#define DHT_11_DATA_PIN (13)

int set_inf_channel(char channel){
    if(channel == inf_channel_1){
        gpio_set_level(INF_SENSOR_SEL_PIN, 0);
    }else if(channel == inf_channel_2){
        gpio_set_level(INF_SENSOR_SEL_PIN, 1);
   }
}

int get_adc_infx(uint16_t *result, char channel){
    uint16_t adc_val;    
    set_inf_channel(channel);
    if(result == NULL){
        return -1;
    }
    *result = 0;
    for(int i = 0; i< ADC_SAMPLE_CNT; i++){
        while(adc_read(&adc_val) != 0);
        *result += adc_val;
    }
    *result = *result/ADC_SAMPLE_CNT;
    return 0;
}

int get_sensor_data(sensor_data_packet_t* sensor_data){
    uint16_t adc_data_inf1, adc_data_inf2;

    if(sensor_data == NULL){
        return -1;
    }
    dht_read_data(DHT_TYPE_DHT11, GPIO_Pin_13, &sensor_data->humidty, &sensor_data->temp);
    if(get_adc_infx(&sensor_data->infrared_1, inf_channel_1) != 0){
        //error handling
        return -1;
    }
    if(get_adc_infx(&sensor_data->infrared_2, inf_channel_2) != 0){
        //error handling
        return -1;
    }

    return 0;
}


void init_sensors(){
    adc_config_t adc_config;
    gpio_config_t io_conf;

    //Adc Init
    adc_config.mode = ADC_READ_TOUT_MODE;
    adc_config.clk_div = 8;
    ESP_ERROR_CHECK(adc_init(&adc_config));

    
    //Sensor Select
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = 1ULL<<INF_SENSOR_SEL_PIN;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);


}


