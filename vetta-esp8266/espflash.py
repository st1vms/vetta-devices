#!/usr/bin/env python

import subprocess
import os, sys
import string, random
import qrcode
from multiprocessing import cpu_count

__IDF_PATH = os.getenv("IDF_PATH")
__ESPTOOL_PATH = os.path.join(
    __IDF_PATH, "components", "esptool_py", "esptool", "esptool.py"
)
__SPIFFSGEN_PATH = os.path.join(os.getcwd(), "spiffsgen.py")

__AP_PASSWORD_FILENAME = "ap"
__AP_PASS_LENGTH = 8
__AP_PASS_CHARS = string.digits

__BUILD_PATH = os.path.join(os.getcwd(), "build")

__IMAGE_STORE_FOLDER_NAME = "output"
__SPIFFS_IMAGE_STORE_FOLDER_PATH = os.path.join(__BUILD_PATH, __IMAGE_STORE_FOLDER_NAME)

__IMAGE_FOLDER_NAME = "spiffs_image"
__SPIFFS_IMAGE_FOLDER_PATH = os.path.join(__BUILD_PATH, __IMAGE_FOLDER_NAME)

__BOOTLOADER_BIN_PATH = os.path.join(__BUILD_PATH, "bootloader", "bootloader.bin")

__PARTITION_TABLE_BIN = os.path.join(__BUILD_PATH, "espvetta-partitions.bin")

__BINARY_BIN_PATH = os.path.join(__BUILD_PATH, "vetta-esp8266.bin")

__PARTITION_STORAGE_BIN = os.path.join(__BUILD_PATH, "partition-storage.bin")

__FLASH_PORT = "/dev/ttyUSB0"
__BAUD_RATE = "115200"
__FLASH_MODE = "dio"
__FLASH_FREQ = "80m"
__FLASH_SIZE = "4MB"

__MONITOR_ARGS = "make monitor"

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
0x100000 {__PARTITION_STORAGE_BIN} \
\
"""

__SPIFFS_BUILD_ARGS = f""" \
python {__SPIFFSGEN_PATH} \
0x1000 {__SPIFFS_IMAGE_FOLDER_PATH} {__PARTITION_STORAGE_BIN} \
--aligned-obj-ix-tables \
--no-magic \
--no-magic-len \
--meta-len 4 \
--obj-name-len 8 \
--page-size 256 \
--block-size 4096 \
\
"""

__BUILD_ARGS = f"make bootloader app -j {cpu_count()}"

def gen_ap_pass(
    qrdump: bool = True,
    output_filename:str=__AP_PASSWORD_FILENAME,
    spiff_image_dir: str = __SPIFFS_IMAGE_FOLDER_PATH,
    store_path_dir: str = __SPIFFS_IMAGE_STORE_FOLDER_PATH,
) -> None:

    if not os.path.exists(spiff_image_dir):
        os.mkdir(spiff_image_dir)

    if not os.path.exists(store_path_dir):
        os.mkdir(store_path_dir)

    fi, fs = (
        os.path.join(spiff_image_dir, output_filename),
        os.path.join(store_path_dir, output_filename),
    )

    t = ''.join(random.choice(__AP_PASS_CHARS) for _ in range(__AP_PASS_LENGTH))
    p = f"{t}\0".encode(
        "utf-8"
    )

    # Write AP password to spiffs image folder
    with open(fi, "wb") as fp:
        fp.write(p)

    # Also dump the password into an output folder, for future references
    with open(fs, "wb") as fp:
        fp.write(p)

    if qrdump:
        # Also dump the qr code in the output folder
        qrcode.make(t).save(f"{fs}.png")


def _run() -> int:
    def _exec_args(args: tuple[str]) -> int:
        try:
            return subprocess.call(args, shell=True, stdout=sys.stdout)
        except KeyboardInterrupt:
            return -1

    os.environ["CPPFLAGS"] = "-DSPIFFS_OBJ_META_LEN=4"
    _res = 0
    for step in (__BUILD_ARGS, __SPIFFS_BUILD_ARGS, __FLASH_ARGS, __MONITOR_ARGS):
        _res = _exec_args(step)
        if _res == -1:
            break
    os.environ["CPPFLAGS"] = ""
    return _res


if __name__ == "__main__":

    # Generate AP password into image and store folder
    gen_ap_pass()

    # Build, Flash and Monitor
    sys.exit(_run())
