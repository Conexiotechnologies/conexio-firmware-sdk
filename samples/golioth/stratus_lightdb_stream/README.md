
# Stratus to Golioth Light DB stream sample

### [📣 Find our walk-through tutorial for this sample application → here](https://docs.conexiotech.com/sample-applications/golioth)

## Overview

This Light DB stream application demonstrates how to connect Conexio Stratus device with Golioth and
periodically send environmental data to Light DB stream. In this sample temperature and humidity
measurements from the SHT4x sensor are sent to `/temp` and `humidity` Light DB stream path. 

## Requirements
- Golioth credentials
- Network connectivity

## Application Configuration

Configure the following Kconfig options based on your Golioth credentials:

- GOLIOTH_SYSTEM_CLIENT_PSK_ID  - PSK ID of registered device
- GOLIOTH_SYSTEM_CLIENT_PSK     - PSK of registered device

by adding these lines to configuration file (e.g. `prj.conf`):

```
CONFIG_GOLIOTH_SYSTEM_CLIENT_PSK_ID="my-psk-id"
CONFIG_GOLIOTH_SYSTEM_CLIENT_PSK="my-psk"
```

## Building and Running the Sample Application
To compile the application, open a terminal window in the application directory and issue the following `west` command
```
west build -b conexio_stratus_ns
```

Once the application is compiled successfully, connect the Stratus device and put it into the DFU mode.

Flash the compiled firmware using `newtmgr`
```
newtmgr -c serial image upload build/zephyr/app_update.bin
```

Open up a serial console and reset the Stratus device. 

## Sample output
The following serial UART output will be displayed in the terminal. 
```
uart:~$ *** Booting Zephyr OS build v2.6.99-ncs1-rc2  ***
<inf> golioth_system: Initializing
<dbg> golioth_lightdb_stream.main: Start Stratus <> Golioth Light DB sensor stream sample
<inf> golioth_system: Starting connect
<dbg> golioth_lightdb_stream: temp: 26.164263; humidity: 29.002288
<inf> golioth_system: Client connected!
<inf> golioth_lightdb_stream: temp: 26.140230; humidity: 28.893554
<inf> golioth_lightdb_stream: temp: 26.161593; humidity: 28.840148
<inf> golioth_lightdb_stream: temp: 26.180285; humidity: 28.811538
```
