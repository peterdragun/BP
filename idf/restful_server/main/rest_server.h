/**
* @file  rest_server.h
*
* @brief Api restful server handlers
* @author Peter Dragun (xdragu01)
*/

#ifndef REST_SERVER_H
#define REST_SERVER_H

#include <string.h>
#include <math.h> //log10

#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_vfs.h"
#include "cJSON.h"
#include "nvs.h"
#include "esp_gap_ble_api.h"
#include "cJSON.h"

#include "alarm.h"
#include "esp_main.h"
#include "sensors.h"

#define SCRATCH_BUFSIZE (10240) /*!< Size of buffer */

#define REST_TAG "REST" /*!< Tag for logging */
#define SECURITY_SYSTEM "SECURITY_SYSTEM" /*!< Tag for logging */
#define REST_CHECK(a, str, goto_tag, ...)                                              \
    do                                                                                 \
    {                                                                                  \
        if (!(a))                                                                      \
        {                                                                              \
            ESP_LOGE(REST_TAG, "%s(%d): " str, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
            goto goto_tag;                                                             \
        }                                                                              \
    } while (0) /*!< Check for errors. If error jump to end of inicialization. */

#define RSSI_AT_1_METER 9 - 62 /*!< Value of RSSI on one meter. +9dbm is current tx power. */
#define ENV_FACTOR      4 /*!< Environment constant. Used in distance to RSSI calculation */

// global variables
extern state_enum_t *security_state;
extern scan_enum_t *scan_type_ptr;
extern char expected_code[19];
extern uint8_t unknown_sensor[6];
extern uint8_t number_of_sensors;
extern uint8_t unknown_sensor_type;
extern sensor_t sensors[MAX_NUMBER_OF_SENSORS];
extern time_t last_alarm;
extern int8_t rssi;

/**
 * @brief Context of request
 */
typedef struct rest_server_context {
    char scratch[SCRATCH_BUFSIZE]; /**< Scratch*/
} rest_server_context_t;

cJSON *json_resp; /*!< BLE scan results */

/**
 * @brief Initialize structures for REST API server
 * 
 * @return On successs return ESP_OK. On error return ESP_FAIL
 */
esp_err_t start_rest_server();

#endif //REST_SERVER_H
