# Blinky micro-ROS Controller Example

This example demonstrates how to combine the classic Zephyr "blinky" sample with a micro-ROS subscriber. The application listens to a ROS2 topic and actuates an LED on the board based on the received data.

## Overview

- **Zephyr Blinky**: Toggles an LED using Zephyr's GPIO API.
- **micro-ROS Subscriber**: Subscribes to a ROS2 topic and sets the LED state according to the published messages.

This allows remote control of the LED from any ROS2 node on your network.

## How It Works

1. The microcontroller runs Zephyr RTOS and the micro-ROS client.
2. The application subscribes to a ROS2 topic (default: `/led_control`).
3. When a message is received (e.g., `std_msgs/msg/Int32`), the LED is turned ON or OFF accordingly.

## Running

1. Start the micro-ROS Agent on your host (see main README for instructions).
2. Connect your board via USB and flash the firmware.
3. Publish messages to the `/led_control` topic from any ROS2 node:

```bash
ros2 topic pub --once /led_control std_msgs/msg/Int32 "{ data: 1 }"  # LED ON
ros2 topic pub --once /led_control std_msgs/msg/Int32 "{ data: 0 }"  # LED OFF
```

## Customization

- Change the topic name or message type in the source code as needed.
- Use overlays to select a different LED or GPIO pin.

## References

- [Zephyr Blinky Sample](https://docs.zephyrproject.org/latest/samples/basic/blinky/README.html#blinky)
- [micro-ROS Documentation](https://micro.ros.org/docs/)