.. _bme280:

BME280 MIKROE-1978 Click board Humidity and Pressure Sensor
############################################################

Overview
********

This sample shows how to use the Zephyr :ref:`sensor_api` API driver for the
`Bosch BME280`_ environmental sensor.

.. _Bosch BME280:
   https://www.bosch-sensortec.com/products/environmental-sensors/humidity-sensors-bme280/`

The sample periodically reads temperature, pressure and humidity data from the
BME280 MIKROE-1978 Click board connected to the Conexio Stratus shield Mikro BUS. The sample checks the
sensor in polling mode (without interrupt trigger).

Building and Running
********************

The sample can be configured to support BME280 sensors connected via either I2C
or SPI. Configuration is done via :ref:`devicetree <dt-guide>`. The devicetree
must have an enabled node with ``compatible = "bosch,bme280";``. See
:dtcompatible:`bosch,bme280` for the devicetree binding and see below for
examples and common configurations.

If the sensor is not built into your board, start by wiring the sensor pins
according to the connection diagram given in the `BME280 datasheet`_ at
page 38.

.. _BME280 datasheet:
   https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bme280-ds002.pdf

Conexio Stratus Shield with a MIKROE-1978 Click board
=============================
   west build -b conexio_stratus_ns    


Sample Output
=============

The sample prints output to the serial console. BME280 device driver messages
are also logged. 

Here is example output for the default application settings, assuming that 
BME280 sensor is connected to the standard Mikro BUS I2C pins:

.. code-block:: none

   *** Booting Zephyr OS build zephyr-v2.6.0-2940-gbb732ada394f  ***
   Conexio Stratus BME280 MIKROE-1978 Click board sensor example
   Found device BME280_I2C, getting sensor data
   temp: 20.150000; press: 99.857675; humidity: 46.447265
   temp: 20.150000; press: 99.859121; humidity: 46.458984
   temp: 20.150000; press: 99.859234; humidity: 46.469726

