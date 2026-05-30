.. _bme680:

BME680: Integrated environmental sensor
#####################################################

Description
***********

This sample application periodically (every 3s) measures the ambient temperature
in degrees Celsius, atmospheric pressure in kilopascal, relative humidity in percentage,
and gas sensor resistance in ohms. The result is written to the console.

References
**********

 - BME680: https://www.bosch-sensortec.com/bst/products/all_products/bme680

Wiring
*******

This sample uses the BME680 sensor controlled using the I2C interface.
Connect Supply: **VDD**, **VDDIO**, **GND** and Interface: **SDA**, **SCL**.
The supply voltage can be in the 1.8V to 3.6V range.
Depending on the baseboard used, the **SDA** and **SCL** lines require Pull-Up
resistors.

Building and Running
********************

This project outputs sensor data to the console. It requires a BME680
sensor. It should work with any platform featuring a I2C peripheral interface.

See the `device documentation <https://docs.conexiotech.com/>`_ for detailed instructions on how to get started.

For the Stratus Pro nrf9151/nRF9161 device, the `west` build command is: 

   west build -b conexio_stratus_pro/nrf9161/ns -- -DBOARD_ROOT=/opt/nordic/ncs/v3.2.1/conexio-firmware-sdk

For the Stratus nRF9160 device, the build command is:

   west build -b conexio_stratus/nrf9160/ns -- -DBOARD_ROOT=/opt/nordic/ncs/v3.2.1/conexio-firmware-sdk


Flash the compiled firmware using `newtmgr`

   newtmgr -c serial image upload build/bme680/zephyr/zephyr.signed.bin

Sample Output
=============

.. code-block:: console

   Device 0x20002b74 name is BME680
   T: 23.988877; P: 97.648568; H: 53.689533; G: 1035.211466
   T: 24.168500; P: 97.648866; H: 53.565966; G: 1046.677896
   T: 24.336533; P: 97.648596; H: 53.353663; G: 1058.656533
   T: 24.589696; P: 97.648366; H: 53.958864; G: 1072.155863
   T: 24.856631; P: 97.648322; H: 53.553669; G: 1096.448788

   <repeats endlessly>
