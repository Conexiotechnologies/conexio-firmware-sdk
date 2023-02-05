
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
CONFIG_GOLIOTH_SYSTEM_CLIENT_PSK_ID="my-psk-id@my-project"
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
<inf> regulator_fixed: sensor-pwr-ctrl sync: 0
uart:~$ *** Booting Zephyr OS build v3.1.99-ncs1  ***
<inf> lis2dh: fs=2, odr=0x4 lp_en=0x0 scale=9576
<inf> golioth_system: Initializing
<inf> golioth_lightdb_stream: Stratus < > Golioth Light DB sensor stream sample started
<inf> golioth_samples: Waiting for interface to be up
<inf> golioth_system: Starting connect
<inf> golioth_system: Client connected!
<dbg> golioth_lightdb_stream: main: Sending temp: 23.184176; humidity: 13.123367
<inf> golioth_lightdb_stream: Environmental data successfully pushed
<dbg> golioth_lightdb_stream: main: Sending temp: 23.184176; humidity: 13.123367
<dbg> golioth_lightdb_stream: env_data_push_handler: Data successfully pushed to the Golioth Cloud
<dbg> golioth_lightdb_stream: main: Sending temp: 23.352407; humidity: 12.842971
<inf> golioth_lightdb_stream: Environmental data successfully pushed
<dbg> golioth_lightdb_stream: main: Sending temp: 23.352407; humidity: 12.842971
<dbg> golioth_lightdb_stream: env_data_push_handler: Data successfully pushed to the Golioth Cloud
```
