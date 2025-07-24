#include "max30102.h"
#include "i2c_config.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "MAX30102";

// Default configuration - much more conservative
const max30102_config_t MAX30102_DEFAULT_CONFIG = {
    .mode = MAX30102_MODE_SPO2,
    .sample_rate = MAX30102_SAMPLERATE_50,   
    .pulse_width = MAX30102_PULSEWIDTH_215,  // Shorter pulse width
    .adc_range = MAX30102_ADCRANGE_4096,
    .sample_avg = MAX30102_SAMPLEAVG_1,
    .led1_power = 0x1F,  // Reduced LED power
    .led2_power = 0x1F,  // Reduced LED power
    .rollover_enable = true,
    .almost_full_threshold = 10  // Lower threshold
};

// Private function prototypes
static max30102_err_t max30102_write_reg(uint8_t reg, uint8_t data);
static max30102_err_t max30102_read_reg(uint8_t reg, uint8_t *data);
static max30102_err_t max30102_read_fifo_data(uint8_t *data, size_t len);

static max30102_err_t max30102_write_reg(uint8_t reg, uint8_t data)
{
    if (!max30102_dev_handle) {
        ESP_LOGE(TAG, "MAX30102 device handle not initialized");
        return MAX30102_ERR_INIT;
    }
    
    uint8_t write_data[2] = {reg, data};
    
    esp_err_t ret = i2c_master_transmit(max30102_dev_handle, write_data, 2, 
                                      pdMS_TO_TICKS(MAX30102_I2C_TIMEOUT_MS));
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C write failed: %s", esp_err_to_name(ret));
        return MAX30102_ERR_I2C;
    }
    
    return MAX30102_OK;
}

static max30102_err_t max30102_read_reg(uint8_t reg, uint8_t *data)
{
    if (!data || !max30102_dev_handle) {
        return MAX30102_ERR_INVALID_PARAM;
    }
    
    esp_err_t ret = i2c_master_transmit_receive(max30102_dev_handle, &reg, 1, data, 1,
                                              pdMS_TO_TICKS(MAX30102_I2C_TIMEOUT_MS));
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C read failed: %s", esp_err_to_name(ret));
        return MAX30102_ERR_I2C;
    }
    
    return MAX30102_OK;
}

static max30102_err_t max30102_read_fifo_data(uint8_t *data, size_t len)
{
    if (!data || len == 0 || !max30102_dev_handle) {
        return MAX30102_ERR_INVALID_PARAM;
    }
    
    uint8_t reg = MAX30102_REG_FIFO_DATA;
    
    esp_err_t ret = i2c_master_transmit_receive(max30102_dev_handle, &reg, 1, data, len,
                                              pdMS_TO_TICKS(MAX30102_I2C_TIMEOUT_MS));
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "FIFO read failed: %s", esp_err_to_name(ret));
        return MAX30102_ERR_I2C;
    }
    
    return MAX30102_OK;
}

max30102_err_t max30102_init(const max30102_config_t *config)
{
    if (!config) {
        return MAX30102_ERR_INVALID_PARAM;
    }
    
    // Check if I2C device handle is available
    if (!max30102_dev_handle) {
        ESP_LOGE(TAG, "I2C device handle not initialized. Call i2c_master_init() first.");
        return MAX30102_ERR_INIT;
    }
    
    // Put sensor in shutdown mode first to stop any ongoing sampling
    max30102_err_t err = max30102_write_reg(MAX30102_REG_MODE_CONFIG, 0x80);
    if (err != MAX30102_OK) {
        ESP_LOGE(TAG, "Failed to put sensor in shutdown mode");
        return err;
    }
    
    vTaskDelay(pdMS_TO_TICKS(50));
    
    // Reset the sensor
    err = max30102_reset();
    if (err != MAX30102_OK) {
        return err;
    }
    
    // Wait for reset to complete
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Verify part ID
    uint8_t part_id;
    err = max30102_get_part_id(&part_id);
    if (err != MAX30102_OK) {
        return err;
    }
    
    if (part_id != 0x15) {  // Expected part ID for MAX30102
        ESP_LOGE(TAG, "Unexpected part ID: 0x%02X", part_id);
        return MAX30102_ERR_INIT;
    }
    
    ESP_LOGI(TAG, "MAX30102 detected, Part ID: 0x%02X", part_id);
    
    // Clear FIFO thoroughly
    err = max30102_clear_fifo();
    if (err != MAX30102_OK) return err;
    
    // Configure FIFO with more conservative settings
    uint8_t fifo_config = (config->sample_avg & 0xE0) | 
                         (config->rollover_enable ? MAX30102_ROLLOVER_EN : 0) |
                         (config->almost_full_threshold & MAX30102_A_FULL_MASK);
    err = max30102_write_reg(MAX30102_REG_FIFO_CONFIG, fifo_config);
    if (err != MAX30102_OK) return err;
    
    ESP_LOGI(TAG, "FIFO config written: 0x%02X", fifo_config);
    
    // Configure SpO2 settings  
    uint8_t spo2_config = (config->adc_range & 0x60) | 
                         (config->sample_rate & 0x1C) |
                         (config->pulse_width & 0x03);
    err = max30102_write_reg(MAX30102_REG_SPO2_CONFIG, spo2_config);
    if (err != MAX30102_OK) return err;
    
    ESP_LOGI(TAG, "SpO2 config written: 0x%02X", spo2_config);
    
    // Set LED power levels
    err = max30102_set_led_power(config->led1_power, config->led2_power);
    if (err != MAX30102_OK) return err;
    
    ESP_LOGI(TAG, "LED power set - Red: 0x%02X, IR: 0x%02X", config->led1_power, config->led2_power);
    
    // Clear FIFO again before starting
    err = max30102_clear_fifo();
    if (err != MAX30102_OK) return err;
    
    // Start the sensor in the specified mode (this should be LAST)
    err = max30102_write_reg(MAX30102_REG_MODE_CONFIG, config->mode);
    if (err != MAX30102_OK) return err;
    
    ESP_LOGI(TAG, "Mode config written: 0x%02X", config->mode);
    
    // Wait a moment for sensor to start
    vTaskDelay(pdMS_TO_TICKS(50));
    
    // Final FIFO clear to remove any startup samples
    err = max30102_clear_fifo();
    if (err != MAX30102_OK) return err;
    
    ESP_LOGI(TAG, "MAX30102 initialized successfully");
    return MAX30102_OK;
}

