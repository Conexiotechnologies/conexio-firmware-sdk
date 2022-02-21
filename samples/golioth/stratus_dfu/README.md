
# Stratus Device Firmware Update (DFU) using Golioth Platform

### [📣 Find our walk-through tutorial for this sample application → here](https://www.rajeevpiyare.com/posts/stratus-to-golioth/)

## Overview
This sample applications demonstrates how to connect
- Conexio Stratus device with Golioth and periodically send environmental data to Light DB stream. In this sample temperature and humidity measurements from the SHT4x sensor are sent to `/temp` and `humidity` Light DB stream path. 
- with Golioth and use Device Firmware Upgrade (DFU) procedure.

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

## Prepare New Firmware
For testing purposes of DFU mechanism the same firmware will be used. To
distinguish between old firmware and a new one, a firmware version will be
assigned during image build process. Execute following command to generate new
application image:
```
west build -b conexio_stratus_ns -- -DCONFIG_MCUBOOT_IMAGE_VERSION=\"2.0.0\"
```

Start DFU using goliothctl
==========================

Run following command on host PC to upload new firmware as artifact to Golioth:
```
goliothctl artifact create new.bin --version 2.0.0
```

Then create new release consisting of this single firmware and roll it out to
all devices in a project:
```
goliothctl release --release-tags 2.0.0 --components main@2.0.0 --rollout true
```
DFU process should be started in Zephyr and this is what should be visible on
serial console:

```
[2021-12-04 19:25:19] uart:~$ *** Booting Zephyr OS build v2.6.99-ncs1-1  ***
[2021-12-04 19[00:00:00.216,491] <inf> golioth_system: Initializing
[2021-12-04 19:25:20] [00:00:03.056,213] <dbg> golioth_dfu.main: Start DFU sample
[2021-12-04 19:25:23] [00:00:03.056,427] <inf> golioth_system: Starting connect
[2021-12-04 19:25:23] [00:00:03.065,734] <dbg> golioth_dfu.main: Sending temperature data 26.185626
[2021-12-04 19:25:23] [00:00:04.066,436] <dbg> golioth_dfu.main: Sending humidity data 16.270538
[2021-12-04 19:25:24] [00:00:04.971,679] <inf> golioth_system: Client connected!
[2021-12-04 19:25:24] [00:00:05.489,624] <dbg> golioth_dfu: Desired
[2021-12-04 19:25:25]                                       a3 01 1a 61 ac 13 f0 02  78 40 33 61 64 34 32 37 |...a.... x@3ad427
[2021-12-04 19:25:25]                                       32 35 31 38 31 66 38 37  31 36 39 32 62 65 35 66 |25181f87 1692be5f
[2021-12-04 19:25:25]                                       62 66 65 32 32 32 62 32  30 37 61 61 33 64 34 35 |bfe222b2 07aa3d45
[2021-12-04 19:25:25]                                       37 35 62 39 34 30 30 36  36 30 64 36 63 30 30 38 |75b94006 60d6c008
[2021-12-04 19:25:25]                                       66 37 66 34 30 36 33 32  33 33 03 81 a6 01 64 6d |f7f40632 33....dm
[2021-12-04 19:25:25]                                       61 69 6e 02 65 32 2e 30  2e 30 03 78 40 63 39 35 |ain.e2.0 .0.x@c95
[2021-12-04 19:25:25]                                       39 64 30 62 30 31 62 33  62 65 62 65 38 64 32 33 |9d0b01b3 bebe8d23
[2021-12-04 19:25:25]                                       64 63 34 61 36 63 30 33  34 36 31 36 33 64 37 61 |dc4a6c03 46163d7a
[2021-12-04 19:25:25]                                       37 65 36 66 63 33 63 39  31 64 66 31 31 30 38 63 |7e6fc3c9 1df1108c
[2021-12-04 19:25:25]                                       37 37 39 32 38 31 35 65  30 61 32 35 36 04 1a 00 |7792815e 0a256...
[2021-12-04 19:25:25]                                       04 88 a4 05 70 2f 2e 75  2f 63 2f 6d 61 69 6e 40 |....p/.u /c/main@
[2021-12-04 19:25:25]                                       32 2e 30 2e 30 06 67 6d  63 75 62 6f 6f 74       |2.0.0.gm cuboot
[2021-12-04 19:25:25] [00:00:05.489,929] <inf> golioth: Manifest sequence-number: 1638667248
[2021-12-04 19:25:25] [00:00:06.066,284] <dbg> golioth_dfu.data_received: Received 1024 bytes at offset 0
[2021-12-04 19:25:25] [00:00:06.066,436] <inf> mcuboot_util: Swap type: none
[2021-12-04 19:25:25] [00:00:06.066,436] <inf> golioth_dfu: swap type: none
[2021-12-04 19:25:25] [00:00:06.670,684] <dbg> golioth_dfu.data_received: Received 1024 bytes at offset 1024
[2021-12-04 19:25:26] [00:00:07.221,252] <dbg> golioth_dfu.data_received: Received 1024 bytes at offset 2048
[2021-12-04 19:25:26] [00:00:07.735,870] <dbg> golioth_dfu.data_received: Received 1024 bytes at offset 3072
[2021-12-04 19:25:27] [00:00:08.340,301] <dbg> golioth_dfu.data_received: Received 1024 bytes at offset 4096
...
[2021-12-04 19:28:01] [00:02:41.984,100] <dbg> golioth_dfu.data_received: Received 1024 bytes at offset 294912
[2021-12-04 19:28:02] [00:02:42.606,506] <dbg> golioth_dfu.data_received: Received 1024 bytes at offset 295936
[2021-12-04 1[00:02:43.055,969] <dbg> golioth_dfu.data_received: Received 827 bytes at offset 296960 (last)
[2021-12-04 19[00:02:43.155,822] <inf> golioth_dfu: Requesting upgrade
[2021-12-04 19[00:02:43.156,188] <inf> golioth_dfu: Rebooting in 1 second(s)
```

