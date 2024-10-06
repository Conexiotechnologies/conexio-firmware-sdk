.. _npm1300_fuel_gauge:

nPM1300: Fuel gauge
###################

The Fuel gauge sample demonstrates how to calculate the battery state of charge using the nPM1300 fuel gauge on 
Stratus Pro nrf9151/nRF9161 devices.

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

   *** Booting nRF Connect SDK v3.5.99-ncs1 ***
   nPM1300 PMIC sample running on conexio_stratus_pro
   nRF Fuel Gauge version: 0.9.2
   PMIC device init successful
   V: 3.818, I: 0.000, SoC: 53.56, TTE: 42246056, TTF: nan

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
