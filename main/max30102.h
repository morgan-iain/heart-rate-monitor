#ifndef MAX30102_H
#define MAX30102_H

#include <stdint.h>
#include <stdbool.h>
#include "driver/i2c_master.h"

// MAX30102 I2C Configuration
#define MAX30102_I2C_ADDR           0x57
#define MAX30102_I2C_TIMEOUT_MS     1000

// MAX30102 Register Addresses
#define MAX30102_REG_INTR_STATUS_1  0x00
#define MAX30102_REG_INTR_STATUS_2  0x01
#define MAX30102_REG_INTR_ENABLE_1  0x02
#define MAX30102_REG_INTR_ENABLE_2  0x03
#define MAX30102_REG_FIFO_WR_PTR    0x04
#define MAX30102_REG_OVF_COUNTER    0x05
#define MAX30102_REG_FIFO_RD_PTR    0x06
#define MAX30102_REG_FIFO_DATA      0x07
#define MAX30102_REG_FIFO_CONFIG    0x08
#define MAX30102_REG_MODE_CONFIG    0x09
#define MAX30102_REG_SPO2_CONFIG    0x0A
#define MAX30102_REG_LED1_PA        0x0C
#define MAX30102_REG_LED2_PA        0x0D
#define MAX30102_REG_PILOT_PA       0x10
#define MAX30102_REG_MULTI_LED_CTRL1 0x11
#define MAX30102_REG_MULTI_LED_CTRL2 0x12
#define MAX30102_REG_TEMP_INTR      0x1F
#define MAX30102_REG_TEMP_FRAC      0x20
#define MAX30102_REG_TEMP_CONFIG    0x21
#define MAX30102_REG_PROX_INT_THRESH 0x30
#define MAX30102_REG_REV_ID         0xFE
#define MAX30102_REG_PART_ID        0xFF

// Configuration values
#define MAX30102_MODE_HR_ONLY       0x02
#define MAX30102_MODE_SPO2          0x03
#define MAX30102_SAMPLEAVG_1        0x00
#define MAX30102_SAMPLEAVG_2        0x20
#define MAX30102_SAMPLEAVG_4        0x40
#define MAX30102_SAMPLEAVG_8        0x60
#define MAX30102_SAMPLEAVG_16       0x80
#define MAX30102_SAMPLEAVG_32       0xA0

#define MAX30102_ROLLOVER_EN        0x10
#define MAX30102_A_FULL_MASK        0x0F

// Sample rates
#define MAX30102_SAMPLERATE_50      0x00
#define MAX30102_SAMPLERATE_100     0x04
#define MAX30102_SAMPLERATE_200     0x08
#define MAX30102_SAMPLERATE_400     0x0C
#define MAX30102_SAMPLERATE_800     0x10
#define MAX30102_SAMPLERATE_1000    0x14
#define MAX30102_SAMPLERATE_1600    0x18
#define MAX30102_SAMPLERATE_3200    0x1C

// Pulse width
#define MAX30102_PULSEWIDTH_69      0x00
#define MAX30102_PULSEWIDTH_118     0x01
#define MAX30102_PULSEWIDTH_215     0x02
#define MAX30102_PULSEWIDTH_411     0x03

// ADC Range
#define MAX30102_ADCRANGE_2048      0x00
#define MAX30102_ADCRANGE_4096      0x20
#define MAX30102_ADCRANGE_8192      0x40
#define MAX30102_ADCRANGE_16384     0x60

// Error codes
typedef enum {
    MAX30102_OK = 0,
    MAX30102_ERR_INIT,
    MAX30102_ERR_I2C,
    MAX30102_ERR_TIMEOUT,
    MAX30102_ERR_INVALID_PARAM,
    MAX30102_ERR_NO_DATA
} max30102_err_t;

// Configuration structure
typedef struct {
    uint8_t mode;
    uint8_t sample_rate;
    uint8_t pulse_width;
    uint8_t adc_range;
    uint8_t sample_avg;
    uint8_t led1_power;
    uint8_t led2_power;
    bool rollover_enable;
    uint8_t almost_full_threshold;
} max30102_config_t;

// Sample data structure
typedef struct {
    uint32_t red;
    uint32_t ir;
    bool valid;
} max30102_sample_t;

// Function prototypes
max30102_err_t max30102_init(const max30102_config_t *config);
max30102_err_t max30102_deinit(void);
max30102_err_t max30102_read_sample(max30102_sample_t *sample);
max30102_err_t max30102_reset(void);
max30102_err_t max30102_get_part_id(uint8_t *part_id);
max30102_err_t max30102_clear_fifo(void);
max30102_err_t max30102_set_led_power(uint8_t led1_power, uint8_t led2_power);

// Default configuration
extern const max30102_config_t MAX30102_DEFAULT_CONFIG;

#endif // MAX30102_H
