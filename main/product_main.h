#define DEEP_SLEEP(sec) esp_deep_sleep(sec*1000*1000);
#define BEACON 0
#define SENSOR 1
#define ZIBIK 2
#define BEACON_FIXED_ADDR 0xbeef
#define DEV_TYPE BEACON     
#define MAX_RSSI_TABLE_ARRAY_SIZE 20

#define LOOP_DELAY_PARAM 0x199999
#define DEEP_SLEEP_T 25

void reset_chip();
int product_main_task();

unsigned short device_addr;

