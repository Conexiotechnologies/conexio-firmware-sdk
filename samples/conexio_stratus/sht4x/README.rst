.. _sht4x:

SHT4X: High accuracy digital I2C humidity and temperature sensor
####################################################################################

Description
***********

This sample application periodically measures the ambient temperature and humidity
from an SHT4X sensor onboard Conexio Stratus kit. The result is written to the console.

The SHT4X has the option to use a heater which makes sense for specific
environments/applications (refer to the datasheet for more information).
To make use of the heater have a look at the Kconfig options for this application.


References
**********

 - `SHT4X sensor <https://www.sensirion.com/en/environmental-sensors/humidity-sensors/humidity-sensor-sht4x/>`_

Wiring
******

This sample uses the SHT4X sensor controlled using the I2C interface of the Conexio Stratus shield.

Building and Running
********************

This project outputs sensor data to the console. It should work with any platform featuring a I2C peripheral
interface.

   west build -b conexio_stratus_ns

Flash the compiled firmware using `newtmgr`

   newtmgr -c serial image upload build/zephyr/app_update.bin


Sample Output
=============

.. code-block:: console

        Conexio Stratus SHT4X sensor example
        SHT4X: 23.64 Temp. [C] ; 30.74 RH [%]
        SHT4X: 23.66 Temp. [C] ; 32.16 RH [%]
        SHT4X: 23.63 Temp. [C] ; 30.83 RH [%]

      <repeats endlessly every 2 seconds>
