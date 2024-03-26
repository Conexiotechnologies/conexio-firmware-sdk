Multicore Hello World application
#################################

The sample demonstrates how to build the Hello World application and blink the red LED on the Conexio Blitz for the multicore SoC.
The sample adds a remote image as a child image from the sample source files.

Requirements
************

The sample supports the following development kits:

* Conexio Blitz
* nRF5340 development kit
* Thingy53

Overview
********

The sample demonstrates how to build a multicore Hello World application with the `zephyr:sysbuild`.
When building with Zephyr Sysbuild, the build system adds child images based on the options selected 
in the project's additional configuration and build files.
This sample shows how to inform the build system about dedicated sources for additional images.
The sample comes with the following additional files:

* `Kconfig.sysbuild` - This file is used to add Sysbuild configuration that is passed to all the images.
  ``SB_CONFIG`` is the prefix for sysbuild’s Kconfig options.
* `sysbuild.cmake` - The CMake file adds additional images using the `ExternalZephyrProject_Add` macro.
  You can also add the dependencies for the images if required.

Both the application and remote cores use the same `main.c` that prints the name of the development kit on which the application is programmed.

Building and Running
********************

   west build -b conexio_blitz_cpuapp

Testing
=======

After programming the sample to your development kit, complete the following steps to test it:

      1. Connect the board to the serial terminal program via USB-C.
      #. Reset the Blitz kit.
      #. Observe the console output for both cores:

         * For the application core, the output should be as follows:

            .. code-block:: console

               *** Booting nRF Connect SDK v3.5.99-ncs1 ***
               Hello world from conexio_blitz_cpuapp

         * For the network core, the output should be as follows:

            .. code-block:: console

               *** Booting nRF Connect SDK v3.5.99-ncs1 ***
               Hello world from conexio_blitz_cpunet

Credit
=============
Nordic Semiconductor