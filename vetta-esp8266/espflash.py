#!/usr/bin/env python

import subprocess
import os, sys
import string, random
import qrcode
from multiprocessing import cpu_count


def _exec_args(args: tuple[str]) -> int:
    try:
        return subprocess.call(args, shell=True, stdout=sys.stdout)
    except KeyboardInterrupt:
        return -1


SPIFFS_IMAGE_FOLDER_PATH = os.path.join(os.getcwd(), "spiffs_image")
SPIFFS_AP_PASS_PATH = os.path.join(os.getcwd(), "spiffs_image", "ap.txt")

CREDS_OUTPUT_FOLDER_PATH = os.path.join(os.getcwd(), "output_creds")
if not os.path.exists(CREDS_OUTPUT_FOLDER_PATH):
    os.mkdir(CREDS_OUTPUT_FOLDER_PATH)

AP_PASS_OUTPUT_FOLDER_PATH = os.path.join(CREDS_OUTPUT_FOLDER_PATH, "ap")

FLASH_PORT = "/dev/ttyUSB0"

ESPTOOL_PATH = os.path.join(
    os.getenv("IDF_PATH"), "components", "esptool_py", "esptool", "esptool.py"
)


BAUD_RATE = "115200"

BUILD_PATH = os.path.join(os.getcwd(), "build")

BINARY_BUILD_PATH = os.path.join(BUILD_PATH, "vetta-esp8266.bin")

BOOTLOADER_BIN_PATH = os.path.join(BUILD_PATH, "bootloader", "bootloader.bin")

PARTITION_BIN = os.path.join(BUILD_PATH, "espvetta-partitions.bin")

FLASH_MODE = "dio"
FLASH_FREQ = "80m"
FLASH_SIZE = "4MB"

BUILD_ARGS = f"make app -j {cpu_count()}"
MONITOR_ARGS = "make monitor"

FLASH_ARGS = f"""
python {ESPTOOL_PATH} \
--chip esp8266 \
--port {FLASH_PORT} \
--baud {BAUD_RATE} \
--before default_reset --after hard_reset write_flash -z \
--flash_mode {FLASH_MODE} --flash_freq {FLASH_FREQ} --flash_size {FLASH_SIZE} \
0x0 {BOOTLOADER_BIN_PATH} 0x10000 {BINARY_BUILD_PATH} 0x8000 {PARTITION_BIN} \
\
"""


def gen_ap_pass() -> None:

    AP_PASS_LENGTH = 8
    AP_PASS_CHARS = string.digits

    p = "".join(random.choice(AP_PASS_CHARS) for _ in range(AP_PASS_LENGTH))

    # Write AP password to spiffs image folder
    with open(SPIFFS_AP_PASS_PATH, "w") as fp:
        fp.writelines([p])

    # Also dump the password into an output folder, for future references
    with open(f"{AP_PASS_OUTPUT_FOLDER_PATH}.txt", "w") as fp:
        fp.writelines([p])

    # Also dump the qr code in the output folder
    qrcode.make(p).save(f"{AP_PASS_OUTPUT_FOLDER_PATH}.png")


def _flashBuildMonitor() -> int:

    _res = 0
    for i, step in enumerate((BUILD_ARGS, FLASH_ARGS, MONITOR_ARGS)):

        if i > 0 and not os.path.exists(BINARY_BUILD_PATH):
            print("Error building binary...")
            res = -1
            break

        _res = _exec_args(step)
        if _res == -1:
            break

    return _res


if __name__ == "__main__":

    gen_ap_pass()
    # Build, Flash and Monitor
    sys.exit(_flashBuildMonitor())
