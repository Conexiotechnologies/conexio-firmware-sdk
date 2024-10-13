
DPS310 Temperature and Pressure Sensor
######################################

Overview
********

This sample application periodically reads temperature and pressure data from
the DPS310 sensor that implements SENSOR_CHAN_AMBIENT_TEMP and
SENSOR_CHAN_PRESS. This sample checks the sensor in polling mode (without
interrupt trigger).

Building and Running
********************

This sample application uses an DPS310 sensor connected to the Stratus board via QWIIC connector.

See the `device documentation <https://docs.conexiotech.com/>`_ for detailed instructions on how to get started.

For the Stratus Pro nrf9151/nRF9161 device, the `west` build command is: 

   west build -b conexio_stratus_pro/nrf9161/ns -- -DBOARD_ROOT=/opt/nordic/ncs/v2.7.0/conexio-firmware-sdk

For the Stratus nRF9160 device, the build command is:

   west build -b conexio_stratus/nrf9160/ns -- -DBOARD_ROOT=/opt/nordic/ncs/v2.7.0/conexio-firmware-sdk


Flash the compiled firmware using `newtmgr`

   newtmgr -c serial image upload build/zephyr/app_update.bin


Sample Output
=============
To check output of this sample, any serial console program can be used.
This example uses ``picocom`` on the serial port ``/dev/tty.SLAB_USBtoUART``:

.. code-block:: console

   $ picocom -b 115200 /dev/tty.SLAB_USBtoUART

.. code-block:: console

   *** Booting nRF Connect SDK v3.5.99-ncs1  ***
   Conexio Stratus DPS310 sensor example
   
   Temp: 23.774363 degC; Presssure: 97.354728 kPa
   Temp: 23.777492 degC; Presssure: 97.353904 kPa
   Temp: 23.784646 degC; Presssure: 97.354064 kPa

Credit 
===============
Zephyr Project
