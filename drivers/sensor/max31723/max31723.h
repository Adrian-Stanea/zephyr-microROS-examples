
#ifndef ZEPHYR_DRIVERS_SENSOR_MAX31723_MAX31723_H_
#define ZEPHYR_DRIVERS_SENSOR_MAX31723_MAX31723_H_

#include <zephyr/drivers/sensor.h>
#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#define DT_DRV_COMPAT adi_max31723

#if DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)
#include <zephyr/drivers/spi.h>
#endif

#define MAX31723_RESOLUTION(x) ((x & 0x3) << 1)
typedef enum {
    RES_9BIT = 0x0,   // Conversion time: 25ms
    RES_10BIT = 0x1,  // Conversion time: 50ms
    RES_11BIT = 0x2,  // Conversion time: 100ms
    RES_12BIT = 0x3,  // Conversion time: 200ms
} max31723_resolution_e;

#define MAX31723_THERMOSTAT_MODE(x) ((x & 0x1) << 3)
typedef enum {
    COMPARATOR_MODE = 0,
    INTERRUPT_MODE = 1,
} max31723_thermostat_mode_e;

#endif // ZEPHYR_DRIVERS_SENSOR_MAX31723_MAX31723_H_