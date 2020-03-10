#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include "esp_http_server.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_vfs.h"
#include "cJSON.h"

#include "esp_gap_ble_api.h"

static const char *REST_TAG = "esp-rest";
#define REST_CHECK(a, str, goto_tag, ...)                                              \
    do                                                                                 \
    {                                                                                  \
        if (!(a))                                                                      \
        {                                                                              \
            ESP_LOGE(REST_TAG, "%s(%d): " str, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
            goto goto_tag;                                                             \
        }                                                                              \
    } while (0)

#define FILE_PATH_MAX (ESP_VFS_PATH_MAX + 128)
#define SCRATCH_BUFSIZE (10240)

typedef struct rest_server_context {
    char base_path[ESP_VFS_PATH_MAX + 1];
    char scratch[SCRATCH_BUFSIZE];
} rest_server_context_t;

#define CHECK_FILE_EXTENSION(filename, ext) (strcasecmp(&filename[strlen(filename) - strlen(ext)], ext) == 0)

/* Send HTTP response with the contents of the requested file */
static esp_err_t rest_common_get_handler(httpd_req_t *req)
{
    return ESP_OK;
}

/* Simple handler for getting system handler */
static esp_err_t system_info_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");
    cJSON *root = cJSON_CreateObject();
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    cJSON_AddStringToObject(root, "version", IDF_VER);
    cJSON_AddNumberToObject(root, "cores", chip_info.cores);
    const char *sys_info = cJSON_Print(root);
    httpd_resp_sendstr(req, sys_info);
    free((void *)sys_info);
    cJSON_Delete(root);
    return ESP_OK;
}

cJSON *json_resp;

/* Simple handler for getting BLE scan results*/
static esp_err_t ble_scan_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");
    json_resp = cJSON_CreateArray();
    // start scanning for near devices
    ESP_LOGI("rest-ble-scan", "Start scan");
    uint32_t duration = 5; // in seconds
    esp_ble_gap_start_scanning(duration);
    // cJSON_AddStringToObject(resp, "name", "ahoj");
    sleep(5);
    const char *scan = cJSON_Print(json_resp);
    httpd_resp_sendstr(req, scan);
    ESP_LOGI(REST_TAG,"resp: %s", scan);
    free((void *)scan);
    ESP_LOGI(REST_TAG,"ble/scan");
    cJSON_Delete(json_resp);
    return ESP_OK;
}

static esp_err_t list_device_get_handler(httpd_req_t *req){
    int bond_cnt = esp_ble_get_bond_device_num();
    esp_ble_bond_dev_t list[bond_cnt];
    ESP_LOGI(REST_TAG,"list length: %d", bond_cnt);
    esp_ble_get_bond_device_list(&bond_cnt, list);
    char address[20];
    uint8_t *a;
    cJSON *json_list = cJSON_CreateArray();
    cJSON *json_obj;
    for (int i = 0; i < bond_cnt; i++){
        a = list[i].bd_addr;
        sprintf(address, "%02x %02x %02x %02x %02x %02x", a[0], a[1], a[2], a[3], a[4], a[5]);
        json_obj = cJSON_CreateObject();
        cJSON_AddStringToObject(json_obj, "address", (const char*)address);
        cJSON_AddItemToArray(json_list, json_obj);
    }
    const char *scan = cJSON_Print(json_list);
    httpd_resp_sendstr(req, scan);
    ESP_LOGI(REST_TAG,"resp: %s", scan);
    free((void *)scan);
    cJSON_Delete(json_list);
    return ESP_OK;
}

void remove_spaces(char* s) {
    const char* d = s;
    do {
        while (*d == ' ') {
            ++d;
        }
        *s++ = *d++;
    } while (*d++);
}

