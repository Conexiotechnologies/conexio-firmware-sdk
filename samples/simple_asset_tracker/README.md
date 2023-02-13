
# Conexio Stratus Simple Tracker

This sample tracker app demonstrates how to easily connect the Conexio Stratus device to the Datacake MQTT broker, sample and periodically send device location data.

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
CONFIG_MQTT_PUB_TOPIC_LOCATION="dtck-pub/<product_slug>/<device_id>/LOCATION"

# MQTT subscription topics
CONFIG_MQTT_SUB_TOPIC="dtck/<product_slug>/<device_id>/+"
```

**Note**

The CA certificate for the default MQTT broker is included in the project and automatically 
provisioned after boot if the sample is built with the TLS configuration.

## Building and running
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
2022-11-13 18:01:24 --> I: sensor-pwr-ctrl sync: 0
2022-11-13 18:01:24 --> I: fs=2, odr=0x4 lp_en=0x0 scale=9576
2022-11-13 18:01:24 --> *** Booting Zephyr OS build v3.1.99-ncs1  ***
2022-11-13 18:01:24 --> I: Setup AIN7 got 0
2022-11-13 18:01:24 --> I: Battery setup: 0 1
2022-11-13 18:01:24 --> I: Stratus MQTT Datacake sample started, version: v1.0.3
2022-11-13 18:01:24 --> I: Provisioning certificates
2022-11-13 18:01:24 --> I: Connecting to LTE network
2022-11-13 18:01:25 --> I: RRC mode: Connected
2022-11-13 18:01:37 --> I: Network registration status: Connected - roaming
2022-11-13 18:01:37 --> I: CoI: PSM parameter update: Periodic TAU: 3600 s, Active time: 0 s
2022-11-13 18:01:37 --> I: eDRX parameter update: eDRX: 163.839996, PTW: 2.560000
2022-11-13 18:01:37 --> nnected to LTE network
2022-11-13 18:01:37 --> D: Incoming RSRP status message, RSRP value is 47
2022-11-13 18:01:37 --> I: IPv4 Address found 159.89.214.202
2022-11-13 18:01:37 --> D: client_id = 
2022-11-13 18:01:37 --> I: TLS enabled
2022-11-13 18:01:37 --> I: Starting GNSS
2022-11-13 18:01:38 --> I: Searching. Current satellites: 0
2022-11-13 18:01:38 --> I: GNSS blocked by LTE activity
2022-11-13 18:01:39 --> I: Searching. Current satellites: 0
2022-11-13 18:01:39 --> I: GNSS blocked by LTE activity
2022-11-13 18:01:40 --> I: Searching. Current satellites: 0
2022-11-13 18:01:40 --> I: GNSS blocked by LTE activity
2022-11-13 18:01:40 --> I: MQTT client connected
2022-11-13 18:01:40 --> I: Subscribing to: dtck/conexio-stratus/4fbfe839-c8aa-4882-a890-02f981753f6d/+ len 59
2022-11-13 18:01:40 --> I: SUBACK packet id: 1234
2022-11-13 18:01:41 --> I: Searching. Current satellites: 0
2022-11-13 18:01:41 --> I: GNSS blocked by LTE activity
2022-11-13 18:01:42 --> I: Searching. Current satellites: 0
2022-11-13 18:01:42 --> I: GNSS blocked by LTE activity
2022-11-13 18:01:43 --> I: Searching. Current satellites: 0
2022-11-13 18:01:43 --> I: GNSS blocked by LTE activity
2022-11-13 18:01:44 --> I: Searching. Current satellites: 0
2022-11-13 18:01:44 --> I: GNSS blocked by LTE activity
2022-11-13 18:01:45 --> I: Searching. Current satellites: 0
2022-11-13 18:01:45 --> I: GNSS blocked by LTE activity
2022-11-13 18:01:46 --> I: Searching. Current satellites: 0
2022-11-13 18:01:46 --> I: GNSS blocked by LTE activity
2022-11-13 18:01:46 --> I: RRC mode: Idle
2022-11-13 18:01:46 --> I: Searching. Current satellites: 0
2022-11-13 18:01:47 --> I: Searching. Current satellites: 0
2022-11-13 18:01:48 --> I: Searching. Current satellites: 2
2022-11-13 18:01:49 --> I: Searching. Current satellites: 2
2022-11-13 18:01:50 --> I: Searching. Current satellites: 2
2022-11-13 18:01:51 --> I: Searching. Current satellites: 2
2022-11-13 18:01:52 --> I: Searching. Current satellites: 2
2022-11-13 18:01:53 --> I: Searching. Current satellites: 2
2022-11-13 18:01:54 --> I: Searching. Current satellites: 3
2022-11-13 18:01:55 --> I: Searching. Current satellites: 3
2022-11-13 18:01:56 --> I: Searching. Current satellites: 3
2022-11-13 18:01:57 --> I: Searching. Current satellites: 3
2022-11-13 18:01:58 --> I: Searching. Current satellites: 3
2022-11-13 18:01:59 --> I: Searching. Current satellites: 3
2022-11-13 18:02:00 --> I: Searching. Current satellites: 3
2022-11-13 18:02:01 --> I: Searching. Current satellites: 3
2022-11-13 18:02:02 --> I: Searching. Current satellites: 3
2022-11-13 18:02:03 --> I: Searching. Current satellites: 3
2022-11-13 18:02:04 --> I: Searching. Current satellites: 4
2022-11-13 18:02:05 --> I: Searching. Current satellites: 6
2022-11-13 18:02:06 --> I: Searching. Current satellites: 6
2022-11-13 18:02:07 --> I: Searching. Current satellites: 5
2022-11-13 18:02:08 --> I: Searching. Current satellites: 6
2022-11-13 18:02:09 --> I: Searching. Current satellites: 5
2022-11-13 18:02:10 --> I: Searching. Current satellites: 5
2022-11-13 18:02:11 --> I: Searching. Current satellites: 6
2022-11-13 18:02:12 --> I: Searching. Current satellites: 6
2022-11-13 18:02:13 --> I: Searching. Current satellites: 6
2022-11-13 18:02:14 --> I: Searching. Current satellites: 5
2022-11-13 18:02:15 --> I: Searching. Current satellites: 4
2022-11-13 18:02:16 --> I: Searching. Current satellites: 4
2022-11-13 18:02:17 --> I: Searching. Current satellites: 4
2022-11-13 18:02:18 --> I: Searching. Current satellites: 4
2022-11-13 18:02:19 --> I: Searching. Current satellites: 5
2022-11-13 18:02:20 --> I: Searching. Current satellites: 5
2022-11-13 18:02:21 --> I: Searching. Current satellites: 5
2022-11-13 18:02:22 --> I: Searching. Current satellites: 5
2022-11-13 18:02:23 --> I: Searching. Current satellites: 4
2022-11-13 18:02:24 --> I: Searching. Current satellites: 4
2022-11-13 18:02:25 --> I: Searching. Current satellites: 4
2022-11-13 18:02:26 --> I: Searching. Current satellites: 4
2022-11-13 18:02:27 --> I: Searching. Current satellites: 5
2022-11-13 18:02:28 --> I: Searching. Current satellites: 7
2022-11-13 18:02:29 --> I: Searching. Current satellites: 7
2022-11-13 18:02:30 --> I: Searching. Current satellites: 7
2022-11-13 18:02:31 --> I: Searching. Current satellites: 8
2022-11-13 18:02:32 --> I: Searching. Current satellites: 7
2022-11-13 18:02:33 --> I: Searching. Current satellites: 6
2022-11-13 18:02:34 --> I: Searching. Current satellites: 6
2022-11-13 18:02:35 --> I: Searching. Current satellites: 5
2022-11-13 18:02:36 --> I: Searching. Current satellites: 5
2022-11-13 18:02:37 --> I: Searching. Current satellites: 6
2022-11-13 18:02:38 --> I: Searching. Current satellites: 7
2022-11-13 18:02:39 --> I: Searching. Current satellites: 7
2022-11-13 18:02:40 --> I: Searching. Current satellites: 6
2022-11-13 18:02:41 --> I: Searching. Current satellites: 5
2022-11-13 18:02:42 --> I: Searching. Current satellites: 7
2022-11-13 18:02:43 --> I: Searching. Current satellites: 7
2022-11-13 18:02:44 --> I: Searching. Current satellites: 8
2022-11-13 18:02:45 --> I: Searching. Current satellites: 8
2022-11-13 18:02:46 --> I: Searching. Current satellites: 5
2022-11-13 18:02:47 --> I: Searching. Current satellites: 6
2022-11-13 18:02:48 --> I: Searching. Current satellites: 6
2022-11-13 18:02:49 --> I: Searching. Current satellites: 5
2022-11-13 18:02:50 --> I: Searching. Current satellites: 5
2022-11-13 18:02:51 --> I: Searching. Current satellites: 6
2022-11-13 18:02:52 --> I: Searching. Current satellites: 7
2022-11-13 18:02:53 --> I: Searching. Current satellites: 7
2022-11-13 18:02:54 --> I: Searching. Current satellites: 6
2022-11-13 18:02:54 --> I: Latitude:       41.908028
2022-11-13 18:02:54 --> I: Longitude:      -87.629970
2022-11-13 18:02:54 --> I: Altitude:       569.3 m
2022-11-13 18:02:54 --> I: Time (UTC):     00:02:54.577
2022-11-13 18:02:54 --> I: Time to first fix: 77 s
2022-11-13 18:02:54 --> I: GNSS enter sleep after fix
2022-11-13 18:03:04 --> I: Publishing: v1.0.3
2022-11-13 18:03:04 --> I: to topic: dtck-pub/conexio-stratus/4fbfe839-c8aa-4882-a890-02f981753f6d/VERSION len: 69
2022-11-13 18:03:04 --> E: Publishing of app_version_info PASSED
2022-11-13 18:03:04 --> I: Publishing RSRP: -93
2022-11-13 18:03:04 --> I: Publishing: -93
2022-11-13 18:03:04 --> I: to topic: dtck-pub/conexio-stratus/4fbfe839-c8aa-4882-a890-02f981753f6d/RSRP len: 66
2022-11-13 18:03:04 --> I: Publishing: 
2022-11-13 18:03:04 --> I: to topic: dtck-pub/conexio-stratus/4fbfe839-c8aa-4882-a890-02f981753f6d/LOCATION len: 70
2022-11-13 18:03:04 --> I: Sending device vitals to cloud at intervals of 1800 sec
2022-11-13 18:03:04 --> I: raw 10115 ~ 2222 mV => 4444 mV
2022-11-13 18:03:04 --> I: Device battery: 4444 mV
2022-11-13 18:03:04 --> I: Publishing: 4444
2022-11-13 18:03:04 --> I: to topic: dtck-pub/conexio-stratus/4fbfe839-c8aa-4882-a890-02f981753f6d/BATTERY len: 69
2022-11-13 18:03:04 --> I: Publishing RSRP: -93
2022-11-13 18:03:04 --> I: Publishing: -93
2022-11-13 18:03:04 --> I: to topic: dtck-pub/conexio-stratus/4fbfe839-c8aa-4882-a890-02f981753f6d/RSRP len: 66
2022-11-13 18:03:04 --> I: Publishing: 
2022-11-13 18:03:04 --> I: to topic: dtck-pub/conexio-stratus/4fbfe839-c8aa-4882-a890-02f981753f6d/LOCATION len: 70
2022-11-13 18:03:04 --> D: Incoming 
2022-11-13 18:03:04 --> RSRP status message, RSRP value is 49
2022-11-13 18:03:05 --> I: RRC mode: Connected
2022-11-13 18:03:05 --> I: PUBACK packet id: 29258
2022-11-13 18:03:05 --> I: MQTT PUBLISH result=0 len=6
2022-11-13 18:03:05 --> I: Received: v1.0.3
2022-11-13 18:03:06 --> I: PUBACK packet id: 14085
2022-11-13 18:03:06 --> I: PUBACK packet id: 15158
2022-11-13 18:03:06 --> I: PUBACK packet id: 15728
2022-11-13 18:03:06 --> I: PUBACK packet id: 11874
2022-11-13 18:03:06 --> I: PUBACK packet id: 43111
2022-11-13 18:03:06 --> I: MQTT PUBLISH result=0 len=0
2022-11-13 18:03:06 --> I: Received: 
2022-11-13 18:03:06 --> I: MQTT PUBLISH result=0 len=6
2022-11-13 18:03:06 --> I: Received: 4444.0
2022-11-13 18:03:12 --> I: RRC mode: Idle
```