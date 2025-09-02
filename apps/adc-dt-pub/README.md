# ADC (with devicetree) micro-ROS Publisher Example

This example demonstrates how to use Zephyr's generic ADC API together with micro-ROS to stream analog sensor data to ROS2 topics. The application publishes ADC readings from each configured channel to a separate ROS2 topic, enabling real-time sensor data integration with the ROS2 ecosystem.

## Overview

- **Zephyr ADC API**: Reads analog values from one or more ADC channels using Zephyr's device tree configuration.
- **micro-ROS Publisher**: Publishes each ADC channel's data to a dedicated ROS2 topic (e.g., `/adc/ain0/mV`, `/adc/ain1/mV`, ...).

The number of topics (and ADC channels) is configured via the board overlay file.

## How It Works

1. The microcontroller runs Zephyr RTOS and the micro-ROS client.
2. ADC channels are configured via the device tree overlay.
3. The application periodically samples each ADC channel.
4. Each channel's value is published to its corresponding ROS2 topic (e.g., `std_msgs/msg/Int32`).

## Running

1. Start the micro-ROS Agent on your host (see main README for instructions).
2. Connect your board via USB and flash the firmware.
3. Subscribe to ADC topics from any ROS2 node:

```shell
ros2 topic echo /adc/ain0/mV
ros2 topic echo /adc/ain1/mV
```
Expected output:
```shell
data: 546
---
data: 534
---
data: 551
---
data: 550
```

## Customization

- Edit the device tree overlay to select ADC channels and their configuration.
- Change topic names or message types in the source code as needed.

## References

- [Zephyr ADC Device Tree Sample](https://docs.zephyrproject.org/latest/samples/drivers/adc/adc_dt/README.html#adc_dt)
- [micro-ROS Documentation](https://micro.ros.org/docs/)