After few a minutes depending on firmware size, new firmware will be booted from first application slot and following messages should appear on the serial console
indicating that the `Desired version (x.x.x) matches current firmware version!`:
```
[2021-12-04 19:29:32] uart:~$ *** Booting Zephyr OS build v2.6.99-ncs1-1  ***
[2021-12-04 19[00:00:00.216,461] <inf> golioth_system: Initializing
[2021-12-04 19:29:34] [00:00:02.709,014] <dbg> golioth_dfu.main: Starting Stratus DFU sample
[2021-12-04 19:29:36] [00:00:02.709,167] <inf> golioth_system: Starting connect
[2021-12-04 19:29:36] [00:00:02.718,475] <dbg> golioth_dfu.main: Sending temperature data 26.383230
[2021-12-04 19:29:36] [00:00:03.719,146] <dbg> golioth_dfu.main: Sending humidity data 19.673294
[2021-12-04 19:29:37] [00:00:04.032,409] <inf> golioth_system: Client connected!
[2021-12-04 19:29:37] [00:00:04.481,170] <dbg> golioth_dfu: Desired
[2021-12-04 19:29:37]                                       a3 01 1a 61 ac 13 f0 02  78 40 33 61 64 34 32 37 |...a.... x@3ad427
[2021-12-04 19:29:37]                                       32 35 31 38 31 66 38 37  31 36 39 32 62 65 35 66 |25181f87 1692be5f
[2021-12-04 19:29:37]                                       62 66 65 32 32 32 62 32  30 37 61 61 33 64 34 35 |bfe222b2 07aa3d45
[2021-12-04 19:29:37]                                       37 35 62 39 34 30 30 36  36 30 64 36 63 30 30 38 |75b94006 60d6c008
[2021-12-04 19:29:37]                                       66 37 66 34 30 36 33 32  33 33 03 81 a6 01 64 6d |f7f40632 33....dm
[2021-12-04 19:29:37]                                       61 69 6e 02 65 32 2e 30  2e 30 03 78 40 63 39 35 |ain.e2.0 .0.x@c95
[2021-12-04 19:29:37]                                       39 64 30 62 30 31 62 33  62 65 62 65 38 64 32 33 |9d0b01b3 bebe8d23
[2021-12-04 19:29:37]                                       64 63 34 61 36 63 30 33  34 36 31 36 33 64 37 61 |dc4a6c03 46163d7a
[2021-12-04 19:29:37]                                       37 65 36 66 63 33 63 39  31 64 66 31 31 30 38 63 |7e6fc3c9 1df1108c
[2021-12-04 19:29:37]                                       37 37 39 32 38 31 35 65  30 61 32 35 36 04 1a 00 |7792815e 0a256...
[2021-12-04 19:29:37]                                       04 88 a4 05 70 2f 2e 75  2f 63 2f 6d 61 69 6e 40 |....p/.u /c/main@
[2021-12-04 19:29:37]                                       32 2e 30 2e 30 06 67 6d  63 75 62 6f 6f 74       |2.0.0.gm cuboot
[2021-12-04 19:29:37] [00:00:04.481,475] <inf> golioth: Manifest sequence-number: 1638667248
[2021-12-04 19:29:37] [00:00:04.482,543] <inf> golioth_dfu: Desired version (2.0.0) matches current firmware version!
```
