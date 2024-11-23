
# Wi-Fi Scan with Conexio Stratus Pro and Expansion Dock

The wifi_scan sample application demonstrates how to use the Nordic Semiconductor's Wi-Fi® chipset on nRF7002EB to scan for the access points without using the wpa_supplicant.

## Requirements

Must have:
* Downloaded the latest [Conexio firmware SDK](https://github.com/Conexiotechnologies/conexio-firmware-sdk).
* **Conexio Expansion Dock** connected to either of the following development kits:
    - Conexio Stratus Pro nRF9151, or
    - Conexio Stratus Pro nRF9161

## Overview

The sample demonstrates Wi-Fi scan operations in the 2.4 GHz and 5 GHz bands.
The sample allows you to perform scan based on below profiles.

* Default scan
* Active scan
* Passive scan
* 2.4 GHz Active scan
* 2.4 GHz Passive scan
* 5 GHz Active scan
* 5 GHz Passive scan
* Scan only non-overlapping channels in the 2.4 GHz band
* Scan only non-DFS channels in the 5 GHz band
* Scan only non-overlapping channels in the 2.4 GHz and non-DFS channels in the 5 GHz band

Using this sample, the Stratus Pro development kit scans available access points in `STA (Station)` mode.

Read more here for [Developing with nRF7002EB](https://docs.nordicsemi.com/bundle/ncs-latest/page/nrf/app_dev/device_guides/nrf70/nrf7002eb_dev_guide.html).

## Build Configuration for nRF Connect extension for VSCode 

Follow these steps to successfully build the application.

1. Import `wifi_scan` application in nRF Connect for VSCode
2. Add Build Configuration
3. Under **Board Target** check the Custom boards
4. From the drop-down menu select the desired Conexio Stratus Pro board: 
- conexio_stratus_pro/nrf9161/ns for Stratus Pro nRF9161
- conexio_stratus_pro/nrf9151/ns for Stratus Pro nRF9151
5. Next, under **Base Devicetree overlays** select the appropriate device overlay file
- conexio_stratus_pro_nrf9161_ns.overlay for Stratus Pro nRF9161
- conexio_stratus_pro_nrf9151_ns.overlay for Stratus Pro nRF9151
6. Next, specify `-DSHIELD=nrf7002eb` in the **Extra Cmake arguments** field
7. Finally, click **Build Configuration** to start the Zephyr application build process.

### Build Configuration for Stratus Pro nRF9151

![Build Configuration for Stratus Pro nRF9151](https://github.com/Conexiotechnologies/conexio-firmware-sdk/blob/main/samples/conexio_stratus/wifi_scan/assets/stratus_pro_nrf9151_build_configs.png)

### Build Configuration for Stratus Pro nRF9161
![Build Configuration for Stratus Pro nRF9161](https://github.com/Conexiotechnologies/conexio-firmware-sdk/blob/main/samples/conexio_stratus/wifi_scan/assets/stratus_pro_nrf9161_build_configs.png)


## Uploading and running the application

Once the application is compiled successfully, 
1. Connect the Stratus Pro device to the PC via USB-C and put it into the DFU mode.
2. Connect the LiPo battery to the expansion dock and switch **ON** the power
3. Flash the compiled firmware using `newtmgr`
```
newtmgr -c serial image upload build/zephyr/app_update.bin
```
4. Open up a serial console and reset the Stratus Pro device once program upload is complete. 

## Sample output

The following serial logs will be displayed in the terminal. 

```
All pins have been configured as non-secure
[Sec Thread] Secure image initializing!
Booting TF-M v2.0.0-ncs2
*** Booting nRF Connect SDK v2.7.0-5cb85570ca43 ***
*** Using Zephyr OS v3.6.99-100befc70c74 ***
[00:00:00.258,575] <inf> lis2dh: fs=2, odr=0x4 lp_en=0x0 scale=9576
[00:00:00.269,165] <inf> wifi_nrf_bus: SPIM spi@b000: freq = 8 MHz
[00:00:00.269,195] <inf> wifi_nrf_bus: SPIM spi@b000: latency = 0
Starting wifi scanner on conexio_stratus_pro with CPU frequency: 64 MHz
Scan requested
Num  | SSID                             (len) | Chan | RSSI | Security | BSSID
1    | Big Kahunas                      11    | 6    | -44  | WPA2-PSK | 9A:9D:5D:75:F7:52
2    |                                  0     | 6    | -45  | EAP   | 9A:9D:5D:75:F7:57
3    |                                  0     | 6    | -45  | WPA2-PSK | 9A:9D:5D:75:F7:53
4    |                                  0     | 6    | -46  | WPA2-PSK | 9A:9D:5D:75:F7:55
5    | Matthews Wifi 2.4GhZ             20    | 5    | -49  | WPA2-PSK | 78:6A:1F:CA:2D:C0
6    | NETGEAR29                        9     | 10   | -50  | WPA2-PSK | 80:CC:9C:9E:3B:4B
7    |                                  0     | 157  | -52  | EAP   | 9A:9D:5D:7C:F7:50
8    |                                  0     | 157  | -53  | WPA2-PSK | 9A:9D:5D:7C:F7:56
9    | Big Kahunas                      11    | 157  | -53  | WPA2-PSK | 9A:9D:5D:7C:F7:53
10   | Matthews Wifi 5ghz               18    | 60   | -56  | WPA2-PSK | 66:6A:1F:CA:2D:C3
11   |                                  0     | 60   | -56  | WPA2-PSK | 78:6A:1F:CA:2D:C3
12   |                                  0     | 36   | -56  | WPA2-PSK | 12:3D:0A:74:21:B7
13   |                                  0     | 1    | -58  | WPA-PSK | BE:34:26:32:62:5E
14   | Rajeev-5G                        9     | 11   | -59  | WPA2-PSK | F8:34:5A:14:DA:08
15   |                                  0     | 11   | -60  | WPA2-PSK | FA:34:5A:14:DA:08
16   | SBG658004                        9     | 11   | -61  | WPA2-PSK | 20:10:7A:13:E9:F6
17   | Rajeev-5G                        9     | 36   | -63  | WPA2-PSK | FA:34:5A:14:DA:00
18   |                                  0     | 40   | -64  | EAP   | DE:34:26:32:62:64
19   |                                  0     | 40   | -64  | WPA-PSK | BE:34:26:32:62:64
20   | Queensolomon8                    13    | 40   | -64  | WPA2-PSK | 9C:34:26:32:62:64
21   |                                  0     | 36   | -64  | WPA2-PSK | F8:34:5A:14:DA:10
22   | Biden-Harris                     12    | 10   | -67  | WPA2-PSK | D8:07:B6:A3:DB:5C
23   | Router? I barely know her        25    | 11   | -68  | WPA2-PSK | 80:CC:9C:B6:0D:33
24   | bigcitykitty_2G                  15    | 4    | -69  | WPA2-PSK | FC:34:97:1B:30:40
25   | MWFrink                          7     | 149  | -70  | WPA2-PSK | C0:7C:D1:5A:BE:50
26   |                                  0     | 6    | -70  | EAP   | D4:B9:2F:09:B9:7D
27   |                                  0     | 157  | -71  | WPA2-PSK | D4:B9:2F:09:B9:83
28   |                                  0     | 149  | -71  | EAP   | CE:7C:D1:5A:BE:50
29   |                                  0     | 149  | -71  | WPA-PSK | C6:7C:D1:5A:BE:50
30   | ARRIS-24FD-5G                    13    | 40   | -71  | WPA2-PSK | 8C:5A:25:BB:30:58
31   |                                  0     | 6    | -71  | WPA2-PSK | D4:B9:2F:09:B9:7E
32   | gobears                          7     | 161  | -72  | WPA3-SAE | DC:8D:8A:E5:FE:D7
33   | SpartanNetwork                   14    | 157  | -72  | WPA2-PSK | D4:B9:2F:09:B9:80
34   |                                  0     | 157  | -72  | EAP   | D4:B9:2F:09:B9:85
35   | NETGEAR29-5G                     12    | 153  | -72  | WPA2-PSK | 80:CC:9C:9E:3B:4D
36   |                                  0     | 6    | -72  | WPA2-PSK | D4:B9:2F:09:B9:79
37   | MWFrink                          7     | 6    | -72  | WPA2-PSK | C0:7C:D1:5A:BE:48
38   |                                  0     | 6    | -72  | OPEN  | DA:7C:D1:5A:BE:48
39   |                                  0     | 161  | -73  | WPA2-PSK | EE:8D:8A:E5:FE:D7
40   |                                  0     | 157  | -73  | WPA2-PSK | D4:B9:2F:09:B9:86
41   | ARRIS-32C5                       10    | 11   | -73  | WPA2-PSK | 84:BB:69:FE:CE:7A
42   | ARRIS-BBBD                       10    | 11   | -73  | WPA2-PSK | A0:68:7E:BC:EF:70
43   |                                  0     | 6    | -74  | EAP   | D6:7C:D1:5A:BE:48
44   | Lily                             4     | 6    | -74  | WPA2-PSK | 8C:5A:25:BB:30:57
45   | Xfinity Mobile                   14    | 40   | -76  | EAP   | CE:34:26:32:62:64
46   | xfinitywifi                      11    | 40   | -76  | OPEN  | AE:34:26:32:62:64
47   | RCC-WIFI                         8     | 1    | -76  | WPA2-PSK | AC:8B:A9:5E:93:62
48   | ARRIS-32C5-5G                    13    | 161  | -77  | WPA2-PSK | 84:BB:69:FE:CE:7B
49   | aries lair                       10    | 153  | -77  | WPA2-PSK | C0:A0:0D:D3:A1:A0
50   | Router? I barely know her-5G     28    | 153  | -77  | WPA2-PSK | 80:CC:9C:B6:0D:34
51   |                                  0     | 153  | -77  | WPA-PSK | E2:A0:0D:D3:A1:A0
52   |                                  0     | 44   | -77  | EAP   | D6:94:35:45:ED:6F
53   | JackieWIFI                       10    | 6    | -77  | WPA2-PSK | 78:F2:9E:3C:EE:C8
54   |                                  0     | 1    | -77  | WPA2-PSK | B2:8B:A9:5E:93:62
55   | Xfinity Mobile                   14    | 44   | -78  | EAP   | D2:94:35:45:ED:6F
56   | xfinitywifi                      11    | 44   | -78  | OPEN  | CA:94:35:45:ED:6F
57   | KeepMyWifisName                  15    | 44   | -78  | WPA2-PSK | C0:94:35:45:ED:6F
58   |                                  0     | 44   | -78  | WPA2-PSK | CE:94:35:45:ED:6F
59   | xfinitywifi                      11    | 149  | -79  | OPEN  | D6:7C:D1:5A:BE:50
60   | RCC-Nest                         8     | 1    | -79  | WPA2-PSK | E6:63:DA:1E:78:95
61   | Xfinity Mobile                   14    | 149  | -80  | EAP   | CA:7C:D1:5A:BE:50
62   |                                  0     | 48   | -80  | WPA-PSK | 7E:F2:9E:3C:EE:D0
63   | JackieWIFI                       10    | 48   | -80  | WPA2-PSK | 78:F2:9E:3C:EE:D0
64   |                                  0     | 48   | -80  | WPA-PSK | 8A:F2:9E:3C:EE:D0
65   |                                  0     | 48   | -80  | EAP   | 86:F2:9E:3C:EE:D0
66   |                                  0     | 1    | -80  | WPA2-PSK | EA:63:DA:1E:78:95
67   | Xfinity Mobile                   14    | 157  | -82  | EAP   | D4:B9:2F:09:B9:84
68   |                                  0     | 44   | -82  | WPA2-PSK | EA:63:DA:1F:78:95
69   | gobears                          7     | 40   | -82  | WPA3-SAE | DC:8D:8A:E5:FE:D6
70   | xfinitywifi                      11    | 157  | -83  | OPEN  | D4:B9:2F:09:B9:82
71   | RCC-WIFI                         8     | 44   | -83  | WPA2-PSK | E0:63:DA:1F:78:95
72   | RCC-Nest                         8     | 44   | -84  | WPA2-PSK | E6:63:DA:1F:78:95
73   | Xfinity Mobile                   14    | 153  | -85  | EAP   | F2:A0:0D:D3:A1:A0
74   |                                  0     | 100  | -85  | WPA2-PSK | 58:96:71:5C:C2:CF
75   | Maliewifi                        9     | 100  | -85  | WPA2-PSK | 58:96:71:5C:C2:C8
76   | RCC Wireless 5.1                 16    | 44   | -85  | WPA2-PSK | 3C:37:86:6C:17:42
77   | ARRIS-C573                       10    | 6    | -86  | WPA2-PSK | 2C:99:24:75:C5:71
78   | Holo                             4     | 2    | -87  | WPA2-PSK | 84:D8:1B:CC:42:E7
79   | Xfinity Mobile                   14    | 48   | -88  | EAP   | 82:F2:9E:3C:EE:D0
Scan request done
```

## Credit

Nordic Semiconductor