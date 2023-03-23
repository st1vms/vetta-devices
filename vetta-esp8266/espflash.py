#!/usr/bin/env python

import subprocess
import os, sys
from multiprocessing import cpu_count

__IDF_PATH = os.getenv("IDF_PATH")
__ESPTOOL_PATH = os.path.join(
    __IDF_PATH, "components", "esptool_py", "esptool", "esptool.py"
)

__BUILD_PATH = os.path.join(os.getcwd(), "build")
if not os.path.exists(__BUILD_PATH):
    os.mkdir(__BUILD_PATH)

__BOOTLOADER_BIN_PATH = os.path.join(__BUILD_PATH, "bootloader", "bootloader.bin")

__PARTITION_TABLE_BIN = os.path.join(__BUILD_PATH, "partition-table.bin")

__BINARY_BIN_PATH = os.path.join(__BUILD_PATH, "vetta-esp8266.bin")

__FLASH_PORT = sys.argv[1] if len(sys.argv) > 1 else "/dev/ttyUSB0"
__BAUD_RATE = "74880"
__FLASH_MODE = "dio"
__FLASH_FREQ = "80m"
__FLASH_SIZE = "4MB"

__MONITOR_ARGS = f"make monitor MONITORBAUD={__BAUD_RATE} ESPPORT={__FLASH_PORT}"

__FLASH_ARGS = f""" \
python {__ESPTOOL_PATH} \
--chip esp8266 --port {__FLASH_PORT} \
--baud {__BAUD_RATE} \
--before default_reset \
--after hard_reset \
write_flash -z \
--flash_mode {__FLASH_MODE} \
--flash_freq {__FLASH_FREQ} \
--flash_size {__FLASH_SIZE} \
0x0 {__BOOTLOADER_BIN_PATH} \
0x8000 {__PARTITION_TABLE_BIN} \
0x10000 {__BINARY_BIN_PATH} \
\
"""

__BUILD_ARGS = f"make bootloader app -j {cpu_count()}"


def _run() -> int:
    def _exec_args(args: tuple[str]) -> int:
        try:
            return subprocess.call(args, shell=True, stdout=sys.stdout)
        except KeyboardInterrupt:
            return -1

    os.environ[
        "CPPFLAGS"
    ] = "-DSPIFFS_OBJ_META_LEN=4 -DSPIFFS_ALIGNED_OBJECT_INDEX_TABLES=4"
    _res = 0
    __STEPS = (__BUILD_ARGS, __FLASH_ARGS) if len(sys.argv) > 1 else  (__BUILD_ARGS, __FLASH_ARGS, __MONITOR_ARGS)

    for step in __STEPS:
        _res = _exec_args(step)
        if _res == -1:
            break
    os.environ["CPPFLAGS"] = ""
    return _res


if __name__ == "__main__":

    # Build, Flash and Monitor
    sys.exit(_run())
