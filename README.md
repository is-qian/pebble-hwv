# CoreDevices Hardware Verification Firmware (HWV)

This repository contains a basic application to verify CoreDevices hardware.

## Getting started

Before getting started, make sure you have a proper Zephyr development
environment. Follow the official
[Zephyr Getting Started Guide](https://docs.zephyrproject.org/latest/getting_started/index.html).

### Initialization

The first step is to initialize the workspace folder (`my-workspace`) where the
`pebble-hwv` and all Zephyr modules will be cloned. Run the following command:

```shell
# initialize my-workspace for the pebble-hwv (main branch)
west init -m https://github.com/coredevices/pebble-hwv --mr main my-workspace
# update Zephyr modules
cd my-workspace
west update
```

### Building and running

To build the application, run the following command:

```shell
cd pebble-hwv
west build -b $BOARD_TARGET app
```

where `$BOARD_TARGET` is the board target, e.g. `asterix_evt1`.

A sample debug configuration is also provided. To apply it, run the following
command:

```shell
west build -b $BOARD_TARGET app -- -DOVERLAY_CONFIG=debug.conf
```

Once you have built the application, run the following command to flash it:

```shell
west flash
```

By default boards use the `jlink` runner. Some boards may require to disable
the level shifter between SWD lines/FTDI to use the JTAG header with JLink.
Alternative runners can be used with `west flash --runner $RUNNER_NAME`.
Supported alternative runners are `nrfjprog` (JLink-based), or, if using FTDI,
`openocd` (make sure to build a recent version).

### Boards

Supported boards:

- `asterix_evt1`

### Snippets

Supported snippets:

- `uart-logging`: Enables logging and console overt UART
- `rtt-logging`: Enables logging and console overt RTT
