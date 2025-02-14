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

Once you have built the application, run the following command to flash it:

```shell
west flash
```

By default boards use the `jlink` runner. Alternative runners can be used with
`west flash -r $RUNNER_NAME`.

### Boards

Supported boards:

- `asterix_evt1`

## Usage

Connect `UART_TX` and `UART_RX` to a host, using e.g. a USB-UART converter.
Open a serial console using `115200@8N1` settings. Once the device is powered
up, either via `VBUS` or `VBAT`, you should observe a boot banner and
immediately after a shell prompt: `uart:~$`. Below you have a short guide of
available commands.

### BLE

| Command | Description |
| --- | --- |
| `hwv ble on` | Turn ON BLE and advertising as `Pebble HWV` |
| `hwv ble off` | Turn OFF BLE |

You can use any utility to test connection and writing to the exposed GATT
characteristic (e.g. [LightBlue](https://punchthrough.com/lightblue/)). Note
that after disconnecting the firmware will no longer advertise.

### Buttons

| Command | Description |
| --- | --- |
| `hwv buttons check` | Check if buttons are pressed/release |

### Charger

| Command | Description |
| --- | --- |
| `hwv charger status` | Check charger status |

To get meaningful status reports, you will need to plug the battery to `VBAT`,
`GND` and `NTC`. To test charging, connect `VBUS` to +5V.

### Display

| Command | Description |
| --- | --- |
| `hwv display on` | Turn ON the display |
| `hwv display off` | Turn OFF the display |
| `hwv display vpattern` | Draw a vertical pattern |
| `hwv display hpattern` | Draw an horizontal pattern |
| `hwv display brightness $VAL` | Adjust display backlight brightness, `$VAL: 0-100` |

### Flash

| Command | Description |
| --- | --- |
| `hwv flash id` | Read flash chip JEDEC ID |
| `hwv flash erase $ADDR` | Erase flash page for the given `$ADDR` |
| `hwv flash read $ADDR $N` | Read `$N` bytes from address `$ADDR` |
| `hwv flash write $ADDR $VAL` | Write `$VAL` (hex encoded, e.g. `aabbccdd`) to `$ADDR` |

### Haptic

| Command | Description |
| --- | --- |
| `hwv haptic configure $VAL` | Configure haptic motor intensity `$VAL: 0-100` |

### Sensors

| Command | Description |
| --- | --- |
| `hwv imu get` | Obtain IMU readings (acc/gyro) |
| `hwv light get` | Obtain ALS readings |
| `hwv mag get` | Obtain magnetometer readings |
| `hwv press get` | Obtain pressure sensor readings |

### Speaker 

| Command | Description |
| --- | --- |
| `hwv speaker play` | Play sound on speaker |

### Microphone 

| Command | Description |
| --- | --- |
| `hwv mic capture` | Capture microphone data |
