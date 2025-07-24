#ifndef I2C_CONFIG_H
#define I2C_CONFIG_H

#include "driver/i2c_master.h"
#include "driver/gpio.h"

// I2C Configuration
#define I2C_MASTER_SDA_IO       18      // GPIO pin for SDA (adjust for your board)
#define I2C_MASTER_SCL_IO       17      // GPIO pin for SCL (adjust for your board)
#define I2C_MASTER_FREQ_HZ      400000  // 400kHz
#define I2C_MASTER_TIMEOUT_MS   1000

// Global I2C bus and device handles
extern i2c_master_bus_handle_t i2c_bus_handle;
extern i2c_master_dev_handle_t max30102_dev_handle;

// Function prototypes
esp_err_t i2c_master_init(void);
esp_err_t i2c_master_deinit(void);

#endif // I2C_CONFIG_H