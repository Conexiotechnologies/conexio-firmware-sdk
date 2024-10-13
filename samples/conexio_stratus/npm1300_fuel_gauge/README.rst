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

For the Stratus Pro nrf9151/nRF9161 device, the west build command is: 

   west build -b conexio_stratus_pro/nrf9161/ns -- -DBOARD_ROOT=/opt/nordic/ncs/v2.7.0/conexio-firmware-sdk

For the Stratus nRF9160 device, the build command is:

   west build -b conexio_stratus/nrf9160/ns -- -DBOARD_ROOT=/opt/nordic/ncs/v2.7.0/conexio-firmware-sdk


Flash the compiled firmware using `newtmgr`

   newtmgr -c serial image upload build/zephyr/app_update.bin

Testing
*******

#. Connect the Stratus kit
#. Start the serial terminal program

If the initialization was successful, the terminal displays the following message with status information:

.. code-block:: console

   *** Booting nRF Connect SDK v2.7.0-5cb85570ca43 ***
   *** Using Zephyr OS v3.6.99-100befc70c74 ***
   nPM1300 PMIC sample running on conexio_stratus_pro
   nRF Fuel Gauge version: 0.9.2
   PMIC device init successful
   V: 3.984, I: -0.872, SoC: 0.00, TTE: nan, TTF: nan
   V: 3.984, I: -0.871, SoC: 0.10, TTE: nan, TTF: nan
   V: 3.989, I: -0.873, SoC: 0.19, TTE: nan, TTF: nan
   ...
   V: 4.052, I: -0.886, SoC: 2.04, TTE: nan, TTF: 4946
   V: 4.047, I: -0.885, SoC: 2.13, TTE: nan, TTF: 4937
   V: 4.052, I: -0.886, SoC: 2.22, TTE: nan, TTF: 4928
   V: 4.052, I: -0.886, SoC: 2.32, TTE: nan, TTF: 4918
   V: 4.057, I: -0.887, SoC: 2.41, TTE: nan, TTF: 4909
   V: 4.057, I: -0.887, SoC: 2.51, TTE: nan, TTF: 4898

.. _table::
   :widths: auto

   ======  ===============  ==================================================
   Symbol  Description      Units
   ======  ===============  ==================================================
   V       Battery voltage  Volts
   I       Current          Amps (negative for charge, positive for discharge)
   SoC     State of Charge  Percent
   TTE     Time to Empty    Seconds (may be NaN)
   TTF     Time to Full     Seconds (may be NaN)
   ======  ===============  ==================================================

Credit 
===============
Nordic Semiconductor
