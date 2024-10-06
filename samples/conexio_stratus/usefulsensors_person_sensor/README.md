# Conexio Stratus + Useful Sensors Person Sensor

This repository contains example code for communicating with the Useful Sensors Person Sensor over I2C
for face detection.

Wiring Information
=============
Connect the sensor breakout board to the Conexio Stratus kit using Qwiic connector.

Building and Running on conexio_stratus
===========================
For the Stratus Pro nrf9151/nRF9161 device, the west build command is: 
```
west build -b conexio_stratus_pro/nrf9161/ns -- -DBOARD_ROOT=/opt/nordic/ncs/v2.7.0/conexio-firmware-sdk
```
For the Stratus nRF9160 device, the build command is:
```
west build -b conexio_stratus/nrf9160/ns -- -DBOARD_ROOT=/opt/nordic/ncs/v2.7.0/conexio-firmware-sdk
```

Flash the compiled firmware using `newtmgr`:
```
newtmgr -c serial image upload build/zephyr/app_update.bin
```

Sample Output
=============
```
*** Booting nRF Connect SDK v3.5.99-ncs1  ***
[00:00:00.205,596] <inf> app: Usefulsensors person sensor sample started on conexio_stratus_pro
[00:00:00.205,627] <inf> app: Setting up i2c
[00:00:00.205,627] <inf> app: I2C device OK
[00:00:03.209,808] <inf> app: ****************
[00:00:03.209,808] <inf> app: 1 face(s) found
[00:00:03.209,838] <inf> app: Face #0: 69 confidence, (100, 56), (126, 108), not facing
[00:00:05.211,944] <inf> app: ****************
[00:00:05.211,944] <inf> app: 1 face(s) found
[00:00:05.211,975] <inf> app: Face #0: 94 confidence, (49, 19), (101, 99), facing
[00:00:06.213,012] <inf> app: ****************
[00:00:06.213,043] <inf> app: 1 face(s) found
[00:00:06.213,043] <inf> app: Face #0: 69 confidence, (98, 58), (128, 106), facing
```
