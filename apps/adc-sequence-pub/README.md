# ADC (sequence) micro-ROS Publisher Example

This example builds upon the [Zephyr ADC Sequence Sample](https://docs.zephyrproject.org/latest/samples/drivers/adc/adc_sequence/README.html#adc_sequence) and demonstrates how to stream buffers of ADC samples from multiple channels to ROS2 topics using micro-ROS. It showcases advanced usage of Zephyr's ADC API, including sequence sampling and publishing arrays of samples for each channel.

## Overview

- **Zephyr ADC Sequence API**: Samples multiple ADC channels in a configurable sequence, collecting buffers of samples per channel.
- **micro-ROS Publisher**: Publishes each channel's buffer as a message (`std_msgs/msg/Int32MultiArray`) to a dedicated ROS2 topic (e.g., `/adc/ain0/mV`, `/adc/ain1/mV`, ...).

The number of channels and samples per buffer are configured via the device tree overlay and Kconfig options.

## How It Works

1. The microcontroller runs Zephyr RTOS and the micro-ROS client.
2. ADC channels and sampling parameters are configured via device tree and Kconfig.
3. The application periodically samples all configured channels, collecting a buffer of samples for each.
4. Each buffer is published to its corresponding ROS2 topic as an array message.

## Running

1. Start the micro-ROS Agent on your host (see main README for instructions).
2. Connect your board via USB and flash the firmware.
3. Subscribe to ADC topics from any ROS2 node:

```bash
ros2 topic echo /adc/ain0/mV
ros2 topic echo /adc/ain1/mV
# ...add more topics as configured in your overlay
```

Each message will contain an array of samples (in mV or raw units, depending on configuration).

## Customization

- Edit the device tree overlay to select ADC channels and their configuration.
- Adjust Kconfig options to set the number of samples per sequence and sampling interval.
- Change topic names or message types in the source code as needed.

## References

- [Zephyr ADC Sequence Sample](https://docs.zephyrproject.org/latest/samples/drivers/adc/adc_sequence/README.html#adc_sequence)
- [micro-ROS Documentation](https://micro.ros.org/docs/)


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
4. Each channel's value is published to its corresponding ROS2 topic.

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
layout:
  dim:
  - label: samples
    size: 5
    stride: 5
  data_offset: 0
data:
- 557
- 546
- 539
- 532
- 530
```

## Customization

- Edit the device tree overlay to select ADC channels and their configuration.
- Change topic names or message types in the source code as needed.

## References

- [Zephyr ADC Device Tree Sample](https://docs.zephyrproject.org/latest/samples/drivers/adc/adc_sequence/README.html)
- [micro-ROS Documentation](https://micro.ros.org/docs/)