static esp_err_t remove_device_post_handler(httpd_req_t *req){
    int total_len = req->content_len;
    int cur_len = 0;
    char *buf = ((rest_server_context_t *)(req->user_ctx))->scratch;
    int received = 0;
    if (total_len >= SCRATCH_BUFSIZE) {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }
    while (cur_len < total_len) {
        received = httpd_req_recv(req, buf + cur_len, total_len);
        if (received <= 0) {
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0';
    cJSON *root = cJSON_Parse(buf);
    char* address_str = cJSON_GetObjectItem(root, "address")->valuestring;
    // remove_spaces(address_str);
    esp_bd_addr_t address_hex;
    char str[3] = "\0\0\0";
    ESP_LOGI(REST_TAG, "address to be removed: %s", address_str);
    for (int i = 0; i < ESP_BD_ADDR_LEN; i++){
        str[0] = address_str[0];
        str[1] = address_str[1];
        address_hex[i] = (uint8_t)strtol(str, NULL, 16);
        address_str = address_str + 2;
        ESP_LOGI(REST_TAG, "address to be removed: %s hex: %x", str, address_hex[i]);
    }
    esp_ble_remove_bond_device(address_hex);
    return ESP_OK;
}

static esp_err_t add_device_post_handler(httpd_req_t *req)
{
    int total_len = req->content_len;
    int cur_len = 0;
    char *buf = ((rest_server_context_t *)(req->user_ctx))->scratch;
    int received = 0;
    if (total_len >= SCRATCH_BUFSIZE) {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }
    while (cur_len < total_len) {
        received = httpd_req_recv(req, buf + cur_len, total_len);
        if (received <= 0) {
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0';

    cJSON *root = cJSON_Parse(buf);
    // TODO hadle not presented object names
    char* name = cJSON_GetObjectItem(root, "name")->valuestring;
    char* characteristic = cJSON_GetObjectItem(root, "char")->valuestring;
    char* gatt = cJSON_GetObjectItem(root, "gatt")->valuestring;
    ESP_LOGI(REST_TAG, "Connecting to: name = %s, characteristic = %s, gatt = %s", name, characteristic, gatt);

    // TODO connect to device
    cJSON_Delete(root);
    httpd_resp_sendstr(req, "Post control value successfully");
    return ESP_OK;
}

esp_err_t start_rest_server(const char *base_path)
{
    REST_CHECK(base_path, "wrong base path", err);
    rest_server_context_t *rest_context = calloc(1, sizeof(rest_server_context_t));
    REST_CHECK(rest_context, "No memory for rest context", err);
    strlcpy(rest_context->base_path, base_path, sizeof(rest_context->base_path));

    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;

    ESP_LOGI(REST_TAG, "Starting HTTP Server");
    REST_CHECK(httpd_start(&server, &config) == ESP_OK, "Start server failed", err_start);

    /* URI handler for fetching system info */
    httpd_uri_t system_info_get_uri = {
        .uri = "/api/v1/system/info",
        .method = HTTP_GET,
        .handler = system_info_get_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &system_info_get_uri);

    /* URI handler for getting web server files */
    httpd_uri_t scan_get_uri = {
        .uri = "/ble/scan",
        .method = HTTP_GET,
        .handler = ble_scan_get_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &scan_get_uri);

    httpd_uri_t ble_add_device_post_uri = {
        .uri = "/ble/device/add",
        .method = HTTP_POST,
        .handler = add_device_post_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &ble_add_device_post_uri);

    httpd_uri_t ble_remove_device_post_uri = {
        .uri = "/ble/device/remove",
        .method = HTTP_POST,
        .handler = remove_device_post_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &ble_remove_device_post_uri);
    
    httpd_uri_t ble_list_device_get_uri = {
        .uri = "/ble/device/list",
        .method = HTTP_GET,
        .handler = list_device_get_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &ble_list_device_get_uri);

    /* URI handler for getting web server files */
    httpd_uri_t common_get_uri = {
        .uri = "/*",
        .method = HTTP_GET,
        .handler = rest_common_get_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &common_get_uri);

    return ESP_OK;
err_start:
    free(rest_context);
err:
    return ESP_FAIL;
}
