#ifndef ALARM_H
#define ALARM_H

#include <string.h>

#include "driver/ledc.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "freertos/task.h"

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
#define LEDC_HS_CH1_GPIO      19
#define LEDC_HS_CH1_CHANNEL   LEDC_CHANNEL_1

#define LEDC_TEST_CH_NUM      4
#define LEDC_TEST_DUTY        2000
#define LEDC_TEST_FADE_TIME   3000
#define TASK_WAIT             150

#define SECURITY_SYSTEM       "SECURITY_SYSTEM"

typedef enum state_enum {Disarmed, Armed, Activating, Alarm, Setup} state_enum_t;

void activate_security();

esp_err_t arm_system();

void alarm_task();

void compare_codes();

void hadle_keypad_task();


#endif
