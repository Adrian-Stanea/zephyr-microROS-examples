# Sensor (Accelerometer) micro-ROS Publisher Example

This example demonstrates how to use Zephyr's generic sensor API together with micro-ROS to stream accelerometer data to ROS2 topics. The application reads sensor data from a configured accelerometer and publishes it to ROS2, enabling real-time integration of motion data with the ROS2 ecosystem

## Overview

- **Zephyr Sensor API**: Reads acceleration data from the ADXL367 sensor using Zephyr's device tree configuration and sensor abstraction.
- **micro-ROS Publisher**: Publishes sensor readings to a dedicated ROS2 topic (e.g., `/sensor/imu`).

The sensor and its configuration are selected via the board overlay file.

## Requirements

To run this application, you must have the following hardware setup:

- **ADXL367 Accelerometer Sensor**
  This example requires the [ADXL367](https://www.analog.com/en/products/adxl367.html)
  digital accelerometer from Analog Devices.

- **SPI Interface Connection**
  The ADXL367 sensor must be connected to your board via an SPI interface. Ensure
  your board supports SPI and that the sensor is properly wired to the
  corresponding SPI pins as defined in your device tree overlay.

⚠️ **Note**: This application is designed to work with any accelerometer that
has a supported sensor driver implementation in Zephyr. While the example uses
the ADXL367 sensor connected via SPI, you can use other accelerometers by
providing the appropriate device tree configuration and ensuring your board
supports the required interface.

## How It Works

1. The microcontroller runs Zephyr RTOS and the micro-ROS client.
2. The accelerometer is configured via the device tree overlay.
3. The application periodically samples the sensor using Zephyr's sensor API.
4. The sensor values (e.g., X, Y, Z acceleration) are published to a ROS2 topic
   (e.g., std_msgs/msg/Int32MultiArray or a custom message).

## Running

1. Start the micro-ROS Agent on your host (see main README for instructions).
2. Connect your board via USB and flash the firmware.
3. Subscribe to the sensor topic:

```shell
ros2 topic echo /sensor/imu
```

Expected output:
```shell
---
x: 0.6299070119857788
y: 3.0980639457702637
z: -2.3921759128570557
---
x: 0.691182017326355
y: 3.0588479042053223
z: -1.8431520462036133
---
```
The acceleartion data is published to the topic using a [geometry_msgs/Point32 Message](https://docs.ros.org/en/noetic/api/geometry_msgs/html/msg/Point32.html)

## Customization

- Edit the device tree overlay to select the sensor and its configuration.
- Change topic names or message types in the source code as needed.

## References

- [Zephyr ADC Device Tree Sample](https://docs.zephyrproject.org/latest/samples/drivers/adc/adc_dt/README.html#adc_dt)
- [micro-ROS Documentation](https://micro.ros.org/docs/)