
SHT3XD: High accuracy digital I2C humidity and temperature sensor
#################################################

Description
***********

This sample application periodically (2 Hz) measures the ambient
temperature and humidity. The result is written to the console.

Wiring
*******

This sample uses the SHT3X_DIS sensor controlled using the I2C interface of the Conexio Stratus device.
Connect Supply: **VDD**, **GND** and Interface: **SDA**, **SCL**.

Building and Running
********************

This project outputs sensor data to the console. It requires a SHT3XD
sensor. It should work with any platform featuring an I2C peripheral
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

   SHT3XD sensor sample running on conexio_stratus_pro
   SHT3XD: 19.64 °C ; 67.35 °F; 41.96 %RH
   SHT3XD: 19.74 °C ; 67.53 °F; 42.06 %RH
   SHT3XD: 19.75 °C ; 67.55 °F; 42.08 %RH

<repeats endlessly>

Credit 
===============
Zephyr Project
