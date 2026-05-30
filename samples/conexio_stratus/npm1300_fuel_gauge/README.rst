.. _npm1300_fuel_gauge:

nPM1300: Fuel gauge and Battery charging
###################

#. The Fuel gauge sample demonstrates how to calculate the battery state of charge using the nPM1300 fuel gauge on 
Stratus Pro nrf9151/nRF9161 devices.

#. Disables the NTC on the nPM1300 PMIC as it is not connected on the Stratus Pro device. Not disabling the NTC
currently prevents battery charging over USB-C. 
There is a nPM1300 driver bug which prevents charging without NTC and is being addressed now in the upcoming 
`Zephyr pull request: <https://github.com/zephyrproject-rtos/zephyr/pull/74930#issuecomment-2408694307>`_ .

Requirements
************

The sample supports the following development kits:

* Conexio Stratus Pro nrf9151/nRF9161

Overview
********

This sample allows to calculate the state of charge, time to empty and time to full information from a battery connected to the
Stratus Pro kit.


Building and running
********************

For the Stratus Pro nRF9151 device, the west build command is:

   west build -b conexio_stratus_pro/nrf9151/ns -- -DBOARD_ROOT=/opt/nordic/ncs/v3.2.1/conexio-firmware-sdk

For the Stratus Pro nRF9161 device, the west build command is:

   west build -b conexio_stratus_pro/nrf9161/ns -- -DBOARD_ROOT=/opt/nordic/ncs/v3.2.1/conexio-firmware-sdk

For the Stratus nRF9160 device, the build command is:

   west build -b conexio_stratus/nrf9160/ns -- -DBOARD_ROOT=/opt/nordic/ncs/v3.2.1/conexio-firmware-sdk


Flash the compiled firmware using `newtmgr`

   newtmgr -c serial image upload build/npm1300_fuel_gauge/zephyr/zephyr.signed.bin

Testing
*******

#. Connect the Stratus kit
#. Start the serial terminal program

If the initialization was successful, the terminal displays the following message with status information:

.. code-block:: console

   *** Booting nRF Connect SDK v2.9.0 ***
   nPM1300 PMIC sample running on conexio_stratus_pro_nrf9151_ns
   nRF Fuel Gauge version: 0.9.2
   PMIC device initialized successful
   V: 3.984, I: -0.872, SoC: 0.00, TTE: nan, TTF: nan
   SoC [                    ] 0.00 %
   V: 3.984, I: -0.871, SoC: 10.50, TTE: nan, TTF: nan
   SoC [##                  ] 10.50 %
   V: 3.989, I: -0.873, SoC: 25.30, TTE: nan, TTF: nan
   SoC [#####               ] 25.30 %
   V: 4.020, I: -0.880, SoC: 50.10, TTE: nan, TTF: 6200
   SoC [##########          ] 50.10 %
   V: 4.052, I: -0.886, SoC: 75.30, TTE: nan, TTF: 4909
   SoC [###############     ] 75.30 %
   V: 4.100, I: -0.890, SoC: 99.80, TTE: nan, TTF: 120
   SoC [###################] 99.80 %

SoC Bar Indicator
=================

Each update prints a 20-character visual bar below the numeric data line,
showing the battery state of charge at a glance:

.. code-block:: console

   SoC [###############     ] 75.30 %

Each ``#`` character represents 5 % of charge. The bar is clamped to the
range 0–100 %, so early-boot readings (while the fuel gauge is converging)
are displayed safely. The bar width can be adjusted by changing
``SOC_BAR_WIDTH`` in ``src/fuel_gauge.c``.

   :widths: auto
   :header-rows: 1

   * - Symbol
     - Description
     - Units
   * - V
     - Battery voltage
     - Volts
   * - I
     - Current
     - Amps (negative for charge, positive for discharge)
   * - SoC
     - State of Charge
     - Percent
   * - TTE
     - Time to Empty
     - Seconds (may be NaN)
   * - TTF
     - Time to Full
     - Seconds (may be NaN)

Credit 
===============
Nordic Semiconductor
