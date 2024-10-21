import argparse
import struct
import time

from pyftdi.i2c import I2cController

# Shunt resistor value, ohms
R_SHUNT = 0.5

# Power bus config: name <-> (addr, current_lsb (A))
BUS_CONFIG = {
    "VUSB": (0x40, 1 / 1e4),
    "VBAT": (0x41, 1 / 1e4),
    "VSYS": (0x42, 1 / 1e4),
    "LIGHT": (0x43, 1 / 1e6),
    "nPM_1.8V": (0x44, 1 / 1e6),
    "nPM_3.0V": (0x45, 1 / 1e6),
    "VDDIO": (0x46, 1 / 1e6),
    "FLASH": (0x47, 1 / 1e6),
    "VBUS_O": (0x48, 1 / 1e4),
    "VDD_nRF": (0x49, 1 / 1e6),
    "LCD": (0x4A, 1 / 1e6),
    "LED": (0x4B, 1 / 1e6),
    "SENSOR_1.8V": (0x4C, 1 / 1e6),
    "SPK": (0x4D, 1 / 1e6),
    "MIC": (0x4E, 1 / 1e6),
    "LRA": (0x4F, 1 / 1e6),
}


class INA226(object):
    _REG_CONFIG = 0x00
    _REG_SHUNT_VOLT = 0x01
    _REG_BUS_VOLT = 0x02
    _REG_POWER = 0x03
    _REG_CURRENT = 0x04
    _REG_CALIB = 0x05
    _REG_MASK = 0x06
    _REG_ALERT = 0x07
    _REG_MF_ID = 0xFE
    _REG_DIE_ID = 0xFF

    _MODE_PD = 0x00
    _MODE_TRIG_VSHUNT = 0x01
    _MODE_TRIG_VBUS = 0x02
    _MODE_TRIG_ALL = 0x03
    _MODE_CONT_VSHUNT = 0x05
    _MODE_CONT_VBUS = 0x06
    _MODE_CONT_ALL = 0x07

    _VSHUNT_CT_140US = 0 << 3
    _VSHUNT_CT_204US = 1 << 3
    _VSHUNT_CT_332US = 2 << 3
    _VSHUNT_CT_588US = 3 << 3
    _VSHUNT_CT_1100US = 4 << 3
    _VSHUNT_CT_2116US = 5 << 3
    _VSHUNT_CT_4156US = 6 << 3
    _VSHUNT_CT_8244US = 7 << 3

    _VBUS_CT_140US = 0 << 6
    _VBUS_CT_204US = 1 << 6
    _VBUS_CT_332US = 2 << 6
    _VBUS_CT_588US = 3 << 6
    _VBUS_CT_1100US = 4 << 6
    _VBUS_CT_2116US = 5 << 6
    _VBUS_CT_4156US = 6 << 6
    _VBUS_CT_8244US = 7 << 6

    _AVG_1 = 0 << 9
    _AVG_4 = 1 << 9
    _AVG_16 = 2 << 9
    _AVG_64 = 3 << 9
    _AVG_128 = 4 << 9
    _AVG_256 = 5 << 9
    _AVG_512 = 6 << 9
    _AVG_1024 = 7 << 9

    _MF_ID = 0x5449
    _DIE_ID = 0x2260

    def __init__(self, i2c, addr, current_lsb):
        self._slave = i2c.get_port(addr)

        # verify it's an INA226
        mf_id = int.from_bytes(
            self._slave.read_from(INA226._REG_MF_ID, 2), byteorder="big", signed=False
        )
        if mf_id != INA226._MF_ID:
            raise ValueError(f"Unexpected manufacturer ID: 0x{mf_id:04x}")

        die_id = int.from_bytes(
            self._slave.read_from(INA226._REG_DIE_ID, 2), byteorder="big", signed=False
        )
        if die_id != INA226._DIE_ID:
            raise ValueError(f"Unexpected die ID: 0x{die_id:04x}")

        # configure calibration, ref 6.5
        self._current_lsb = current_lsb
        cal = int(0.00512 / (self._current_lsb * R_SHUNT))
        if cal > 0xFFFF:
            raise ValueError("Unsupported calibration value, adjust current LSB")

        self._slave.write_to(INA226._REG_CALIB, struct.pack(">H", cal))

        # configure:
        #  - Vshunt and Vbus, continuous mode
        #  - Vshunt conversion time: 1.1ms
        #  - Vbus conversion time: 1.1ms
        #  - # averages: 256
        # expected conversion time: = 281.6ms
        config = (
            INA226._MODE_CONT_ALL
            | INA226._VSHUNT_CT_1100US
            | INA226._VBUS_CT_1100US
            | INA226._AVG_256
        )
        self._slave.write_to(INA226._REG_CONFIG, struct.pack(">H", config))

    @property
    def bus_voltage(self):
        raw = self._slave.read_from(INA226._REG_BUS_VOLT, 2)
        val = int.from_bytes(raw, byteorder="big", signed=False)
        return val * 0.00125

    @property
    def shunt_voltage(self):
        raw = self._slave.read_from(INA226._REG_SHUNT_VOLT, 2)
        val = int.from_bytes(raw, byteorder="big", signed=False)
        return val * 0.00025

    @property
    def current(self):
        raw = self._slave.read_from(INA226._REG_CURRENT, 2)
        val = int.from_bytes(raw, byteorder="big", signed=False)
        return val * self._current_lsb

    @property
    def power(self):
        raw = self._slave.read_from(INA226._REG_POWER, 2)
        val = int.from_bytes(raw, byteorder="big", signed=False)
        return val * 25 * self._current_lsb


def parse_args():
    parser = argparse.ArgumentParser()

    parser.add_argument("-b", "--bus", type=str, required=True, help="Power bus")
    parser.add_argument("-f", "--ftdi", type=str, default="ftdi:///0", help="FTDI URL")

    return parser.parse_args()


def main():
    args = parse_args()

    config = BUS_CONFIG.get(args.bus)
    if config is None:
        raise ValueError(f"Bus {args.bus} not supported")

    i2c = I2cController()
    i2c.configure(args.ftdi)

    ina226 = INA226(i2c, config[0], config[1])

    while True:
        try:
            print(
                f"{args.bus}: "
                f"{ina226.bus_voltage:.3f} V, "
                f"{ina226.shunt_voltage * 1000:.3f} mV, "
                f"{ina226.current * 1000:.3f} mA, "
                f"{ina226.power * 1000:.3f} mW",
                end="\r"
            )

            time.sleep(0.5)
        except KeyboardInterrupt:
            print("")
            break


if __name__ == "__main__":
    main()
