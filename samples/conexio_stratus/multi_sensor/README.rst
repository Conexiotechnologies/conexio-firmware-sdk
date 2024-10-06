
Multi-sensor Application
#############################

Overview
********

This sample application periodically blinks the LED, reads the accelerometer data from the
LIS2DH sensor onboard Conexio Stratus, if SHT4x sensor is connected, reads and displays the sensor data on the console.

Configuration
********************
To enable LISDH accelerometer sensor, set 

  CONFIG_LIS2DH=y

To enable SHT4x sensor, set 

  CONFIG_SHT4X=y

  CONFIG_PM_DEVICE=y


Building and Running
********************

See the `device documentation <https://docs.conexiotech.com/>`_ for detailed instructions on how to get started.


For the Stratus Pro nrf9151/nRF9161 device, the `west` build command is: 

   west build -b conexio_stratus_pro/nrf9161/ns -- -DBOARD_ROOT=/opt/nordic/ncs/v2.7.0/conexio-firmware-sdk

For the Stratus nRF9160 device, the build command is:

   west build -b conexio_stratus/nrf9160/ns -- -DBOARD_ROOT=/opt/nordic/ncs/v2.7.0/conexio-firmware-sdk


Flash the compiled firmware using `newtmgr`

   newtmgr -c serial image upload build/zephyr/app_update.bin
