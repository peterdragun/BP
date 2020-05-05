#include "sensors.h"

uint8_t unknown_sensor[6];
sensor_t sensors[MAX_NUMBER_OF_SENSORS];
uint8_t number_of_sensors = 0;
TaskHandle_t xHandle_increment_beeps;
const char sensors_nvs_key[5][3] = {"s1\0", "s2\0", "s3\0", "s4\0", "s5\0" };

esp_err_t compare_uuids(uint8_t *uuid1, uint8_t *uuid2){
    if(sizeof(uuid1) != sizeof(uuid2)){
        return ESP_FAIL;
    }
    for (int i = 0; i < sizeof(uuid1); i++){
        if(uuid1[i] != uuid2[i]){
            return ESP_FAIL;
        }
    }
    return ESP_OK;
}

int find_idx(uint8_t *address){
    for (int i = 0; i < number_of_sensors; i++){
        if(compare_uuids(sensors[i].address, address) == ESP_OK){
            return i;
        }
    }
    return -1;
}

esp_err_t record_sensor(uint8_t *address){
    int idx = find_idx(address);
    if (idx < 0){
        for (int i = 0; i < 6; i++){
            unknown_sensor[i] = address[i];
        }
        return ESP_FAIL;
    }
    sensors[idx].missed_beeps = 0;
    return ESP_OK;
}

esp_err_t add_new_sensor(uint8_t *address){
    if (number_of_sensors == MAX_NUMBER_OF_SENSORS){
        return ESP_FAIL;
    }
    if (find_idx(address) != -1){
        return ESP_FAIL;
    }
    nvs_handle_t nvs_handle;
    uint64_t number = 0;
    esp_err_t ret = nvs_open("storage", NVS_READWRITE, &nvs_handle);
    if (ret == ESP_OK) {
        ESP_LOGI(SENSORS_TAG, "Writing sensor address to NVS memory... ");
        for (uint8_t i = 0; i < 6; i++){
            printf("%x\n", address[i]);
            number |= address[i];
            number = number << 8;
            sensors[number_of_sensors].address[i] = address[i];
            unknown_sensor[i] = 0;
        }
        ret = nvs_set_u64(nvs_handle, sensors_nvs_key[number_of_sensors], number);
        if(ret == ESP_OK){
            nvs_set_u8(nvs_handle, "sensors_cnt", number_of_sensors+1);
            nvs_commit(nvs_handle);
        }
        nvs_close(nvs_handle);
        ESP_LOGI(SENSORS_TAG, "Done");
    }
    sensors[number_of_sensors].missed_beeps = 0;
    sensors[number_of_sensors].last_alarm = 0;
    number_of_sensors++;
    return ESP_OK;
}

esp_err_t remove_sensor(uint8_t *address){
    int idx = find_idx(address);
    if(idx < 0){
        return ESP_FAIL;
    }
    nvs_handle_t nvs_handle;
    uint64_t number = 0;
    esp_err_t ret = nvs_open("storage", NVS_READWRITE, &nvs_handle);
    if (ret == ESP_OK) {
        if (idx != number_of_sensors-1){
            for (uint8_t i = 0; i < 6; i++){
                printf("%x\n", address[i]);
                number |= address[i];
                number = number << 8;
                sensors[number_of_sensors].address[i] = address[i];
            }
            ret = nvs_set_u64(nvs_handle, sensors_nvs_key[idx], number);
        }
        nvs_set_u8(nvs_handle, "sensors_cnt", --number_of_sensors);
        nvs_erase_key(nvs_handle, sensors_nvs_key[number_of_sensors]);
        nvs_commit(nvs_handle);
        nvs_close(nvs_handle);
    }
    esp_ble_remove_bond_device(address);
    return ESP_OK;
}

esp_err_t record_alarm(uint8_t *address){
    int idx = find_idx(address);
    if(idx < 0){
        return ESP_FAIL;
    }
    time(&last_alarm);
    sensors[idx].last_alarm = last_alarm;
    return ESP_OK;
}

int not_responding(){
    int cnt = 0;
    for(int i = 0; i < number_of_sensors; i++){
        if (sensors[i].missed_beeps > 3){
            cnt++;
        }
    }
    return cnt;
}

void increment_sensor_beeps_task(){
    uint8_t wait_time;
    while (1) {
        ESP_LOGI("incrementing task", "start");
        for(uint8_t i = 0; i < number_of_sensors; i++){
            sensors[i].missed_beeps++;
            if (sensors[i].missed_beeps++ >= 4){
                //BAD - alarm or smth
                ESP_LOGE("incrementing task", "senozor sa nehlasi!!!!!");
            }
        }
        wait_time = 40;
        if(*security_state > Activating){
            wait_time = 20;
        }
        vTaskDelay(wait_time*1000 / portTICK_PERIOD_MS);
    }
}
