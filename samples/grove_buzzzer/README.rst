.. _buzzer-sample:

Groove Buzzer
######

Overview
********

Buzzer is a simple application which turns on a groove buzzer using the :ref:`GPIO
API <gpio_api>`. The source code shows how to configure GPIO pins as outputs,
then turn them on and off.

.. _buzzer-sample-requirements:

Requirements
************

The Conexio Stratus board must be attached to the Stratus shield to directly
interface the groove buzzer and enable it. The buzzer uses GPIO PIN 27 to
get the signal from the MCU. 

Building and Running
********************

Build and flash Buzzer as follows:

   west build -b conexio_stratus_ns

After flashing, the buzzer starts to buzzzzz every 5 seconds.
