LED Blink
###################

Overview
********

Blink is a simple application which blinks the user LED forever using the `GPIO
API`. The source code shows how to configure GPIO pins as outputs,
then turn them on and off on the Stratus main board.


Requirements
************

You will see this error if you try to build Blink sample for an unsupported board:

The board must have an LED connected via a GPIO pin. These are called "User
LEDs" on many of Zephyr's `boards`. The LED must be configured using the
``led0`` alias. This is usually done in the
`BOARD.dts file <devicetree-in-out-files>` or a `devicetree overlay
<set-devicetree-overlays>`.

Building and Running
********************

Build and flash Blink sample as follows:

   west build -b conexio_stratus_pro_ns

Flash the compiled firmware using `newtmgr`

   newtmgr -c serial image upload build/zephyr/app_update.bin

After flashing the Conexio Stratus board, the onboard LED starts to blink every second.

Credit
=============
Nordic Semiconductor