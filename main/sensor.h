#ifndef __SENSOR_H__
#define __SENSOR_H__


typedef struct  {
    int16_t temp;
    int16_t humidty;
    uint16_t infrared_1;
    uint16_t infrared_2;
}sensor_data_packet_t;

enum{
    inf_channel_1=0,
    inf_channel_2
}inf_channel_x;


void init_sensors();
int get_sensor_data(sensor_data_packet_t* sensor_data);

#endif 
