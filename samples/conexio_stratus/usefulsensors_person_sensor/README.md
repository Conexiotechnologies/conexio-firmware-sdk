# Conexio Stratus + Useful Sensors Person Sensor

This repository contains example code for communicating with the Useful Sensors Person Sensor over I2C
for face detection.

Wiring Information
=============
Connect the sensor breakout board to the Conexio Stratus kit using Qwiic connector.

Building and Running on conexio_stratus
===========================
To compile te application, execute:
For Conexio Stratus:
```
west build -b conexio_stratus_ns
```
For Conexio Stratus Pro:
```
west build -b conexio_stratus_pro_ns
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
