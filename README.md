# Zephyr & micro-ROS Applications

A set of example applications that demonstrate how to:

- **Use Zephyr’s generic driver APIs** to interface with hardware (sensors, actuators, etc.).
- **Expose hardware via micro-ROS to the ROS2 ecosystem** using standard interfaces like topics and services.
- **Enable real-time communication** between embedded devices and ROS2 networks.
- **Ensure portability and interoperability** across various boards and hardware setups.

## Table of Contents
- [Zephyr \& micro-ROS Applications](#zephyr--micro-ros-applications)
  - [Table of Contents](#table-of-contents)
  - [Getting Started](#getting-started)
      - [micro-ROS Zephyr Module Integration](#micro-ros-zephyr-module-integration)
    - [Initialization](#initialization)
    - [Python Dependencies](#python-dependencies)
    - [Building and flashing](#building-and-flashing)
    - [Running the examples](#running-the-examples)
      - [micro-ROS Agent Setup](#micro-ros-agent-setup)
        - [What is the micro-ROS Agent?](#what-is-the-micro-ros-agent)
      - [Quick Setup with Docker (Recommended)](#quick-setup-with-docker-recommended)
        - [Step 1: Run the micro-ROS Agent](#step-1-run-the-micro-ros-agent)
        - [Step 2: Interact with ROS2 Topics](#step-2-interact-with-ros2-topics)
  - [License](#license)

**Features:**

- **ADC Integration**: Real-time analog sensor data streaming to ROS2 topics
- **Cross-Platform**: Support for multipleG

## Getting Started

Before getting started, make sure you have a proper Zephyr development
environment. Follow the official
[Zephyr Getting Started Guide](https://docs.zephyrproject.org/latest/getting_started/index.html).

#### micro-ROS Zephyr Module Integration

This repository follows the **standard Zephyr build paradigm** by integrating micro-ROS as a Zephyr module, ensuring seamless compatibility with existing Zephyr workflows and tooling.

⚠️ **Important**: The micro-ROS module branch/version in this repository is configured for **ROS2 Humble**. Ensure your micro-ROS Agent also uses the `humble` distribution to avoid compatibility issues:

### Initialization

The first step is to initialize the workspace folder (``microros-workspace``) where
the ``zephyr-microROS-examples`` and all Zephyr modules will be cloned. Run the following
command:

```shell
# initialize microros-workspace for the zephyr-microROS-examples (main branch)
west init -m https://github.com/Adrian-Stanea/zephyr-microROS-examples --mr humble microros-workspace

# update Zephyr modules
cd microros-workspace
west update
```

### Python Dependencies

Install aditional Python3 dependencies in a ``.venv`` required by ``libmicroros``.
This component needs ``colcon`` and other Python3 packages in order to build
micro-ROS packages. See [micro-ROS module for Zephyr](micro_ROS_zephyr_module)
for more details.

```shell
cd microros-workspace
python3 -m venv ./.venv
source ./.venv/bin/activate

pip3 install catkin_pkg lark-parser empy colcon-common-extensions

# NOTE: Use empy version 3.3.4 for Humble
pip3 install empy==3.3.4
```

[micro_ROS_zephyr_module]: https://github.com/micro-ROS/micro_ros_zephyr_module

### Building and flashing

To build an application, run the following command from the associated directory
under ``zephyr-microROS-examples/apps/``

```shell
# Prepare environment
cd microros-workspace
source zephyr/zephyr-env.sh
export ZEPHYR_SDK_INSTALL_DIR="<PATH_TO_ZEPHYR_SDK_INSTALL_DIR>"

# Build example application
cd microros-workspace/zephyr-microROS-examples/apps/<EXAMPLE_APP>
west build -p -b max78002evkit/max78002/m4
```

where `-b` is any board for which you may later add an overlay.

A sample debug configuration is also provided as an additional build target. To apply it, run the following
command:

```shell
west build -p -b max78002evkit/max78002/m4 -- -DEXTRA_CONF_FILE=debug.conf
```

Once you have built the application, run the following command to flash it:

```shell
west flash
```

⚠️ **Warning & Good Practice:**
A common issue when building is that previous example applications may set configuration options for the `libmicroros` library (such as the number of publishers, subscribers, or nodes) that persist and limit resources for future builds. For example, if `prj.conf` sets:

```
CONFIG_MICROROS_NODES="1"
CONFIG_MICROROS_PUBLISHERS="8"
CONFIG_MICROROS_SUBSCRIBERS="1"
CONFIG_MICROROS_CLIENTS="1"
CONFIG_MICROROS_SERVERS="1"
CONFIG_MICROROS_RMW_HISTORIC="4"
CONFIG_MICROROS_XRCE_DDS_MTU="512"
CONFIG_MICROROS_XRCE_DDS_HISTORIC="4"
config_microros_xrce_dds_historic="4"
```

the library may compile, but new entities (publishers, subscribers, etc.) may not have resources available, leading to undefined behavior.

**Best practice:**
Before building a new application, remove the `modules/lib/micro_ros_zephyr_module` directory and run `west update` to fetch a clean version of the micro-ROS module:

```shell
rm -rf microros-workspace/modules/lib/micro_ros_zephyr_module
west update
```
This ensures your build uses the correct configuration and avoids resource conflicts.

### Running the examples

For a detailed overview of each example application, including usage instructions and supported features, please refer to the [apps directory](apps/README.md).

#### micro-ROS Agent Setup

Before running any micro-ROS applications, you need a [micro-ROS Agent](https://micro.ros.org/docs/concepts/build_system/#:~:text=micro_ros_setup%20flash_firmware.sh-,micro%2DROS%20agent,-The%20micro%2DROS) running on your development machine (laptop/desktop) to bridge communication between your microcontroller and the ROS2 ecosystem.

##### What is the micro-ROS Agent?

The micro-ROS Agent is a ROS2 node that acts as a bridge between the DDS network and micro-ROS nodes running on microcontrollers. It handles message serialization, deserialization, and transport between your embedded device and ROS2 topics/services.

#### Quick Setup with Docker (Recommended)

The easiest way to run the micro-ROS Agent is using the official Docker images from [micro-ROS Docker Hub](https://hub.docker.com/u/microros).

##### Step 1: Run the micro-ROS Agent

For serial communication with your microcontroller:

```bash
docker run --rm -it \
    --device=/dev/ttyUSB0 \
    --group-add dialout \
    --cap-add SYS_RAWIO \
    microros/micro-ros-agent:humble serial --dev /dev/ttyUSB0 --baud 115200 -v6
```

**Note:** Replace `/dev/ttyUSB0` with your actual device path (e.g., `/dev/ttyUSB1` for some boards).

You should connect your board to your laptop via USB after starting the micro-ROS Agent. Once the board is connected, the micro-ROS application on the board will automatically attempt to connect to the Agent via the specified serial port.

**Agent logs when no client is connected:**

<div style="overflow-x: auto;">

```shell
[1756883856.983134] info     | TermiosAgentLinux.cpp | init                     | running...             | fd: 3
[1756883856.983859] info     | Root.cpp           | set_verbose_level        | logger setup           | verbose_level: 6
```

</div>

This means that the Agent is waiting for a client.

**Agent logs when a client (your board) connects:**

<div style="overflow-x: auto;">

```shell
[1756883888.254055] info     | TermiosAgentLinux.cpp | init                     | running...             | fd: 3
[1756883888.254612] info     | Root.cpp           | set_verbose_level        | logger setup           | verbose_level: 6
[1756883890.219070] info     | Root.cpp           | create_client            | create                 | client_key: 0x306BA847, session_id: 0x81
[1756883890.219133] info     | SessionManager.hpp | establish_session        | session established    | client_key: 0x306BA847, address: 0
[1756883890.219389] debug    | SerialAgentLinux.cpp | send_message             | [** <<SER>> **]        | client_key: 0x306BA847, len: 19, data:
0000: 81 00 00 00 04 01 0B 00 00 00 58 52 43 45 01 00 01 0F 00
[1756883890.244102] debug    | SerialAgentLinux.cpp | recv_message             | [==>> SER <<==]        | client_key: 0x306BA847, len: 56, data:
0000: 81 80 00 00 01 07 2E 00 00 0A 00 01 01 03 00 00 1F 00 00 00 00 01 01 10 17 00 00 00 7A 65 70 68
0020: 79 72 5F 67 70 69 6F 5F 63 6F 6E 74 72 6F 6C 6C 65 72 00 00 00 00 00 00
[1756883890.340169] debug    | SerialAgentLinux.cpp | recv_message             | [==>> SER <<==]        | client_key: 0x306BA847, len: 13, data:
0000: 81 00 00 00 0B 01 05 00 00 00 00 00 80
[1756883890.452193] debug    | SerialAgentLinux.cpp | recv_message             | [==>> SER <<==]        | client_key: 0x306BA847, len: 13, data:
0000: 81 00 00 00 0B 01 05 00 00 00 00 00 80
[1756883890.463566] info     | ProxyClient.cpp    | create_participant       | participant created    | client_key: 0x306BA847, participant_id: 0x000(1)
[1756883890.463664] debug    | SerialAgentLinux.cpp | send_message             | [** <<SER>> **]        | client_key: 0x306BA847, len: 14, data:
```

</div>

You will see additional debug and info messages indicating communication between the Agent and your board.

##### Step 2: Interact with ROS2 Topics

To view topics and data published by your micro-ROS device, run a separate container with ROS2 tools:

```bash
docker run --rm -it \
    --device=/dev/ttyUSB0 \
    --group-add dialout \
    --cap-add SYS_RAWIO \
    microros/base:humble bash
```

Inside this container, you can use standard ROS2 CLI tools:
```bash
# List available topics
ros2 topic list

# Echo messages from a specific topic
ros2 topic echo /your_sensor_topic

# List services
ros2 service list

# Show node information
ros2 node list
```

## License

This repository is open-sourced under the Apache-2.0 license. See the [LICENSE](LICENSE) file for details.
