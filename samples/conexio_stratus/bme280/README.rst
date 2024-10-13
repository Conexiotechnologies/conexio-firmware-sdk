
BME280 MIKROE-1978 Click board Humidity and Pressure Sensor
############################################################

Overview
********

This sample shows how to use the Zephyr :ref:`sensor_api` API driver for the
`Bosch BME280`_ environmental sensor.

The sample periodically reads temperature, pressure and humidity data from the
BME280 sensor connected to the Conexio Stratus.

Building and Running
********************

The sample can be configured to support BME280 sensors connected via I2C.

See the `device documentation <https://docs.conexiotech.com/>`_ for detailed instructions on how to get started.

For the Stratus Pro nrf9151/nRF9161 device, the `west` build command is: 

   west build -b conexio_stratus_pro/nrf9161/ns -- -DBOARD_ROOT=/opt/nordic/ncs/v2.7.0/conexio-firmware-sdk

For the Stratus nRF9160 device, the build command is:

   west build -b conexio_stratus/nrf9160/ns -- -DBOARD_ROOT=/opt/nordic/ncs/v2.7.0/conexio-firmware-sdk


Flash the compiled firmware using `newtmgr`

   newtmgr -c serial image upload build/zephyr/app_update.bin


Sample Output
=============

The sample prints output to the serial console. BME280 device driver messages
are also logged. 

Here is example output for the default application settings, assuming that 
BME280 sensor is connected to the I2C pins:

.. code-block:: none

   *** Booting nRF Connect SDK v3.5.99-ncs1  ***
   Conexio Stratus BME280 sensor example
   Found device BME280_I2C, getting sensor data
   temp: 20.150000; press: 99.857675; humidity: 46.447265
   temp: 20.150000; press: 99.859121; humidity: 46.458984
   temp: 20.150000; press: 99.859234; humidity: 46.469726

Credit 
===============
Zephyr Project
