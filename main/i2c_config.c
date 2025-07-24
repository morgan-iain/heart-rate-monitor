#include "sdkconfig.h"
#include "i2c_config.h"
#include "esp_log.h"

static const char *TAG = "I2C_CONFIG";

// Global handles
i2c_master_bus_handle_t i2c_bus_handle = NULL;
i2c_master_dev_handle_t max30102_dev_handle = NULL;

esp_err_t i2c_master_init(void)
{
    // Configure I2C bus
    i2c_master_bus_config_t bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    
    esp_err_t err = i2c_new_master_bus(&bus_config, &i2c_bus_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C bus initialization failed: %s", esp_err_to_name(err));
        return err;
    }
    
    // Configure MAX30102 device
    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = 0x57,  // MAX30102 I2C address
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };
    
    err = i2c_master_bus_add_device(i2c_bus_handle, &dev_config, &max30102_dev_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "I2C device add failed: %s", esp_err_to_name(err));
        i2c_del_master_bus(i2c_bus_handle);
        i2c_bus_handle = NULL;
        return err;
    }
    
    ESP_LOGI(TAG, "I2C master initialized - SDA: %d, SCL: %d, Freq: %d Hz", 
             I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO, I2C_MASTER_FREQ_HZ);
    
    return ESP_OK;
}

esp_err_t i2c_master_deinit(void)
{
    esp_err_t err = ESP_OK;
    
    if (max30102_dev_handle) {
        err = i2c_master_bus_rm_device(max30102_dev_handle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "I2C device remove failed: %s", esp_err_to_name(err));
        }
        max30102_dev_handle = NULL;
    }
    
    if (i2c_bus_handle) {
        err = i2c_del_master_bus(i2c_bus_handle);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "I2C bus delete failed: %s", esp_err_to_name(err));
        }
        i2c_bus_handle = NULL;
    }
    
    ESP_LOGI(TAG, "I2C master deinitialized");
    return err;
}
