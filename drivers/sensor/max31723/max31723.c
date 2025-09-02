/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_max31723

// Zephyr Sensor & Device Model includes
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/spi.h>

// Zephyr utility includes
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>

// Device header file
#include "max31723.h"
#include <math.h>

// TODO: Add RTIO
// TODO: Implement trigger ISRs based on thermostat output

LOG_MODULE_REGISTER(MAX31723, CONFIG_SENSOR_LOG_LEVEL);

struct max31723_data {
    uint16_t temperature_raw;
};

struct max31723_dev_config {
    struct spi_dt_spec bus;
    max31723_resolution_e resolution;
    max31723_thermostat_mode_e thermostat_mode;
    bool oneshot_mode;
};

static inline bool max31723_bus_is_ready(const struct device *dev)
{
    const struct max31723_dev_config *config = dev->config;

    return spi_is_ready_dt(&config->bus);
}

static int max31723_reg_write(const struct device *dev, 
                        uint8_t reg, 
                        uint8_t data) 
{
    int ret = 0;    
    const struct max31723_dev_config *config = dev->config;
    struct spi_dt_spec *spi = &config->bus;
    
    // setup cmd for register write
    uint8_t cmd[] = { reg | 0x80, data };

    // setup buffer structs for Zephyr SPI API
    const struct spi_buf tx_buf = {
        .buf = cmd,
        .len = sizeof(cmd)
    };
    const struct spi_buf_set tx = {
        .buffers = &tx_buf,
        .count = 1
    };

    // write register value
    ret = spi_write_dt(&spi, &tx);
    if (ret) {
        LOG_DBG("spi_write FAIL %d\n", ret);
    }

    return ret;
}

static int max31723_reg_read(const struct device *dev, 
                        uint8_t reg, 
                        uint8_t *data) 
{
    int ret = 0;
    const struct max31723_dev_config *config = dev->config;
    struct spi_dt_spec *spi = &config->bus;
    
    // setup tx for writing the read address
    const struct spi_buf tx_buf = {
        .buf = reg & 0x07,
        .len = 1
    };
    const struct spi_buf_set tx = {
        .buffers = &tx_buf,
        .count = 1
    };

    // setup data for rx buffer
    struct spi_buf rx_buf[2];
    rx_buf[0].buf = NULL;
    rx_buf[0].len = 1;
    rx_buf[1].buf = data;
    rx_buf[1].len = 1;

    const struct spi_buf_set rx = {
        .buffers = &rx_buf,
        .count = ARRAY_SIZE(rx_buf)
    };

    // read register value
    ret = spi_transceive_dt(spi, &tx, &rx);
    if (ret) {
        LOG_DBG("spi_read FAIL %d\n", ret);
    }

    return ret;
}


static int max31723_init(const struct device *dev)
{
    const struct max31723_dev_config *config = dev->config;

    int cfg_data, ret = 0;

    if ( !(max31723_bus_is_ready(dev)) ) {
        LOG_ERR("Could not initialize MAX31723");
        return -1;
    }

    cfg_data |= config->resolution; // set the resolution bits
    cfg_data |= config->thermostat_mode;

    max31723_reg_write(dev, 0x00, cfg_data);

    return 0;
}

static int max31723_sample_fetch(const struct device *dev,
                      enum sensor_channel chan)
{
    const struct max31723_dev_config *config = dev->config;
    struct max31723_data *data = dev->data;
    
    uint8_t temp_data;
    int ret;

    __ASSERT_NO_MSG( chan == SENSOR_CHAN_ALL );

    // get temperature data 
    ret = max31723_reg_read(dev, 0x01, temp_data);
    if (ret) 
    {
        return -1;
    }
    data->temperature_raw = temp_data;
    
    ret = max31723_reg_read(dev, 0x01, temp_data);
    data->temperature_raw |= (temp_data << 8);
    if (ret)
    {
        return -1;
    }

    return 0;
}

static int max31723_channel_get(const struct device *dev,
                     enum sensor_channel chan,
                     struct sensor_value *val)
{
    struct max31723_data *data = dev->data;
    uint16_t temperature_raw = data->temperature_raw;

    if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
        return -ENOTSUP;
    }

    // convert raw data to real temperature
    // first byte is integral type,
    // second byte is 8 fractional bits, with only the upper 4 valid
    if (temperature_raw < 0x8000u)
    {
        // val1 is int part
        // val2 is the fract part * 1000000
        val->val1 = temperature_raw >> 8;
        val->val2 = ((temperature_raw & 0xF0) * 1000U * 1000U) >> 8;
    }
    else {
        // val1 is int part (negative)
        // val2 is the fract part * 1000000 (negative)
        val->val1 = (temperature_raw >> 8) * -1;
        val->val2 = ((temperature_raw & 0xF0) * 1000U * 1000U) >> 8;
        val->val2 *= -1;
    }

    return 0;
}

static DEVICE_API(sensor, max31723_api) = {
    .sample_fetch = &max31723_sample_fetch,
    .channel_get = &max31723_channel_get,
};

/* Initializes a struct max31723_config for an instance on a SPI bus. */
#define MAX31723_CONFIG_SPI(inst) \
    {                                        \
        .bus = SPI_DT_SPEC_INST_GET(inst,   \
                    SPI_WORD_SET(8) |       \
                    SPI_TRANSFER_MSB |      \
                    SPI_MODE_CPOL |         \
                    SPI_MODE_CPHA,          \
                    0),                     \
        .resolution = MAX31723_RESOLUTION(DT_INST_PROP(inst, resolution)),         \
        .thermostat_mode = MAX31723_THERMOSTAT_MODE(DT_INST_PROP(inst, thermostat_mode)),   \
        .oneshot_mode = DT_INST_PROP(inst, oneshot_mode),                          \
    }

#define MAX31723_INST(inst)                                                 \
    static struct max31723_data max31723_data_##inst;                       \
                                                                            \
    static const struct max31723_dev_config max31723_dev_config_##inst =    \
         MAX31723_CONFIG_SPI(inst);                                               \
                                                                            \
    SENSOR_DEVICE_DT_INST_DEFINE(inst, &max31723_init, NULL,                 \
        &max31723_data_##inst, &max31723_dev_config_##inst, POST_KERNEL,    \
        CONFIG_SENSOR_INIT_PRIORITY, &max31723_api);

DT_INST_FOREACH_STATUS_OKAY(MAX31723_INST);
