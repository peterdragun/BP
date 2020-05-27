#include "alarm.h"

static state_enum_t security = Disarmed;
state_enum_t *security_state = &security; /*!< Current state of security system */
time_t last_alarm = 0; /*!< Timestamp of last alarm */

static int keypad_col = 0;

char entered_code[19] = {0}; /*!< Buffer for entered code*/
char expected_code[19] = "123456"; /*!< Expected code. Set to default code. */
static int wrong_attempts = 0;
int *wrong_attempts_ptr = &wrong_attempts; /*!< Number of wrong codes */

int not_responding(); // defined in sensors.h

/*! LEDC channels */
ledc_channel_config_t ledc_channel[LEDC_CH_NUM] = {
    {
        .channel    = LEDC_CHANNEL_0,
        .duty       = 0,
        .gpio_num   = LEDC_HS_CH0_BUZZER,
        .speed_mode = LEDC_HS_MODE,
        .hpoint     = 0,
        .timer_sel  = LEDC_TIMER_0
    },
    {
        .channel    = LEDC_CHANNEL_1,
        .duty       = 0,
        .gpio_num   = LEDC_HS_CH1_LED_R,
        .speed_mode = LEDC_HS_MODE,
        .hpoint     = 0,
        .timer_sel  = LEDC_TIMER_0
    },
    {
        .channel    = LEDC_CHANNEL_2,
        .duty       = 0,
        .gpio_num   = LEDC_HS_CH2_LED_Y,
        .speed_mode = LEDC_HS_MODE,
        .hpoint     = 0,
        .timer_sel  = LEDC_TIMER_0
    },
};

TaskHandle_t xHandle_alarm; /*!< Task handle for alarm */
TaskHandle_t xHandle_activate; /*!< Task handle for activating */
TaskHandle_t xHandle_search; /*!< Task handle for searching near devices */

char *state_to_str(){
    char *state;
    switch (*security_state){
    case Setup:
        state = "Setup";
        break;
    case Disarmed:
        state = "Disarmed";
        break;
    case Activating:
        state = "Activating";
        break;
    case Armed:
        state = "Armed";
        break;
    case Alarm:
        state = "Alarm";
        break;
    default:
        state = "Unknown";
        break;
    }
    return state;
}

void activate_security(){
    int ch;
    // beep every second
    for (size_t sec = 0; sec < 30; sec++){
        ESP_LOGI(SECURITY_SYSTEM, "System will be activated in: %d seconds.", 30 - sec);
        for (ch = 0; ch < LEDC_CH_NUM; ch+=2) {
            ledc_set_duty(ledc_channel[ch].speed_mode, ledc_channel[ch].channel, LEDC_DUTY);
            ledc_update_duty(ledc_channel[ch].speed_mode, ledc_channel[ch].channel);
        }
        vTaskDelay(200 / portTICK_PERIOD_MS);

        for (ch = 0; ch < LEDC_CH_NUM; ch+=2) {
            ledc_set_duty(ledc_channel[ch].speed_mode, ledc_channel[ch].channel, 0);
            ledc_update_duty(ledc_channel[ch].speed_mode, ledc_channel[ch].channel);
        }
        vTaskDelay(100);
    }

    // 3 quick beeps
    for (size_t i = 0; i < 3; i++){
        for (ch = 0; ch < LEDC_CH_NUM; ch+=2) {
            ledc_set_duty(ledc_channel[ch].speed_mode, ledc_channel[ch].channel, LEDC_DUTY);
            ledc_update_duty(ledc_channel[ch].speed_mode, ledc_channel[ch].channel);
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);

        for (ch = 0; ch < LEDC_CH_NUM; ch+=2) {
            ledc_set_duty(ledc_channel[ch].speed_mode, ledc_channel[ch].channel, 0);
            ledc_update_duty(ledc_channel[ch].speed_mode, ledc_channel[ch].channel);
        }
        vTaskDelay(50);
    }

    ESP_LOGI(SECURITY_SYSTEM, "Armed");
    *security_state = Armed;
    xTaskCreate(search_devices_task, "search_devices", 1024*2, NULL, configMAX_PRIORITIES-1, &xHandle_search);
    vTaskDelete(NULL);
}

