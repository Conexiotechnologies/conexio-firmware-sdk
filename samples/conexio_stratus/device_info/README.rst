
Fetch Modem and Device Information
#############################

Overview
********

This sample application demonstrates how to fetch some important Modem related information
such as last reset reason or RSRP - Reference Signal Received Power from the Stratus devices.

Building and Running
********************

See the `device documentation <https://docs.conexiotech.com/>`_ for detailed instructions on how to get started.

For the Stratus Pro nrf9151/nRF9161 device, the `west` build command is: 

   west build -b conexio_stratus_pro/nrf9161/ns -- -DBOARD_ROOT=/opt/nordic/ncs/v2.7.0/conexio-firmware-sdk

For the Stratus nRF9160 device, the build command is:

   west build -b conexio_stratus/nrf9160/ns -- -DBOARD_ROOT=/opt/nordic/ncs/v2.7.0/conexio-firmware-sdk


Flash the compiled firmware using `newtmgr`

   newtmgr -c serial image upload build/zephyr/app_update.bin
