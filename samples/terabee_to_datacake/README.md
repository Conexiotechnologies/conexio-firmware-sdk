
# Sending TeraBee Level Data to Datacake via MQTT
This MQTT sample demonstrates how to easily connect the Conexio Stratus device to the Datacake MQTT broker, send, and receive data.

## Requirements

The sample supports the Conexio Stratus development kit and Terabee level sensor.


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
CONFIG_MQTT_PUB_TOPIC_DISTANCE="dtck-pub/<product_slug>/<device_id>/DISTANCE"

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





