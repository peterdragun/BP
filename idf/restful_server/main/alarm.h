/**
* @file  alarm.h
*
* @brief Handlings for alarm and security states
* @author Peter Dragun (xdragu01)
*/

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
#define GPIO_COL_0            12 /*!< Pin connected to column 0 of keypad*/
#define GPIO_COL_1            13 /*!< Pin connected to column 1 of keypad*/
#define GPIO_COL_2            14 /*!< Pin connected to column 2 of keypad*/
/*! Output pin bitmask for keypad*/
#define GPIO_OUTPUT_PIN_SEL   ((1ULL<<GPIO_COL_0) | (1ULL<<GPIO_COL_1) | (1ULL<<GPIO_COL_2))

#define GPIO_ROW_0            27 /*!< Pin connected to row 0 of keypad*/
#define GPIO_ROW_1            26 /*!< Pin connected to row 1 of keypad*/
#define GPIO_ROW_2            25 /*!< Pin connected to row 2 of keypad*/
#define GPIO_ROW_3            33 /*!< Pin connected to row 3 of keypad*/
/*! Input pin bitmask for keypad */
#define GPIO_INPUT_PIN_SEL    ((1ULL<<GPIO_ROW_0) | (1ULL<<GPIO_ROW_1) | (1ULL<<GPIO_ROW_2) | (1ULL<<GPIO_ROW_3))

// Buzzer and LED alarm indications
#define LEDC_HS_MODE          LEDC_HIGH_SPEED_MODE /*!< Mode of LEDC */
#define LEDC_HS_CH0_BUZZER    18 /*!< Pin connected to buzzer */
#define LEDC_HS_CH1_LED_R     19 /*!< Pin connected to red alarm diode*/
#define LEDC_HS_CH2_LED_Y     5 /*!< Pin connected to yellow warning diode*/

#define LEDC_CH_NUM           3 /*!< Number of LEDC channels */
#define LEDC_DUTY             2000 /*!< Duty of LEDC*/
#define TASK_WAIT             150 /*!< Delay between beeps of alarm*/

#define SECURITY_SYSTEM       "SECURITY_SYSTEM"  /*!< Tag for logging*/

/**
 * @brief State of security system
 */
typedef enum state_enum {Setup, Disarmed, Activating, Armed, Alarm} state_enum_t;

/**
 * @brief BLE scan types for BLE devices
 */
typedef enum scan_enum {Just_scan, Add_new, Search_known} scan_enum_t;

// global variables
extern scan_enum_t *scan_type_ptr;

/**
 * @brief Convert enum of security state to string
 * 
 * @return String representation of security state
 */
char *state_to_str();

/**
 * @brief Task for activating system
 */
void activate_security();

/**
 * @brief Activate security system
 * 
 * @return On successs return ESP_OK. On fail return ESP_FAIL
 */
esp_err_t arm_system();

/**
 * @brief Task for alarm
 */
void alarm_task();

/**
 * @brief Task for searching BLE devices
 */
void search_devices_task();

/**
 * @brief Task to make sure alarm stoped
 */
void stop_alarm_task();

/**
 * @brief Compare code from keypad with stored code. If wrong code was entered 3 times start alarm.
 * This function is triggered by pressing # on keypad. On '*#' change state Setup->Disarmed or Disarmed->Activating.
 */
void compare_codes();

/**
 * @brief Task for scanning kaypad presses
 */
void hadle_keypad_task();

/**
 * @brief Task for long beep
 */
void long_beep();

#endif //ALARM_H
