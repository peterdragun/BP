#ifndef REST_SERVER_H
#define REST_SERVER_H

#include <string.h>

#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_vfs.h"
#include "cJSON.h"
#include "nvs.h"
#include "esp_gap_ble_api.h"
#include "cJSON.h"

#include "alarm.h"
#include "esp_main.h"

#define SCRATCH_BUFSIZE (10240)

#define REST_TAG "esp-rest"
#define REST_CHECK(a, str, goto_tag, ...)                                              \
    do                                                                                 \
    {                                                                                  \
        if (!(a))                                                                      \
        {                                                                              \
            ESP_LOGE(REST_TAG, "%s(%d): " str, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
            goto goto_tag;                                                             \
        }                                                                              \
    } while (0)


#define SECURITY_SYSTEM "SECURITY_SYSTEM"

extern state_enum_t *security_state;

typedef struct rest_server_context {
    char base_path[ESP_VFS_PATH_MAX + 1];
    char scratch[SCRATCH_BUFSIZE];
} rest_server_context_t;

cJSON *json_resp; // BLE scan results

esp_err_t start_rest_server(const char *base_path);


#endif
