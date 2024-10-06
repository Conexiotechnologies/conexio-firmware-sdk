
# Connecting Stratus to Datacake via MQTT
This MQTT sample demonstrates how to easily connect the Conexio Stratus device to the Datacake MQTT broker, send, and receive data.

### [📣 Find our walk-through tutorial for this sample application → here](https://www.rajeevpiyare.com/posts/stratus-to-datacake/)

## Requirements

The sample supports the Conexio Stratus development kit.


## Overview
The sample connects to the Datacake MQTT broker and publishes the data it receives on the configured subscribe topic to the 
configured publish topic. On a button press event, the application publishes the device vitals to the Datacake and 
periodically publishes the environmental data such as temperature and humidity.
By default, the sample can establish a secure (TLS) connection or a non-secure connection to the 
configured MQTT broker. The sample disables power saving modes (PSM and eDRX) so that network events are processed 
as soon as possible.

## Configuration
See the project `Kconfig` for different application configurations.

Add the Datacake credentials to the Application Code by editing the `prf.conf`.

```
# MQTT application configuration authentication
CONFIG_MQTT_PASS="DATACAKE_ACCESS_TOKEN"
CONFIG_MQTT_USER="DATACAKE_ACCESS_TOKEN"
```

This configuration parameter sets the MQTT broker hostname and port.
```
# MQTT broker configuration
CONFIG_MQTT_BROKER_HOSTNAME="mqtt.datacake.co"
CONFIG_MQTT_BROKER_PORT=8883
```

This configuration parameter sets the MQTT Publish and Subscribe topics.
```
# MQTT topics for recording measurement values
# Change this as per your Datacake MQTT Integration and fields
CONFIG_MQTT_PUB_TOPIC_TEMP="dtck-pub/<product_slug>/<device_id>/TEMPERATURE"
CONFIG_MQTT_PUB_TOPIC_HUM="dtck-pub/<product_slug>/<device_id>/HUMIDITY"
CONFIG_MQTT_PUB_TOPIC_VER="dtck-pub/<product_slug>/<device_id>/VERSION"
CONFIG_MQTT_PUB_TOPIC_IMEI="dtck-pub/<product_slug>/<device_id>/IMEI"
CONFIG_MQTT_PUB_TOPIC_BAT="dtck-pub/<product_slug>/<device_id>/BATTERY"
CONFIG_MQTT_PUB_TOPIC_RSRP="dtck-pub/<product_slug>/<device_id>/RSRP"
CONFIG_MQTT_PUB_TOPIC_BUTTON="dtck-pub/<product_slug>/<device_id>/BUTTON"

# MQTT subscription topics
CONFIG_MQTT_SUB_TOPIC="dtck/<product_slug>/<device_id>/+"
```

**Note**

The CA certificate for the default MQTT broker is included in the project and automatically 
provisioned after boot if the sample is built with the TLS configuration.

## Building and running
To compile the application, open a terminal window in the application directory and issue the following `west` command.

For the Stratus Pro nrf9151/nRF9161 device, the west build command is: 
```
west build -b conexio_stratus_pro/nrf9161/ns -- -DBOARD_ROOT=/opt/nordic/ncs/v2.7.0/conexio-firmware-sdk
```

For the Stratus nRF9160 device, the build command is:
```
west build -b conexio_stratus/nrf9160/ns -- -DBOARD_ROOT=/opt/nordic/ncs/v2.7.0/conexio-firmware-sdk
```

Once the application is compiled successfully, connect the Stratus device and put it into the DFU mode.

Flash the compiled firmware using `newtmgr`
```
newtmgr -c serial image upload build/zephyr/app_update.bin
```

Open up a serial console and reset the Stratus device. 

## Sample output
The following serial UART output will be displayed in the terminal. 
If you are connecting your Stratus device for the first time, give it a few minutes to register to the network and establish 
the LTE connection with the tower.

