LED Blink on Button Press
###################

Overview
********

This simple application demonstrates how to use the button on the Stratus dev kit and blink the LED
when pressed. The source code shows how to configure GPIO pins as outputs, setup the button handler,
then turn on and off the LED on the Stratus main board.

Building and Running
********************
Build the sample for the respective Stratus device using the nRF Connect for VSCode plugin.

Then flash the compiled firmware using `newtmgr`

   newtmgr -c serial image upload build/zephyr/app_update.bin

After flashing the Conexio Stratus board, reset the board and press the BUTTON/MODE to see the LED blink.
