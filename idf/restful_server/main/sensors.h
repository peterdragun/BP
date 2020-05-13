#ifndef SENSORS_H
#define SENSORS_H

#include <time.h>

#include "esp_log.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "driver/gpio.h"

#include "alarm.h"

#define MAX_NUMBER_OF_SENSORS 5

#define SENSORS_TAG "sensors"

extern state_enum_t *security_state;
extern time_t last_alarm;
extern ledc_channel_config_t ledc_channel[LEDC_CH_NUM];
extern TaskHandle_t xHandle_alarm;
extern TaskHandle_t xHandle_search;

typedef enum sensors_type {PIR, Magnetic} sensors_type_t; 

typedef struct{
    uint8_t address[6];
    sensors_type_t type;
    uint8_t missed_beeps;
    time_t last_connection;
    time_t last_alarm;
} sensor_t;

esp_err_t compare_uint8_array(uint8_t *uuid1, uint8_t *uuid2, uint8_t size);

esp_err_t record_sensor(uint8_t *address);

esp_err_t add_new_sensor(uint8_t *address);

esp_err_t remove_sensor(uint8_t *address);

/*
* Checks if address belongs to one of defined senzors and then set alarm.
*/
esp_err_t record_alarm(uint8_t *address);

int not_responding();

void increment_sensor_task();

#endif
