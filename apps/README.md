# Example Applications

This directory contains example applications that demonstrate how to use Zephyr's generic driver APIs for seamless hardware integration across a wide range of boards. By leveraging these APIs, you can connect low-level hardware components to standard ROS2 interfaces, enabling sensor data streaming and actuator control that works out of the box with micro-ROS. Each example highlights practical use cases for bridging Zephyr drivers with ROS2 topics and services, ensuring portability and interoperability in your robotics projects.

## ADC

- **[adc-dt-pub:](./adc-dt-pub/README.md)**
  *Streams ADC channel readings to individual ROS2 topics using Zephyr's generic ADC API and device tree configuration.*

- **[adc-sequence-pub:](./adc-sequence-pub/README.md)**
  *Publishes buffers of ADC samples from multiple channels to ROS2 topics, showcasing advanced sequence sampling.*

## GPIO

- **[blinky-controller-sub](./blinky-controller-sub/README.md)**
  *Combines Zephyr's blinky sample with a micro-ROS subscriber to control an LED from a ROS2 topic.*
