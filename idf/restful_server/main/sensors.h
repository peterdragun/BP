/**
* @file  sensors.h
*
* @brief Operations with sensors
* @author Peter Dragun (xdragu01)
*/

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

#define DISARMED_WAIT 40
#define ARMED_WAIT 20
#define REPEAT_WAIT 20

extern state_enum_t *security_state;
extern time_t last_alarm;
extern ledc_channel_config_t ledc_channel[LEDC_CH_NUM];
extern TaskHandle_t xHandle_alarm;
extern TaskHandle_t xHandle_search;

/**
 * @brief Types of sensors
 */
typedef enum sensors_type {PIR, Magnetic} sensors_type_t; 

/**
 * @brief Sensor structure
 */
typedef struct{
    uint8_t address[6];     /**< Bluetooth address of sensor*/
    sensors_type_t type;    /**< Type of sensor*/
    uint8_t missed_beeps;   /**< Number of missed connections*/
    time_t last_connection; /**< Timestamp of last connection*/
    time_t last_alarm;      /**< Timestamp of last alarm*/
} sensor_t;

/**
 * @brief Compare two arrays of same lenght specified by parameter.
 * 
 * @param uuid1 First array
 * @param uuid2 Second array
 * @param size Size of arrays
 * 
 * @return If arrays are same function returns ESP_OK otherwise ESP_FAIL is returned
 */
esp_err_t compare_uint8_array(uint8_t *uuid1, uint8_t *uuid2, uint8_t size);

/**
 * @brief Set sensors last connection value to present time and set missed_beeps to 0
 * 
 * @param address Sensor address
 * 
 * @return On successs return ESP_OK. If sensor does not exists return ESP_FAIL
 */
esp_err_t record_sensor(uint8_t *address);

/**
 * @brief Add new sensor to known sensors
 * 
 * @param address Sensor address
 * 
 * @return On successs return ESP_OK. If sensor already exists return ESP_FAIL
 */
esp_err_t add_new_sensor(uint8_t *address);

/**
 * @brief Remove sensors from known sensors array and remove from bonded devices
 * 
 * @param address Sensor address
 * 
 * @return On successs return ESP_OK. If sensor does not exists return ESP_FAIL
 */
esp_err_t remove_sensor(uint8_t *address);

/**
 * @brief Checks if address belongs to one of defined senzors and then set alarm.
 * 
 * @param address Address of sensor which triggered alarm.
 * 
 * @return On successs return ESP_OK. If sensor does not exists return ESP_FAIL
 */
esp_err_t record_alarm(uint8_t *address);

/**
 * @brief Checking value of missed beeps on all sensors. All grater then 3 are counted as not responding.
 * 
 * @return Number of not responding sensors
 */
int not_responding();

/**
 * @brief Task for incrementing missed_beeps on all sensors
 */
void increment_sensor_task();

#endif // SENSORS_H
