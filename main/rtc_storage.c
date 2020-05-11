#include "rtc_storage.h"
#include <string.h>
#define MEM_ADDR 0x60001202
#define MAX_SIZE 700
typedef struct{
    int size;
    char *data;
}rtc_storage_t;

rtc_storage_t rtc_storage = {
                                .size = 0,
                                .data = (char*)MEM_ADDR
                            };


int init_storage(){
    return 0;
}

char* read_storage(int len, char *status){
    if(rtc_storage.size - len < 0){
        *status = -1;
    }
    *status = 0;
    return rtc_storage.data + rtc_storage.size - len;
}

int put_storage(char *buff, int len){
    if(len+rtc_storage.size > MAX_SIZE){
        return -1;//not enough mem
    }
    memcpy(rtc_storage.data+rtc_storage.size, buff, len);
    rtc_storage.size += len;
    return 0;
}

/*
4

0 1 2 3

*/