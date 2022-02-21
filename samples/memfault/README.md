
# Conexio Stratus to Memfault cloud

This sample shows how to use the Memfault SDK to collect coredumps, reboot reasons, metrics and trace events from Conexio Stratus. The sample connects to an LTE network and sends the collected data to Memfault’s cloud using HTTPS.

### [📣 Find our walk-through tutorial for this sample application → here](https://www.rajeevpiyare.com/posts/stratus-to-memfault/)


## Metrics

The sample adds properties specific to the application, while the Memfault SDK integration layer in NCS adds the system property metrics.
Some metrics are collected by the Memfault SDK directly.
There are also some metrics, which are specific to NCS that are enabled by default:

* LTE metrics:

  * Enabled and disabled using :option:`CONFIG_MEMFAULT_NCS_LTE_METRICS`.
  * ``Ncs_LteTimeToConnect`` - Time from the point when the device starts to search for an LTE network until the time when it gets registered with the network.
  *  ``Ncs_LteConnectionLossCount`` - The number of times that the device has lost the LTE network connection after the initial network registration.

* Stack usage metrics:

   * Shows how many bytes of unused space is left in a stack.
   * Configurable using :option:`CONFIG_MEMFAULT_NCS_STACK_METRICS`.
   * ``Ncs_AtCmdUnusedStack`` - The :ref:`at_cmd_readme` library's stack.
   * ``Ncs_ConnectionPollUnusedStack``- Stack used by the cloud libraries 

In addition to showing the capturing of metrics provided by the Memfault SDK integration layer in |NCS|, the sample also shows how to capture an application-specific metric.
This metric is defined in :file:`samples/memfault/config/memfault_metrics_heartbeat_config.h`:


## Coredumps

Coredumps can be triggered either by using the Memfault shell command ``mflt crash``, or by pressing a button:

*  **Button 1** triggers a stack overflow

These faults cause crashes that are captured by Memfault.
After rebooting, the crash data can be sent to the Memfault cloud for further inspection and analysis.
See [Memfault documentation](https://docs.memfault.com/docs/platform/projects-and-fleets/) for more information on the debugging possibilities offered by the Memfault platform.

The sample enables Memfault shell by default.
The shell offers multiple test commands to test a wide range of functionality offered by Memfault SDK.
Run the command ``mflt help`` in the terminal for more information on the available commands.


## Configuration

The Memfault SDK allows the configuration of some of its options using Kconfig.
To configure the options in the SDK that are not available for configuration using Kconfig, use :file:`samples/memfault/config/memfault_platform_config.h`.
See [Memfault SDK](https://github.com/memfault/memfault-firmware-sdk/) for more information.

## Minimal setup

To send data to the Memfault cloud, a project key must be configured using :option:`CONFIG_MEMFAULT_NCS_PROJECT_KEY`.

**Note**: The Memfault SDK requires certificates required for the HTTPS transport.
   The certificates are by default provisioned automatically by the |NCS| integration layer for Memfault SDK to sec tags 1001 - 1005.
   If other certificates exist at these sec tags, HTTPS uploads will fail.


## Additional configuration

There are two sources for Kconfig options when using Memfault SDK in |NCS|:

* Kconfig options defined within the Memfault SDK.
* Kconfig options defined in the |NCS| integration layer of the Memfault SDK. These configuration options are prefixed with ``CONFIG_MEMFAULT_NCS``.

Check and configure the following options in Memfault SDK that are used by the sample:

* :option:`CONFIG_MEMFAULT`
* :option:`CONFIG_MEMFAULT_ROOT_CERT_STORAGE_NRF9160_MODEM`
* :option:`CONFIG_MEMFAULT_SHELL`
* :option:`CONFIG_MEMFAULT_HTTP_ENABLE`
* :option:`CONFIG_MEMFAULT_HTTP_PERIODIC_UPLOAD`
* :option:`CONFIG_MEMFAULT_HTTP_PERIODIC_UPLOAD_INTERVAL_SECS`
* :option:`CONFIG_MEMFAULT_HTTP_PERIODIC_UPLOAD_USE_DEDICATED_WORKQUEUE`
* :option:`CONFIG_MEMFAULT_COREDUMP_COLLECT_BSS_REGIONS`

If :option:`CONFIG_MEMFAULT_ROOT_CERT_STORAGE_NRF9160_MODEM` is enabled, TLS certificates used for HTTP uploads are provisioned to the nRF9160 modem when :c:func:`memfault_zephyr_port_install_root_certs` is called.

Check and configure the following options for Memfault that are specific to |NCS|:

* :option:`CONFIG_MEMFAULT_NCS_PROJECT_KEY`
* :option:`CONFIG_MEMFAULT_NCS_LTE_METRICS`
* :option:`CONFIG_MEMFAULT_NCS_STACK_METRICS`
* :option:`CONFIG_MEMFAULT_NCS_INTERNAL_FLASH_BACKED_COREDUMP`

If :option:`CONFIG_MEMFAULT_NCS_INTERNAL_FLASH_BACKED_COREDUMP` is enabled, :option:`CONFIG_PM_PARTITION_SIZE_MEMFAULT_STORAGE` can be used to set the flash partition size for the flash storage.


## Building and Running the Sample Application
To compile the application, open a terminal window in the application directory and issue the following `west` command
```
west build -b conexio_stratus_ns
```

Once the application is compiled successfully, connect the Stratus device and put it into the DFU mode.

Flash the compiled firmware using `newtmgr`
```
newtmgr -c serial image upload build/zephyr/app_update.bin
```

Open up a serial console and reset the Stratus device. 

## Sample output
The following serial UART output will be displayed in the terminal. 
```
uart:~$ *** Booting Zephyr OS build v2.6.99-ncs1  ***
<inf> <mflt>: Reset Reason, RESETREAS=0x1
inf> <mflt>: Reset Causes:
<inf> <mflt>:  Pin Reset
<inf> <mflt>: GNU Build ID: 098b8b74929371d5ad655dfe0d8df6cf8e59cd91
Conexio Stratus Memfault sample has started
<inf> memfault_sample: Connecting to LTE network, this may take several minutes...
<inf> memfault_sample: Active LTE mode changed: LTE-M
<inf> memfault_sample: Network registration status: Connected - roaming
<inf> memfault_sample: Connected to LTE network. Time to connect: 2301 ms
<inf> memfault_sample: Sending already captured data to Memfault
uart:~$ <dbg> <mflt>: Response Complete: Parse Status 0 HTTP Status 202!
<dbg> <mflt>: Body: Accepted
<dbg> <mflt>: Response Complete: Parse Status 0 HTTP Status 202!
<dbg> <mflt>: Body: Accepted
<dbg> <mflt>: No more data to send
```