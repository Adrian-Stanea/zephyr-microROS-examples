# DAC (with devicetree) micro-ROS Subscriber Example

This example demonstrates how to use Zephyr's generic DAC API together with
micro-ROS to control analog outputs from ROS2 topics. The application subscribes
to a ROS2 topic and outputs the received values to a configured DAC channel,
enabling real-time control of analog outputs from the ROS2 ecosystem.

## Overview

- **Zephyr DAC API**: Writes analog values to a DAC channel using Zephyr's
  device tree configuration.
- **micro-ROS Subscriber**: Subscribes to a ROS2 topic (dac/aout{channel_id}/raw)
  to receive values for the DAC output.
- **Configurable Resolution**: Automatically adapts to the DAC resolution
  specified in the device tree.

## Requirements

To run this application, you must have the following hardware setup:

- **AD5592R DAC Device**
  This example requires the [AD5592R](https://www.analog.com/en/products/ad5592r.html) digital-to-analog converter from Analog Devices.

- **SPI Interface Connection**
  The AD5592R device must be connected to your board via an SPI interface. Ensure your board supports SPI and that the device is properly wired to the corresponding SPI pins as defined in your device tree overlay.


## How It Works

1. The microcontroller runs Zephyr RTOS and the micro-ROS client.
2. The DAC channel is configured via the device tree overlay.
3. The application subscribes to the corresponding ROS2 topic (`dac/aout{channel_id}/raw`).
4. When a value is received, it is validated against the DAC's resolution range.
5. Valid values are written to the DAC channel, producing the corresponding
   analog output voltage.

## Running

1. Start the micro-ROS Agent on your host (see main README for instructions).
2. Connect your board via USB and flash the firmware.
3. Publish values to the DAC topic from any ROS2 node:

Data published for a 12-bit DAC (available raw values: 0...4095):
```shell
ros2 topic pub --once /dac/aout0/raw std_msgs/msg/Int32 '{data: 2048}'

# For full voltage output
ros2 topic pub --once /dac/aout0/raw std_msgs/msg/Int32 '{data: 4095}'

# For half voltage output
ros2 topic pub --once /dac/aout0/raw std_msgs/msg/Int32 '{data: 2047}'
```

⚠️ **Note**: Values must be within the range of the DAC resolution
(`0 to 2^resolution - 1`). Values outside this range will be ignored.

## Customization

- Edit the device tree overlay to select the DAC channel and its configuration.
- Change topic names or message types in the source code as needed.

## References

- [Zephyr DAC API Documentation](https://docs.zephyrproject.org/latest/hardware/peripherals/dac.html)
- [micro-ROS Documentation](https://micro.ros.org/docs/)