void alarm_task(){
    int ch;
    while (1) {
        for (ch = 0; ch < LEDC_CH_NUM - 1; ch++) {
            ledc_set_duty(ledc_channel[ch].speed_mode, ledc_channel[ch].channel, LEDC_DUTY);
            ledc_update_duty(ledc_channel[ch].speed_mode, ledc_channel[ch].channel);
        }
        vTaskDelay(TASK_WAIT / portTICK_PERIOD_MS);

        for (ch = 0; ch < LEDC_CH_NUM - 1; ch++) {
            ledc_set_duty(ledc_channel[ch].speed_mode, ledc_channel[ch].channel, 0);
            ledc_update_duty(ledc_channel[ch].speed_mode, ledc_channel[ch].channel);
        }
        vTaskDelay(TASK_WAIT / portTICK_PERIOD_MS);
    }
}

void stop_alarm_task(){
    // 3 quick beeps
    int ch;
    for (size_t i = 0; i < 3; i++){
        for (ch = 0; ch < LEDC_CH_NUM; ch+=2) {
            ledc_set_duty(ledc_channel[ch].speed_mode, ledc_channel[ch].channel, LEDC_DUTY);
            ledc_update_duty(ledc_channel[ch].speed_mode, ledc_channel[ch].channel);
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);

        for (ch = 0; ch < LEDC_CH_NUM; ch+=2) {
            ledc_set_duty(ledc_channel[ch].speed_mode, ledc_channel[ch].channel, 0);
            ledc_update_duty(ledc_channel[ch].speed_mode, ledc_channel[ch].channel);
        }
        vTaskDelay(50);
    }
    vTaskDelete(NULL);
}

void long_beep(){
    int ch;
    for (ch = 0; ch < LEDC_CH_NUM; ch+=2) {
        ledc_set_duty(ledc_channel[ch].speed_mode, ledc_channel[ch].channel, LEDC_DUTY);
        ledc_update_duty(ledc_channel[ch].speed_mode, ledc_channel[ch].channel);
    }
    vTaskDelay(500 / portTICK_PERIOD_MS);

    for (ch = 0; ch < LEDC_CH_NUM; ch+=2) {
        ledc_set_duty(ledc_channel[ch].speed_mode, ledc_channel[ch].channel, 0);
        ledc_update_duty(ledc_channel[ch].speed_mode, ledc_channel[ch].channel);
    }
    vTaskDelete(NULL);
}

