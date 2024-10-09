# vetta-devices

ESP8266 Vetta device firmware

## Software Requirements

- Linux
- Git
- Python
- Autotools/CMake

## To update submodules

```git submodule update --init --recursive --remote --progress```

## Development Framework (ESP8266_FREERTOS_SDK) Installation steps:

- Install toolchain for your operating system: https://docs.espressif.com/projects/esp8266-rtos-sdk/en/latest/get-started/index.html#setup-toolchain

- Define Environment variable `IDF_PATH` pointing to your `ESP8266_RTOS_SDK` folder : https://docs.espressif.com/projects/esp8266-rtos-sdk/en/latest/get-started/index.html#setup-path-to-esp8266-rtos-sdk

- Install required Python packages:

```shell
python -m pip install --user -r $IDF_PATH/requirements.txt
```

## How to flash the firmware

- Initialize the project by entering inside the vetta-esp8226 folder, and issuing the command:

```shell
make menuconfig
```

- Connect the ESP8266 through an USB port and run the `espflash.py` tool with Python
