#ifndef ALARM_H
#define ALARM_H

#include <string.h>

#include "driver/ledc.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "freertos/task.h"
#include "esp_gap_ble_api.h" // esp_ble_gap_start_scanning

// Keypad rows and cols
#define GPIO_COL_0            12
#define GPIO_COL_1            13
#define GPIO_COL_2            14
#define GPIO_OUTPUT_PIN_SEL   ((1ULL<<GPIO_COL_0) | (1ULL<<GPIO_COL_1) | (1ULL<<GPIO_COL_2))

#define GPIO_ROW_0            27
#define GPIO_ROW_1            26
#define GPIO_ROW_2            25
#define GPIO_ROW_3            33
#define GPIO_INPUT_PIN_SEL    ((1ULL<<GPIO_ROW_0) | (1ULL<<GPIO_ROW_1) | (1ULL<<GPIO_ROW_2) | (1ULL<<GPIO_ROW_3))

// Buzzer and LED alarm indications
#define LEDC_HS_TIMER         LEDC_TIMER_0
#define LEDC_HS_MODE          LEDC_HIGH_SPEED_MODE
#define LEDC_HS_CH0_BUZZER    18
#define LEDC_HS_CH0_CHANNEL   LEDC_CHANNEL_0
#define LEDC_HS_CH1_LED_R     19
#define LEDC_HS_CH1_CHANNEL   LEDC_CHANNEL_1
#define LEDC_HS_CH2_LED_Y     5
#define LEDC_HS_CH2_CHANNEL   LEDC_CHANNEL_2

#define LEDC_CH_NUM           3
#define LEDC_DUTY             2000
#define LEDC_FADE_TIME        3000
#define TASK_WAIT             150

#define SECURITY_SYSTEM       "SECURITY_SYSTEM"

#define MAX_NUMBER_OF_SENSORS 5

typedef enum state_enum {Setup, Disarmed, Activating, Armed, Alarm} state_enum_t;
typedef enum scan_enum {Just_scan, Add_new, Search_known} scan_enum_t;

extern scan_enum_t *scan_type_ptr;

typedef struct{
    uint8_t address[6];
    uint8_t missed_beeps;
} sensor_t;

void activate_security();

esp_err_t arm_system();

void alarm_task();

void search_devices_task();

void stop_alarm_task();

void compare_codes();

void hadle_keypad_task();


#endif
