.. _sht4x:

SHT4X: High accuracy digital I2C humidity and temperature sensor
####################################################################################

Description
***********

This sample application periodically measures the ambient temperature and humidity
from an SHT4X sensor connected to  Conexio Stratus kit. The result is written to the console.

The SHT4X has the option to use a heater which makes sense for specific
environments/applications (refer to the datasheet for more information).
To make use of the heater have a look at the Kconfig options for this application.


References
**********

 - `SHT4X sensor <https://www.sensirion.com/en/environmental-sensors/humidity-sensors/humidity-sensor-sht4x/>`_

Wiring
******

This sample uses the SHT4X sensor controlled using the I2C interface of the Conexio Stratus.

Building and Running
********************

This project outputs sensor data to the console. It should work with any platform featuring a I2C peripheral
interface.

See the `device documentation <https://docs.conexiotech.com/>`_ for detailed instructions on how to get started.

For the Stratus Pro nrf9151/nRF9161 device, the `west` build command is: 

   west build -b conexio_stratus_pro/nrf9161/ns -- -DBOARD_ROOT=/opt/nordic/ncs/v2.7.0/conexio-firmware-sdk

For the Stratus nRF9160 device, the build command is:

   west build -b conexio_stratus/nrf9160/ns -- -DBOARD_ROOT=/opt/nordic/ncs/v2.7.0/conexio-firmware-sdk


Flash the compiled firmware using `newtmgr`

   newtmgr -c serial image upload build/zephyr/app_update.bin


Sample Output
=============

.. code-block:: console

        SHT4X sensor sample running on conexio_stratus_pro
        SHT4X: 23.64 Temp. [C] ; 30.74 RH [%]
        SHT4X: 23.66 Temp. [C] ; 32.16 RH [%]
        SHT4X: 23.63 Temp. [C] ; 30.83 RH [%]

      <repeats endlessly every 2 seconds>

Credit 
===============
Zephyr Project