void search_devices_task(){
    uint32_t duration = 5; // in seconds
    *scan_type_ptr = Search_known;
    while (1){
        esp_ble_gap_start_scanning(duration);
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}

void add_char_to_code(char c){
    int entered_code_len = strlen(entered_code);
    if (entered_code_len >= 18){
        memset(entered_code, 0, sizeof(entered_code));
        entered_code[0] = c;
    }else{
        entered_code[entered_code_len] = c;
    }
}

esp_err_t arm_system(){
    ESP_LOGI(SECURITY_SYSTEM, "System activating");
    if (not_responding() != 0){
        ESP_LOGW(SECURITY_SYSTEM, "Some of sensors are not responding! System cannot be activated");
        ledc_set_duty(ledc_channel[1].speed_mode, ledc_channel[1].channel, LEDC_DUTY);
        ledc_update_duty(ledc_channel[1].speed_mode, ledc_channel[1].channel);
        xTaskCreate(long_beep, "long_beep", 1024*2, NULL, configMAX_PRIORITIES-1, NULL);
        return ESP_FAIL;
    }
    if (*security_state != Disarmed && *security_state != Setup){
        ESP_LOGW(SECURITY_SYSTEM, "Security system is already activated!");
        return ESP_FAIL;
    }
    *security_state = Activating;
    xTaskCreate(activate_security, "activate_security", 1024*2, NULL, configMAX_PRIORITIES-1, &xHandle_activate);
    return ESP_OK;
}

void compare_codes(){
    if ((entered_code[0] == '*') && (strlen(entered_code) == 1)){
        if(*security_state == Disarmed){
            arm_system();
        }else if(*security_state == Setup){
            *security_state = Disarmed;
            ESP_LOGI(SECURITY_SYSTEM, "System disarmed");
            xTaskCreate(long_beep, "long_beep", 1024*2, NULL, configMAX_PRIORITIES-1, NULL);
        }
    }else if (!strcmp(expected_code, entered_code)){
        if(*security_state == Disarmed){
            *security_state = Setup;
            ESP_LOGI(SECURITY_SYSTEM, "System in setup");
            xTaskCreate(long_beep, "long_beep", 1024*2, NULL, configMAX_PRIORITIES-1, NULL);
        }else if (*security_state != Setup){
            if(*security_state == Activating){
                vTaskDelete(xHandle_activate);
                xTaskCreate(stop_alarm_task, "stop_alarm", 1024*2, NULL, configMAX_PRIORITIES-1, NULL);
            }else if(*security_state == Alarm){
                vTaskDelete(xHandle_alarm);
            }else if(*security_state != Armed){
                xTaskCreate(stop_alarm_task, "stop_alarm", 1024*2, NULL, configMAX_PRIORITIES-1, NULL);
                vTaskDelete(xHandle_search);
                ESP_LOGW(SECURITY_SYSTEM, "Security system is not activated!");
                memset(entered_code, 0, sizeof(entered_code));
                return;
            }
            ESP_LOGI(SECURITY_SYSTEM, "System disarmed");
            *wrong_attempts_ptr = 0;
            // make sure buzzer stopped beeping
            for (int ch = 0; ch < LEDC_CH_NUM; ch++) {
                ledc_set_duty(ledc_channel[ch].speed_mode, ledc_channel[ch].channel, 0);
                ledc_update_duty(ledc_channel[ch].speed_mode, ledc_channel[ch].channel);
            }
            *security_state = Disarmed;
        }
    }else{
        ESP_LOGW(SECURITY_SYSTEM, "Wrong code!!!");
        if(*security_state == Armed){
            (*wrong_attempts_ptr)++;
            if (*wrong_attempts_ptr >= 3){
                ESP_LOGI(SECURITY_SYSTEM, "Alarm");
                *security_state = Alarm;
                *wrong_attempts_ptr = 0;
                vTaskDelete(xHandle_search);
                xTaskCreate(alarm_task, "alarm_task", 1024*2, NULL, configMAX_PRIORITIES-1, &xHandle_alarm);
            }
        }
    }
    memset(entered_code, 0, sizeof(entered_code));
}

void hadle_keypad_task(void *arg)
{
    int row[4];
    char c = '\0';
    while(1){
        row[0] = gpio_get_level(GPIO_ROW_0);
        row[1] = gpio_get_level(GPIO_ROW_1);
        row[2] = gpio_get_level(GPIO_ROW_2);
        row[3] = gpio_get_level(GPIO_ROW_3);
        switch (keypad_col){
        case 0:
            if (row[0] == 1){
                c = '*';
            }else if (row[1] == 1){
                c = '7';
            }else if (row[2] == 1){
                c = '4';
            }else if (row[3] == 1){
                c = '1';
            }
            gpio_set_level(GPIO_COL_0, 1);
            gpio_set_level(GPIO_COL_1, 0);
            gpio_set_level(GPIO_COL_2, 0);
            keypad_col++;
            break;
        case 1:
            if (row[0] == 1){
                c = '0';
            }else if (row[1] == 1){
                c = '8';
            }else if (row[2] == 1){
                c = '5';
            }else if (row[3] == 1){
                c = '2';
            }
            gpio_set_level(GPIO_COL_0, 0);
            gpio_set_level(GPIO_COL_1, 1);
            gpio_set_level(GPIO_COL_2, 0);
            keypad_col++;
            break;
        case 2:
            if (row[0] == 1){
                compare_codes();
            }else if (row[1] == 1){
                c = '9';
            }else if (row[2] == 1){
                c = '6';
            }else if (row[3] == 1){
                c = '3';
            }
            gpio_set_level(GPIO_COL_0, 0);
            gpio_set_level(GPIO_COL_1, 0);
            gpio_set_level(GPIO_COL_2, 1);
            keypad_col = 0;
            break;
        }
        if (c != '\0'){
            ets_printf("%c", c);
            add_char_to_code(c);
            c = 0;
        }
        vTaskDelay(pdMS_TO_TICKS(500/portTICK_PERIOD_MS));
    }
}

