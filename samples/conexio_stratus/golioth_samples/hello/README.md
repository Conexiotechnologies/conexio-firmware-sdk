# Stratus Pro - Golioth Hello sample

## Overview

This sample application demonstrates how to connect with Golioth and
publish simple Hello messages.

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

To build the application, see the [Conexio device documentation](https://docs.conexiotech.com/master/building-and-programming-an-application/compiling-applications-with-nrf-connect-extension-for-vs-code) 
for detailed instructions on how to build and flash the application to the Stratus devices.

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
*** Booting Zephyr OS build zephyr-v3.4.0-553-g40d224022608 ***
[00:00:00.020,000] <inf> net_config: Initializing network
[00:00:00.020,000] <inf> net_config: IPv4 address: 192.0.2.1
[00:00:00.020,000] <dbg> hello_zephyr: main: start hello sample
[00:00:00.020,000] <inf> golioth_samples: Waiting for interface to be up
[00:00:00.020,000] <inf> golioth_mbox: Mbox created, bufsize: 1100, num_items: 10, item_size: 100
[00:00:00.070,000] <inf> golioth_coap_client: Start CoAP session with host: coaps://coap.golioth.io
[00:00:00.070,000] <inf> golioth_coap_client: Session PSK-ID: your-device-id@your-golioth-project
[00:00:00.070,000] <inf> golioth_coap_client: Entering CoAP I/O loop
[00:00:01.260,000] <inf> golioth_coap_client: Golioth CoAP client connected
[00:00:01.260,000] <inf> hello_zephyr: Sending hello! 0
[00:00:01.260,000] <inf> hello_zephyr: Golioth client connected
[00:00:06.270,000] <inf> hello_zephyr: Sending hello! 1
[00:00:11.280,000] <inf> hello_zephyr: Sending hello! 2
```
