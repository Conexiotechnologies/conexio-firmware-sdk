# Stratus Pro - Golioth stream sample

## Overview

This Golioth data streaming application demonstrates how to connect with
Golioth and periodically send data to the stream service. In this sample
temperature measurements will be displayed on the `/temp` stream
path. For platforms that do not have a temperature sensor, a value is
generated from 20 up to 30.


## Requirements

* Golioth SDK pre-installed for NCS v2.9.0
* Golioth account and credentials
* Network connectivity

### Installing the Golioth Firmware SDK

## Adding the Golioth Firmware SDK to an existing nRF Connect SDK based projects
Add the following entry to the `ncs/v2.9.0/nrf/west.yml` file of an existing West based project:

```yaml
# Golioth repository.
- name: golioth
  path: modules/lib/golioth-firmware-sdk
  revision: v0.17.0
  url: https://github.com/golioth/golioth-firmware-sdk.git
  submodules: true
```

> [!WARNING]
> To ensure that default Kconfig values are propagated correctly, place
> the golioth entry first in your west manifest.

and clone all repositories by running:

```console
west update
```

## Building and Running the sample

To build the application, see the `Conexio device documentation <https://docs.conexiotech.com/master/building-and-programming-an-application/compiling-applications-with-nrf-connect-extension-for-vs-code>`_ for 
detailed instructions on how to build and flash the application to the Stratus devices.

Flash the compiled firmware using `newtmgr`

  newtmgr -c serial image upload build/zephyr/app_update.bin

### Authentication specific configuration

#### PSK based auth - Hardcoded

Configure the following Kconfig options based on your Golioth
credentials by adding these lines to configuration file `prj.conf`:

```cfg
CONFIG_GOLIOTH_SAMPLE_PSK_ID="my-psk-id"
CONFIG_GOLIOTH_SAMPLE_PSK="my-psk"
```
### Sample output

This is the output from the serial console:

```console
*** Booting nRF Connect SDK v2.9.0-7787b2649840 ***
*** Using Zephyr OS v3.7.99-1f8f3dc29142 ***
*** Golioth Firmware SDK v0.17.0 ***
[00:00:00.512,329] <dbg> golioth_stream: main: Start Golioth stream sample for conexio_stratus_pro
[00:00:00.512,329] <inf> golioth_samples: Bringing up network interface
[00:00:00.512,359] <inf> golioth_samples: Waiting to obtain IP address
[00:00:04.811,828] <inf> lte_monitor: Network: Searching
[00:00:10.509,307] <inf> lte_monitor: Network: Registered (roaming)
[00:00:10.510,070] <inf> golioth_mbox: Mbox created, bufsize: 1232, num_items: 10, item_size: 112
[00:00:12.051,727] <inf> golioth_coap_client_zephyr: Golioth CoAP client connected
[00:00:12.051,788] <inf> golioth_stream: Sending temperature 20.000000 (async)
[00:00:12.052,825] <inf> golioth_stream: Golioth client connected
[00:00:12.052,825] <inf> golioth_coap_client_zephyr: Entering CoAP I/O loop
[00:00:12.437,438] <dbg> golioth_stream: temperature_async_push_handler: Temperature data successfully pushed
[00:00:22.052,856] <inf> golioth_stream: Sending temperature 20.500000 (async)
[00:00:22.437,713] <dbg> golioth_stream: temperature_async_push_handler: Temperature data successfully pushed
[00:00:32.053,100] <inf> golioth_stream: Sending temperature 21.000000 (async)
[00:00:32.447,967] <dbg> golioth_stream: temperature_async_push_handler: Temperature data successfully pushed
[00:00:42.053,344] <inf> golioth_stream: Sending temperature 21.500000 (async)
[00:00:42.459,228] <dbg> golioth_stream: temperature_async_push_handler: Temperature data successfully pushed
[00:00:52.053,588] <inf> golioth_stream: Sending temperature 22.000000 (async)
[00:00:52.461,547] <dbg> golioth_stream: temperature_async_push_handler: Temperature data successfully pushed
```

### Credit 
Golioth IoT