.. _sht4x:

SHT4X: Humidity and temperature sensing with Power Gating
####################################################################################

Description
***********

This sample application periodically measures the ambient temperature and humidity
from an SHT4X sensor connected to the Conexio Stratus kit. The result is written to the console.

The SHT4X has the option to use a heater which makes sense for specific
environments/applications (refer to the datasheet for more information).
To make use of the heater have a look at the Kconfig options for this application.

In addition, to save power, the sensor is power gated when not in use and activated just before sampling and then
turned off again. GPIO PIN 26 is connected to the power port of the sensors which is used to turn logic high or low
to power gate the sensors onboard Stratus Pro.

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
   [00:00:00.256,408] <inf> lis2dh: fs=2, odr=0x4 lp_en=0x0 scale=9576
   *** Booting nRF Connect SDK v2.7.0-5cb85570ca43 ***
   *** Using Zephyr OS v3.6.99-100befc70c74 ***
   SHT4X: Temp: 24.738307 degC; RH: 55.985946 %
   SHT4X: Temp: 24.716945 degC; RH: 55.997390 %
   SHT4X: Temp: 24.711604 degC; RH: 55.943984 %
   SHT4X: Temp: 24.700923 degC; RH: 56.594406 %
   SHT4X: Temp: 24.700923 degC; RH: 56.499038 %
   SHT4X: Temp: 24.692912 degC; RH: 56.211029 %
   SHT4X: Temp: 24.674219 degC; RH: 56.071792 %
   SHT4X: Temp: 24.663538 degC; RH: 55.968780 %

   <repeats endlessly every 3 seconds>

Credit 
===============
Zephyr Project