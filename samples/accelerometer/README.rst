.. _lis2dh:

LIS2DH: Motion Sensor Monitor
#############################

Overview
********

This sample application periodically reads accelerometer data from the
LIS2DH sensor onboard Conexio Stratus and displays the sensor data on the console.

Requirements
************

This sample uses the ST Microelectronics LIS2DH MEMS digital output motion sensor, an 
ultra-low-power high-performance 3-axis Femto accelerometer sensor.


References
**********

For more information about the LIS2DH motion sensor and its capabilities see
http://www.st.com/en/mems-and-sensors/lis2dh.html.

Building and Running
********************

See the board documentation for detailed instructions on how to flash
and get access to the console where acceleration data is displayed.

Building on conexio_stratus
===========================

:ref:`conexio_stratus` includes a ST LIS2DH12 accelerometer which
supports the LIS2DH interface.

   west build -b conexio_stratus_ns

Sample Output
=============

.. code-block:: console
    
    Conexio Stratus accelerometer sensor example
    Polling at 0.5 Hz
    #1 @ 12 ms: x -5.387328 , y 5.578368 , z -5.463744
    #2 @ 2017 ms: x -5.310912 , y 5.654784 , z -5.501952
    #3 @ 4022 ms: x -5.349120 , y 5.692992 , z -5.463744

   <repeats endlessly every 2 seconds>
