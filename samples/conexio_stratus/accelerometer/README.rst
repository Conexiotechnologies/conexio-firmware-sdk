.. _lis2dh:

LIS2DH: Motion Sensor Monitor
#############################

Overview
********

This sample application periodically reads accelerometer data from the
LIS2DH sensor onboard Conexio Stratus and displays the sensor data on the console.

Requirements
************

This sample uses the ST Microelectronics LIS2DH MEMS digital output motion sensor, an 
ultra-low-power high-performance 3-axis Femto accelerometer sensor.


References
**********

For more information about the LIS2DH motion sensor and its capabilities see
http://www.st.com/en/mems-and-sensors/lis2dh.html.

Building and Running
********************

See the `device documentation <https://docs.conexiotech.com/>`_ for detailed instructions on how to flash
and get access to the console where acceleration data is displayed.

Building on conexio_stratus
===========================

Conexio Stratus DK includes a ST LIS2DH12 accelerometer which
supports the LIS2DH interface.

For the Stratus Pro nrf9151/nRF9161 device, the west build command is: 

   west build -b conexio_stratus_pro/nrf9161/ns -- -DBOARD_ROOT=/opt/nordic/ncs/v2.7.0/conexio-firmware-sdk

For the Stratus nRF9160 device, the build command is:

   west build -b conexio_stratus/nrf9160/ns -- -DBOARD_ROOT=/opt/nordic/ncs/v2.7.0/conexio-firmware-sdk


Flash the compiled firmware using `newtmgr`

   newtmgr -c serial image upload build/zephyr/app_update.bin

Sample Output
=============

.. code-block:: console
    
    Conexio Stratus accelerometer sensor example
    Polling at 0.5 Hz
    #1 @ 12 ms: x -5.387328 , y 5.578368 , z -5.463744
    #2 @ 2017 ms: x -5.310912 , y 5.654784 , z -5.501952
    #3 @ 4022 ms: x -5.349120 , y 5.692992 , z -5.463744

   <repeats endlessly every 2 seconds>


Credit 
===============
Zephyr Project
