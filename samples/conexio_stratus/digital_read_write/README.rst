.. _digital_read_write:

Digital Read/Write GPIO
#######################

Overview
********

This sample demonstrates how to use a GPIO pin as a digital input and another
as a digital output simultaneously on the Conexio Stratus Pro using the Zephyr
GPIO API.

The input pin is polled every second. The output pin mirrors the input state —
driven high when the input is asserted, low otherwise. Both states are logged
to the serial console.

Pin assignment (set via DT overlay):

.. list-table::
   :widths: auto
   :header-rows: 1

   * - Role
     - Pin
     - Configuration
   * - Digital input
     - P0.18
     - Pull-up, active-low (asserts when connected to GND)
   * - Digital output
     - P0.24
     - Active-high, starts low

To test without extra hardware: connect P0.18 to GND to assert the input and
observe P0.24 switching high with a multimeter or logic analyser.

Requirements
************

* Conexio Stratus Pro (nRF9151 or nRF9161)
* nRF Connect SDK v2.9.0 or newer

Building and Running
********************

For the Stratus Pro nRF9151 device:

   west build -b conexio_stratus_pro/nrf9151/ns -- -DBOARD_ROOT=/opt/nordic/ncs/v3.2.1/conexio-firmware-sdk

For the Stratus Pro nRF9161 device:

   west build -b conexio_stratus_pro/nrf9161/ns -- -DBOARD_ROOT=/opt/nordic/ncs/v3.2.1/conexio-firmware-sdk

Flash the compiled firmware using ``newtmgr``

   newtmgr -c serial image upload build/digital_read_write/zephyr/zephyr.signed.bin

Sample Output
=============

With input pin floating (idle):

.. code-block:: console

   Digital read/write sample running on conexio_stratus_pro_nrf9151_ns
   [00:00:00.210,000] <inf> digital_rw: Input pin P0.18 configured (pull-up, active-low)
   [00:00:00.210,100] <inf> digital_rw: Output pin P0.24 configured (active-high, starts low)
   [00:00:01.210,000] <inf> digital_rw: Input  P0.18 = 0 (idle)
   [00:00:01.210,100] <inf> digital_rw: Output P0.24 = 0

With P0.18 connected to GND (input asserted):

.. code-block:: console

   [00:00:02.210,000] <inf> digital_rw: Input  P0.18 = 1 (ASSERTED)
   [00:00:02.210,100] <inf> digital_rw: Output P0.24 = 1

Credit
======
Conexio Technologies, Inc
