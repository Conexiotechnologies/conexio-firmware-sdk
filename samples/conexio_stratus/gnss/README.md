
# Stratus GNSS Sample Application

The sample first initializes the GNSS module. 
Then it handles events from the interface, reads the associated data and outputs information to the console. 

Using `lte_lc_func_mode_set()`, the modem is set to normal mode. Then it deactivates LTE using `LTE_LC_FUNC_MODE_DEACTIVATE_LTE`, 
so that the GNSS can operate as intended. In an actual full application, you wouldn’t completely deactivate LTE but 
rather configure PSM or eDRX so that the GNSS and LTE can be used rather often without having to completely 
activate and deactivate the modes. Here it is purely for testing Stratus nRF9160 DK GPS only.

By default, the sample runs in periodic mode.
We have configured the GNSS in periodic mode, by setting the fix interval to 2 minutes and the fix retry period to 8 minutes.
```
CONFIG_GNSS_PERIODIC_INTERVAL=120
CONFIG_GNSS_PERIODIC_TIMEOUT=480
```

After getting the first fix, the GNSS will sleep then wakes up when the GNSS fix interval has 
passed (120 seconds or 2 minutes) and starts searching for a new fix. Then it sleeps and repeats the process.

Build the sample and flash it to your STratus board.
To test the application, make sure you are outdoors under an open sky to achieve the best conditions.

The sample app also display the carrier-to-noise density ratio for all the valid satellites the GNSS is currently 
tracking, which is found in the member cn0. This tells us something about the signal strength from that satellite and it has 
to be above 30 dB/Hz (cn0 = 300) for the GNSS receiver to consider the satellite “healthy” and use it in calculations.
Printing the satellite information and signal strength is useful for debugging purposes if your application is not getting a fix.
As per Nordic Semi, because the GNSS is solving for four unknown variables, it needs at least 4 satellites to get a valid fix.


## Stratus GPS Antenna configurations

Following settings are needed to enable the oboard GPS antenna and the LNA in order for the GPS to work.

```
# Enable modem GPS mode
CONFIG_LTE_NETWORK_MODE_LTE_M_NBIOT_GPS=y

# Stratus DK GPS configurations
CONFIG_MODEM_ANTENNA=y
# Enable onboard GPS antenna
CONFIG_MODEM_ANTENNA_GNSS_ONBOARD=y
CONFIG_MODEM_ANTENNA_AT_MAGPIO="AT\%XMAGPIO=1,0,0,1,1,1574,1577"
# Stratus GPS external antenna configuration with LNA enabled
CONFIG_MODEM_ANTENNA_AT_COEX0="AT\%XCOEX0=1,1,1565,1586"
```

## Building and Running

See the (device documentation)[https://docs.conexiotech.com] for detailed instructions on how to get started.

For the Stratus Pro nrf9151/nRF9161 device, the `west` build command is: 
```
west build -b conexio_stratus_pro/nrf9161/ns -- -DBOARD_ROOT=/opt/nordic/ncs/v2.7.0/conexio-firmware-sdk
```

For the Stratus nRF9160 device, the build command is:

```
west build -b conexio_stratus/nrf9160/ns -- -DBOARD_ROOT=/opt/nordic/ncs/v2.7.0/conexio-firmware-sdk
```

Flash the compiled firmware using `newtmgr`

```
newtmgr -c serial image upload build/zephyr/app_update.bin
```

## Sample Output

```
*** Booting nRF Connect SDK v2.7.0-5cb85570ca43 ***
*** Using Zephyr OS v3.6.99-100befc70c74 ***
[00:00:00.583,709] <inf> GNSS_sample: Stratus GNSS sample started
[00:00:00.583,709] <inf> GNSS_sample: Connecting to LTE network
[00:00:01.562,622] <inf> GNSS_sample: RRC mode: Connected
[00:00:03.604,370] <inf> GNSS_sample: Network registration status: Connected - roaming
[00:00:03.604,553] <inf> GNSS_sample: Connected to LTE network
[00:00:03.604,553] <inf> GNSS_sample: Deactivating LTE
[00:00:05.000,640] <inf> GNSS_sample: RRC mode: Idle
[00:00:05.076,110] <inf> GNSS_sample: Activating GNSS
[00:00:05.078,826] <inf> GNSS_sample: Starting GNSS
[00:00:05.104,583] <inf> GNSS_sample: Searching...
[00:00:06.108,734] <inf> GNSS_sample: Searching...
[00:00:07.110,412] <inf> GNSS_sample: Searching...
[00:00:08.109,893] <inf> GNSS_sample: Searching...
...
...
[00:00:33.112,548] <inf> GNSS_sample: Searching...
[00:00:34.112,518] <inf> GNSS_sample: Searching...
[00:00:35.112,579] <inf> GNSS_sample: Searching...
[00:00:36.112,579] <inf> GNSS_sample: Searching...
[00:00:37.112,701] <inf> GNSS_sample: Searching...
[00:00:38.112,670] <inf> GNSS_sample: Searching...
[00:00:39.178,436] <inf> GNSS_sample: Searching...
[00:00:39.178,497] <inf> GNSS_sample: Latitude:       41.908028
[00:00:39.178,527] <inf> GNSS_sample: Longitude:      -87.629970
[00:00:39.178,527] <inf> GNSS_sample: Altitude:       569.3 m
[00:00:39.178,558] <inf> GNSS_sample: Time (UTC):     15:18:00.567
[00:00:39.178,558] <inf> GNSS_sample: Time to first fix: 77 s
[00:00:39.180,928] <inf> GNSS_sample: GNSS enter sleep after fix
```

## Credit 

Nordic Semiconductor