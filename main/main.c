#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "max30102.h"
#include "i2c_config.h"

static const char *TAG = "MAIN";

#define SAMPLE_INTERVAL_MS  25   // Even faster reading - 40Hz
#define STACK_SIZE          4096
#define TASK_PRIORITY       5

// Task handle for sensor reading
static TaskHandle_t sensor_task_handle = NULL;

static void sensor_task(void *pvParameters)
{
    max30102_sample_t sample;
    TickType_t last_wake_time = xTaskGetTickCount();
    uint32_t sample_count = 0;
    uint32_t no_data_count = 0;
    
    ESP_LOGI(TAG, "Starting sensor reading task...");
    ESP_LOGI(TAG, "Waiting for sensor to stabilize...");
    
    // Give sensor extra time to stabilize after initialization
    vTaskDelay(pdMS_TO_TICKS(200));
    
    ESP_LOGI(TAG, "Sample format: [COUNT] Red: XXXXXX, IR: XXXXXX");
    ESP_LOGI(TAG, "----------------------------------------");
    
    while (1) {
        max30102_err_t err = max30102_read_sample(&sample);
        
        if (err == MAX30102_OK && sample.valid) {
            sample_count++;
            no_data_count = 0;  // Reset no-data counter
            printf("[%lu] Red: %6lu, IR: %6lu\n", 
                   sample_count, sample.red, sample.ir);
        } else if (err == MAX30102_ERR_NO_DATA) {
            // No data available, this is normal but track it
            no_data_count++;
            if (no_data_count % 40 == 1) {  // Print every 40 no-data events (less frequent)
                printf("[%lu] No data available (count: %lu)\n", sample_count, no_data_count);
            }
        } else {
            ESP_LOGW(TAG, "Sample read error: %d", err);
            // Try to recover by clearing FIFO
            max30102_clear_fifo();
            vTaskDelay(pdMS_TO_TICKS(100));  // Brief pause for recovery
        }
        
        // Check if we've been getting no data for too long
        if (no_data_count > 100) {
            ESP_LOGW(TAG, "No data for %lu attempts. Resetting sensor...", no_data_count);
            max30102_reset();
            vTaskDelay(pdMS_TO_TICKS(100));
            max30102_init(&MAX30102_DEFAULT_CONFIG);
            no_data_count = 0;
        }
        
        // Wait for next sample interval
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(SAMPLE_INTERVAL_MS));
    }
}

static esp_err_t init_system(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Initialize I2C
    ESP_LOGI(TAG, "Initializing I2C...");
    ret = i2c_master_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C initialization failed");
        return ret;
    }
    
    // Initialize MAX30102 sensor
    ESP_LOGI(TAG, "Initializing MAX30102 sensor...");
    max30102_err_t max_err = max30102_init(&MAX30102_DEFAULT_CONFIG);
    if (max_err != MAX30102_OK) {
        ESP_LOGE(TAG, "MAX30102 initialization failed: %d", max_err);
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "System initialization complete");
    return ESP_OK;
}

static void cleanup_system(void)
{
    ESP_LOGI(TAG, "Cleaning up system resources...");
    
    // Stop sensor task if running
    if (sensor_task_handle) {
        vTaskDelete(sensor_task_handle);
        sensor_task_handle = NULL;
    }
    
    // Deinitialize MAX30102
    max30102_deinit();
    
    // Deinitialize I2C
    i2c_master_deinit();
    
    ESP_LOGI(TAG, "System cleanup complete");
}

void app_main(void)
{
    esp_log_level_set("*", ESP_LOG_INFO);
    
    ESP_LOGI(TAG, "=== MAX30102 Sensor Reader ===");
    ESP_LOGI(TAG, "ESP32-S3 with MAX30102 Heart Rate & SpO2 Sensor");
    ESP_LOGI(TAG, "Sample Rate: %d ms", SAMPLE_INTERVAL_MS);
    
    // Initialize system components
    if (init_system() != ESP_OK) {
        ESP_LOGE(TAG, "System initialization failed");
        return;
    }
    
    // Create sensor reading task
    BaseType_t task_result = xTaskCreate(
        sensor_task,
        "sensor_task",
        STACK_SIZE,
        NULL,
        TASK_PRIORITY,
        &sensor_task_handle
    );
    
    if (task_result != pdPASS) {
        ESP_LOGE(TAG, "Failed to create sensor task");
        cleanup_system();
        return;
    }
    
    ESP_LOGI(TAG, "Application started successfully");
    
    // Main loop - could add additional functionality here
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(5000));  // Print status every 5 seconds
        ESP_LOGI(TAG, "System running... Free heap: %lu bytes", 
                 esp_get_free_heap_size());
    }
}
