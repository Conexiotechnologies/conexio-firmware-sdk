.. _blinky-sample:

LED Blink
######

Overview
********

Blink is a simple application which blinks an LED forever using the :ref:`GPIO
API <gpio_api>`. The source code shows how to configure GPIO pins as outputs,
then turn them on and off, both on the Stratus main board and the shield.

.. _blinky-sample-requirements:

Requirements
************

You will see this error if you try to build Blink sample for an unsupported board:

.. code-block:: none

   Unsupported board: led0 devicetree alias is not defined

The board must have an LED connected via a GPIO pin. These are called "User
LEDs" on many of Zephyr's :ref:`boards`. The LED must be configured using the
``led0`` :ref:`devicetree <dt-guide>` alias. This is usually done in the
:ref:`BOARD.dts file <devicetree-in-out-files>` or a :ref:`devicetree overlay
<set-devicetree-overlays>`.

Building and Running
********************

Build and flash Blink sample as follows:

   west build -b conexio_stratus_ns

After flashing the Conexio Stratus board, the LED starts to blink every second.