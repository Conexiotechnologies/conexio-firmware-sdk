
MQTT: Read and control device over MQTT Protocol
####################################################################################

Description
***********

This sample application shows how to use the MQTT protocol and library to connect to an MQTT 
broker and send data back and forth to another MQTT client to remotely control 
the LEDs and monitor the status of buttons on your Stratus board.

When button 1/ Mode button is pressed on the board, the message “Hello from Conexio Stratus DK” is 
published to `CONFIG_MQTT_PUB_TOPIC`, and any client subscribed to this topic will receive this message.

On the other hand, when the `LEDON` message is sent from another client to the `CONFIG_MQTT_SUB_TOPIC`
topic, the Blue LED on the board is turned ON. When the `LEDOFF` message is sent from another client to the 
`CONFIG_MQTT_SUB_TOPIC`` topic, the LED is turned OFF.


Building and Running
********************

To compile the application issue command

   west build -b conexio_stratus_ns

Flash the compiled firmware using `newtmgr` to the Stratus DK

   newtmgr -c serial image upload build/zephyr/app_update.bin


Sample Output
=============

.. code-block:: console

      *** Booting nRF Connect SDK v2.5.0 ***
      [00:00:00.250,854] <inf> MQTT: Starting MQTT sample on conexio_stratus
      [00:00:00.250,885] <inf> MQTT: Initializing modem library
      [00:00:00.488,494] <inf> MQTT: Connecting to LTE network
      [00:01:09.398,864] <inf> MQTT: RRC mode: Connected
      [00:01:11.441,558] <inf> MQTT: Network registration status: Connected - roaming
      [00:01:11.441,711] <inf> MQTT: Connected to LTE network
      [00:01:11.696,868] <inf> MQTT: IPv4 Address found 91.121.93.94
      [00:01:12.195,983] <inf> MQTT: MQTT client connected
      [00:01:12.196,014] <inf> MQTT: Subscribing to: stratus/sub/topic len 17
      [00:01:12.509,826] <inf> MQTT: SUBACK packet id: 1234
      [00:01:16.851,287] <inf> MQTT: RRC mode: Idle
      [00:01:52.350,280] <inf> MQTT: RRC mode: Connected
      [00:01:52.467,407] <inf> MQTT: MQTT PUBLISH result=0 len=6
      [00:01:52.467,895] <inf> MQTT: Received: LEDOFF
      [00:01:56.277,648] <inf> MQTT: RRC mode: Idle
      [00:02:01.310,577] <inf> MQTT: RRC mode: Connected
      [00:02:01.426,727] <inf> MQTT: MQTT PUBLISH result=0 len=5
      [00:02:01.427,185] <inf> MQTT: Received: LEDON
      [00:02:05.244,934] <inf> MQTT: RRC mode: Idle
      [00:02:12.820,953] <inf> MQTT: RRC mode: Connected
      [00:02:12.947,113] <inf> MQTT: MQTT PUBLISH result=0 len=6
      [00:02:12.947,570] <inf> MQTT: Received: LEDOFF
      [00:02:16.627,197] <inf> MQTT: RRC mode: Idle

Credit 
===============
Nordic Semiconductor