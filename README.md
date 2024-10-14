# Pebble Nordic
 
This repository contains a basic application to test Pebble development boards
base on Nordic nRF SoCs.

## Getting started

Before getting started, make sure you have a proper nRF Connect SDK development
environment. Follow the official
[Installation guide](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/installation/install_ncs.html).

### Initialization

The first step is to initialize the workspace folder (`my-workspace`) where the
`pebble-nordic` and all nRF Connect SDK modules will be cloned. Run the
following command:

```shell
# initialize my-workspace for the pebble-nordic (main branch)
west init -m https://github.com/teslabs/pebble-nordic --mr main my-workspace
# update nRF Connect SDK modules
cd my-workspace
west update
```

### Building and running

To build the application, run the following command:

```shell
cd pebble-nordic
west build -b $BOARD app
```

where `$BOARD` is the target board.

A sample debug configuration is also provided. To apply it, run the following
command:

```shell
west build -b $BOARD app -- -DOVERLAY_CONFIG=debug.conf
```

Once you have built the application, run the following command to flash it:

```shell
west flash
```
