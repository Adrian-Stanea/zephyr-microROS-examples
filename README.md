# Zephyr Basic Sensor Application

This repository contains a Zephyr example application. It is a minimal offspring of https://github.com/zephyrproject-rtos/example-application/tree/main.  The purpose of this repository is to demonstrate Some of the features demonstrated in this example are:

- Basic [Zephyr application][app_dev] skeleton
- [Zephyr workspace applications][workspace_app]
- [Zephyr modules][modules]
- [West T2 topology][west_t2]
- Custom [devicetree bindings][bindings]
- Out-of-tree [drivers][drivers]

The *primary* purpose of this project is to demonstrate how to build a new sensor driver using a workspace application. It includes a very simple driver for the [MAX31723][max31723], an incredibly simple temperature sensor. 

This repository is versioned together with the [Zephyr main tree][zephyr]. This
means that every time that Zephyr is tagged, this repository is tagged as well
with the same version number, and the [manifest](west.yml) entry for `zephyr`
will point to the corresponding Zephyr tag. For example, the `sensor-app`
v2.6.0 will point to Zephyr v2.6.0. Note that the `main` branch always
points to the development branch of Zephyr, also `main`.

[app_dev]: https://docs.zephyrproject.org/latest/develop/application/index.html
[workspace_app]: https://docs.zephyrproject.org/latest/develop/application/index.html#zephyr-workspace-app
[modules]: https://docs.zephyrproject.org/latest/develop/modules.html
[west_t2]: https://docs.zephyrproject.org/latest/develop/west/workspaces.html#west-t2
[bindings]: https://docs.zephyrproject.org/latest/guides/dts/bindings.html
[drivers]: https://docs.zephyrproject.org/latest/reference/drivers/index.html
[max31723]: https://www.analog.com/en/products/MAX31723.html
[zephyr]: https://github.com/zephyrproject-rtos/zephyr

## Getting Started

Before getting started, make sure you have a proper Zephyr development
environment. Follow the official
[Zephyr Getting Started Guide](https://docs.zephyrproject.org/latest/getting_started/index.html).

### Initialization

The first step is to initialize the workspace folder (``my-workspace``) where
the ``basic-sensor-app`` and all Zephyr modules will be cloned. Run the following
command:

```shell
# initialize my-workspace for the example-application (main branch)
west init -m https://github.com/Brandon-Hurst/basic-sensor-app --mr max31723 sensor-workspace

# update Zephyr modules
cd sensor-workspace
west update
```

### Building and running

To build the application, run the following command:

```shell
cd basic-sensor-app
west build -p -b $BOARD app
```

where `$BOARD` is the `apard32690/max32690/m4` board target, or any board for which you may later add an overlay.

A sample debug configuration is also provided as an additional build target. To apply it, run the following
command:

```shell
west build -b $BOARD app -- -DEXTRA_CONF_FILE=debug.conf
```

Once you have built the application, run the following command to flash it:

```shell
west flash
```