```
[2021-11-21 16:31:38] *** Booting nRF Connect SDK v2.5.0  ***
[2021-11-21 16:31:39] [00:00:00.213,592] <inf> mqtt_app: Stratus MQTT Datacake sample started, version: v1.0.0
[2021-11-21 16:31:39] [00:00:00.213,623] <inf> watchdog: Watchdog timeout installed. Timeout: 60000
[2021-11-21 16:31:39] [00:00:00.213,653] <inf> watchdog: Watchdog started
[2021-11-21 16:31:39] [00:00:00.213,653] <dbg> watchdog.watchdog_feed_enable: Watchdog feed enabled. Timeout: 30000
[2021-11-21 16:31:39] [00:00:00.213,684] <inf> mqtt_app: Provisioning certificates
[2021-11-21 16:31:39] [00:00:00.398,468] <inf> mqtt_app: Disabling PSM and eDRX
[2021-11-21 16:31:39] [00:00:00.399,353] <inf> mqtt_app: LTE Link Connecting...
[2021-11-21 16:31:40] +CEREG: 2,"412D","03382810",7
[2021-11-21 16:31:40] +CSCON: 1
[2021-11-21 16:31:41] +CEREG: 5,"412D","03382810",7,,,"11100000","11100000"
[2021-11-21 16:31:41] %CESQ: 35,1,10,1
[2021-11-21 16:31:42] [00:00:03.091,156] <inf> mqtt_app: LTE Link Connected!
[2021-11-21 16:31:42] [00:00:03.123,840] <dbg> mqtt_app.modem_rsrp_handler: Incoming RSRP status message, RSRP value is 35
[2021-11-21 16:31:42] [00:00:03.368,347] <inf> mqtt_app: IPv4 Address found 144.126.245.197
[2021-11-21 16:31:42] [00:00:03.369,140] <inf> mqtt_app: client_id: 352656103852334
[2021-11-21 16:31:42]
[2021-11-21 16:31:42] [00:00:03.369,140] <inf> mqtt_app: TLS enabled
[2021-11-21 16:31:42] [00:00:03.369,201] <inf> env_sensors: Environmental sensors initialized
[2021-11-21 16:31:46] [00:00:06.870,300] <inf> mqtt_app: MQTT client connected
[2021-11-21 16:31:46] [00:00:06.870,330] <inf> mqtt_app: Subscribing to: dtck/conexio-stratus/4fbfe839-c8aa-4882-a890-02f981753f6d/+ len 59
[2021-11-21 16:31:46] [00:00:07.249,328] <inf> mqtt_app: SUBACK packet id: 1234
[2021-11-21 16:31:47] [00:00:08.378,997] <inf> mqtt_app: Publishing: 26.06
[2021-11-21 16:31:47] [00:00:08.379,028] <inf> mqtt_app: to topic: dtck-pub/conexio-stratus/4fbfe839-c8aa-4882-a890-02f981753f6d/TEMPERATURE len: 73
[2021-11-21 16:31:47] [00:00:08.388,702] <inf> mqtt_app: Publishing: 23.24
[2021-11-21 16:31:47] [00:00:08.388,702] <inf> mqtt_app: to topic: dtck-pub/conexio-stratus/4fbfe839-c8aa-4882-a890-02f981753f6d/HUMIDITY len: 70
[2021-11-21 16:31:47] [00:00:08.910,614] <inf> mqtt_app: PUBACK packet id: 43056
[2021-11-21 16:31:47] [00:00:09.183,288] <inf> mqtt_app: PUBACK packet id: 62515
[2021-11-21 16:31:47] [00:00:09.205,596] <inf> mqtt_app: MQTT PUBLISH result=0 len=5
[2021-11-21 16:31:47] [00:00:09.206,390] <inf> mqtt_app: Received: 26.06
[2021-11-21 16:31:47] [00:00:09.252,532] <inf> mqtt_app: MQTT PUBLISH result=0 len=5
[2021-11-21 16:31:47] [00:00:09.253,234] <inf> mqtt_app: Received: 23.24
[2021-11-21 16:31:56] +CSCON: 0
[2021-11-21 16:31:56] %CESQ: 32,1,1,0
[2021-11-21 16:31:56] +CEREG: 5,"412D","03392510",7,,,"11100000","11100000"
[2021-11-21 16:31:57] [00:00:18.281,677] <dbg> mqtt_app.modem_rsrp_handler: Incoming RSRP status message, RSRP value is 32
[2021-11-21 16:32:01] +CSCON: 1
```

You will notice that the Stratus connects to the Datacake MQTT broker after which it publishes the sensor data to the configured topics. 
