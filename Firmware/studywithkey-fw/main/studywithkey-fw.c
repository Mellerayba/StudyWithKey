#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"

#define BUTTON_GPIO GPIO_NUM_4
static const char *TAG = "swk";

void app_main(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << BUTTON_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    int output = 1;
    int64_t last = 0;
    ESP_LOGI(TAG, "Stage 2: watching GPIO%d, with debouncing", BUTTON_GPIO);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10));
        int level = gpio_get_level(BUTTON_GPIO);
        if (level == output) {
            continue;
        }
        if (esp_timer_get_time()-last < 30000){
           continue;
        }
        output = level;
        if(output == 0){
            ESP_LOGI(TAG, "button pressed");
        }
        last = esp_timer_get_time();
    }
}