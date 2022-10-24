# Conexio Stratus + Terabee EVO ToF Sensor

This repository contains example firmware for communicating with the Terabee EVO ToF Sensor.

## Connection Pinouts

|Stratus  |Terabee    |
|---------|-----------|
|5V       |VDD        |
|GND      |GND        |
|SDA      |SDA        |
|SCL      |SCL        |


Building and Running on conexio_stratus
===========================
To compile te application, execute:
```
west build -b conexio_stratus_ns
```

Flash the compiled firmware using `newtmgr`:
```
newtmgr -c serial image upload build/zephyr/app_update.bin
```


Sample Output
=============
```
*** Booting Zephyr OS build v2.6.99-ncs1  ***
Conexio Stratus Terabee sample started
Distance: 1685 mm
Distance: 1695 mm
```
