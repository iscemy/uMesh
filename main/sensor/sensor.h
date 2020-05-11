#ifndef __DHT_H__
#define __DHT_H__



typedef struct{
    int16_t temp;
    int16_t humidty;
    uint16_t infrared_1;
    uint16_t infrared_2;
}sensor_data_packet_t;

typedef enum{
    inf_channel_1=0,
    inf_channel_2
}

#endif  // __DHT_H__