max30102_err_t max30102_deinit(void)
{
    // Put sensor in shutdown mode
    return max30102_write_reg(MAX30102_REG_MODE_CONFIG, 0x80);
}

max30102_err_t max30102_read_sample(max30102_sample_t *sample)
{
    if (!sample) {
        return MAX30102_ERR_INVALID_PARAM;
    }
    
    sample->valid = false;
    
    // Read FIFO pointers and overflow counter
    uint8_t write_ptr, read_ptr, overflow_counter;
    max30102_err_t err = max30102_read_reg(MAX30102_REG_FIFO_WR_PTR, &write_ptr);
    if (err != MAX30102_OK) return err;
    
    err = max30102_read_reg(MAX30102_REG_FIFO_RD_PTR, &read_ptr);
    if (err != MAX30102_OK) return err;
    
    err = max30102_read_reg(MAX30102_REG_OVF_COUNTER, &overflow_counter);
    if (err != MAX30102_OK) return err;
    
    // Debug log FIFO state more frequently when there are issues
    static uint32_t debug_counter = 0;
    debug_counter++;
    if (debug_counter % 10 == 0 || overflow_counter > 0) {
        ESP_LOGI(TAG, "FIFO Debug [%lu] - WR: %d, RD: %d, OVF: %d", 
                 debug_counter, write_ptr, read_ptr, overflow_counter);
    }
    
    // Handle overflow - this might indicate continuous overflow
    if (overflow_counter > 0) {
        ESP_LOGW(TAG, "FIFO OVERFLOW! Counter: %d - Clearing and stopping sensor", overflow_counter);
        
        // Stop sensor temporarily
        max30102_write_reg(MAX30102_REG_MODE_CONFIG, 0x80); // Shutdown
        vTaskDelay(pdMS_TO_TICKS(10));
        
        // Clear FIFO
        max30102_clear_fifo();
        vTaskDelay(pdMS_TO_TICKS(10));
        
        // Restart sensor
        max30102_write_reg(MAX30102_REG_MODE_CONFIG, MAX30102_MODE_SPO2);
        
        return MAX30102_ERR_NO_DATA;
    }
    
    // Calculate number of samples available (handle rollover)
    int8_t samples_available;
    if (write_ptr >= read_ptr) {
        samples_available = write_ptr - read_ptr;
    } else {
        samples_available = (32 - read_ptr) + write_ptr;  // FIFO is 32 deep
    }
    
    if (samples_available <= 0) {
        return MAX30102_ERR_NO_DATA;
    }
    
    // If too many samples, clear some to prevent overflow
    if (samples_available > 8) {
        ESP_LOGW(TAG, "FIFO getting full (%d samples). Clearing excess.", samples_available);
        max30102_clear_fifo();
        return MAX30102_ERR_NO_DATA;
    }
    
    // Read one sample (6 bytes: 3 bytes red + 3 bytes IR)
    uint8_t fifo_data[6];
    err = max30102_read_fifo_data(fifo_data, 6);
    if (err != MAX30102_OK) return err;
    
    // Convert to 18-bit values (only upper 18 bits are valid)
    sample->red = ((uint32_t)fifo_data[0] << 16) | ((uint32_t)fifo_data[1] << 8) | fifo_data[2];
    sample->red &= 0x03FFFF;  // Mask to 18 bits
    
    sample->ir = ((uint32_t)fifo_data[3] << 16) | ((uint32_t)fifo_data[4] << 8) | fifo_data[5];
    sample->ir &= 0x03FFFF;   // Mask to 18 bits
    
    sample->valid = true;
    return MAX30102_OK;
}

max30102_err_t max30102_reset(void)
{
    return max30102_write_reg(MAX30102_REG_MODE_CONFIG, 0x40);
}

max30102_err_t max30102_get_part_id(uint8_t *part_id)
{
    return max30102_read_reg(MAX30102_REG_PART_ID, part_id);
}

max30102_err_t max30102_clear_fifo(void)
{
    max30102_err_t err = max30102_write_reg(MAX30102_REG_FIFO_WR_PTR, 0x00);
    if (err != MAX30102_OK) return err;
    
    err = max30102_write_reg(MAX30102_REG_OVF_COUNTER, 0x00);
    if (err != MAX30102_OK) return err;
    
    return max30102_write_reg(MAX30102_REG_FIFO_RD_PTR, 0x00);
}

max30102_err_t max30102_set_led_power(uint8_t led1_power, uint8_t led2_power)
{
    max30102_err_t err = max30102_write_reg(MAX30102_REG_LED1_PA, led1_power);
    if (err != MAX30102_OK) return err;
    
    return max30102_write_reg(MAX30102_REG_LED2_PA, led2_power);
}
