.. _at_client_sample:

Cellular: AT Client
###################

.. contents::
   :local:
   :depth: 2

The AT Client sample demonstrates the asynchronous serial communication taking place over UART to the cellular modem.
The sample enables you to use an external computer or MCU to send AT commands to the LTE-M/NB-IoT modem of your nRF91 Series device.

Requirements
************

The sample supports the nRF91 based development kits.

Overview
********

The AT Client sample acts as a proxy for sending directives to the cellular modem using AT commands.
This facilitates the reading of responses or analyzing of events related to the cellular modem.
You can initiate the commands manually from a terminal such as the nRF Connect Serial Terminal , or visually using the Cellular Monitor app.
Both apps are part of nRF Connect for Desktop.

For more information on the AT commands, see the nRF91x1 AT Commands Reference Guide or nRF9160 AT Commands Reference Guide depending on the SiP you are using.

Building and running
********************

   west build -b conexio_stratus_ns

Flash the compiled firmware using `newtmgr`

   newtmgr -c serial image upload build/zephyr/app_update.bin

Testing
=======

After programming the sample to your development kit, test it by performing the following steps:

1. Press the reset button on the the Conexio Stratus to reboot the kit and start the AT Client sample.
#. Connect to the Conexio Stratus with a Serial Terminal with baud rate of 115200.
#. Run the following commands from the Serial Terminal:

   a. Enter the command: :command:`AT+CFUN?`.

      This command reads the current functional mode of the modem and triggers the command :command:`AT+CFUN=1` which sets the functional mode of the modem to normal.

   #. Enter the command: :command:`AT%XOPERID`.

      This command returns the network operator ID.

   #. Enter the command: :command:`AT%XMONITOR`.

      This command returns the modem parameters.

   #. Enter the command: :command:`AT%XTEMP?`.

      This command displays the current modem temperature.

   #. Enter the command: :command:`AT%CMNG=1`.

      This command displays a list of all certificates that are stored on your device.
      If you add the device to nRF Cloud, a CA certificate, a client certificate, and a private key with security tag 16842753 (which is the security tag for nRF Cloud credentials) are displayed.


Sample output
=============

Following is a sample output of the command :command:`AT%XMONITOR`:

.. code-block:: console

   AT%XMONITOR
   %XMONITOR: 5,"","","24201","76C1",7,20,"0102DA03",105,6400,53,24,"","11100000","11100000"
   OK

Credit
=============
Nordic Semiconductor