# Conexio Stratus [nRF9160] to IoT Creators Cloud sample application 

## Overview
This sample application connects nRF9160 based Conexio Stratus dev kit to the IoT Creators platform.
In *Active mode* the application sends environmental data [temp & humidity] every 60 seconds to the IoT Creators platform.

## Requirements
- Conexio Stratus IoT Cellular kit
- nRF Connect SDK v2.1.1 or latest

## Prerequisites

Before compiling and flashing this sample application please go over the IoT Creators documentation
[here](https://docs.iotcreators.com/docs/3-register-devkit) on how to register and communicate between 
the IoT Creators platform and your device.

Details on how to get started with the Stratus kit can be found [here](https://docs.conexiotech.com/building-and-programming-an-application).

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
2023-01-31 20:38:51 --> I: sensor-pwr-ctrl sync: 0
2023-01-31 20:38:51 --> I: fs=2, odr=0x4 lp_en=0x0 scale=9576
2023-01-31 20:38:51 --> *** Booting Zephyr OS build v3.1.99-ncs1  ***
2023-01-31 20:38:51 --> I: Conexio Stratus < > IoT Creators Demo Application
2023-01-31 20:38:51 --> Dev kit: conexio_stratus
2023-01-31 20:38:52 --> I: Reading Modem IMEI
2023-01-31 20:38:52 --> I: Modem IMEI: +CGSN: "351516178699401"
2023-01-31 20:38:52 --> 
2023-01-31 20:38:52 --> OK
2023-01-31 20:38:52 --> 
2023-01-31 20:38:52 --> 
2023-01-31 20:38:52 --> I: Setting up the APN
2023-01-31 20:38:52 --> I: AT+CGDCONT?: +CGDCONT: 0,"IPV4V6","scs.telekom.tma.iot","",0,0
2023-01-31 20:38:52 --> 
2023-01-31 20:38:52 --> OK
2023-01-31 20:38:52 --> 
2023-01-31 20:38:52 --> 
2023-01-31 20:39:05 --> +CEREG: 2,"412D","03392510",7
2023-01-31 20:39:05 --> I: LTE cell changed: Cell ID: 54076688, Tracking area: 16685
2023-01-31 20:39:05 --> +CSCON: 1
2023-01-31 20:39:05 --> I: RRC mode: Connected
2023-01-31 20:39:05 --> +CEREG: 2,"412D","03392510",7,0,11
2023-01-31 20:39:06 --> +CSCON: 0
2023-01-31 20:39:06 --> I: RRC mode: Idle
2023-01-31 20:39:08 --> +CEREG: 2,"A2EB","0051F115",7
2023-01-31 20:39:08 --> I: LTE cell changed: Cell ID: 5370133, Tracking area: 41707
2023-01-31 20:39:08 --> +CSCON: 1
2023-01-31 20:39:08 --> I: RRC mode: Connected
2023-01-31 20:39:13 --> +CEREG: 5,"A2EB","0051F115",7,,,"11100000","11100000"
2023-01-31 20:39:13 --> I: Network registration status: Connected - roaming
2023-01-31 20:39:13 --> I: PSM parameter update: TAU: 3240, Active time: -1
2023-01-31 20:39:13 --> +CEDRXP: 4,"1001","1001","0001"
2023-01-31 20:39:13 --> I: eDRX parameter update: eDRX: 163.839996, PTW: 2.560000
2023-01-31 20:39:13 --> I: Transmitting initial UDP data
2023-01-31 20:39:13 --> I: Sending UDP payload, length: 87, data: {"Msg":"Event: Stratus connected, 351516178699401, 310260, 12","Oper":"310260","Bd":12}
2023-01-31 20:39:18 --> I: Transmitting sensor data to IoTCreators cloud
2023-01-31 20:39:18 --> I: Sending UDP payload, length: 36, data: {"Temp":24.674219,"Humid":12.413818}
2023-01-31 20:39:19 --> +CSCON: 0
2023-01-31 20:39:19 --> I: RRC mode: Idle
2023-01-31 20:39:19 --> +CSCON: 1
2023-01-31 20:39:19 --> I: RRC mode: Connected
2023-01-31 20:39:22 --> +CSCON: 0
2023-01-31 20:39:22 --> I: RRC mode: Idle
2023-01-31 20:39:48 --> I: Transmitting sensor data to IoTCreators cloud
2023-01-31 20:39:48 --> I: Sending UDP payload, length: 36, data: {"Temp":24.714274,"Humid":12.347061}
2023-01-31 20:39:48 --> +CSCON: 1
2023-01-31 20:39:48 --> I: RRC mode: Connected
2023-01-31 20:39:52 --> +CSCON: 0
2023-01-31 20:39:52 --> I: RRC mode: Idle
2023-01-31 20:40:18 --> I: Transmitting sensor data to IoTCreators cloud
2023-01-31 20:40:18 --> I: Sending UDP payload, length: 36, data: {"Temp":24.663538,"Humid":12.453872}
2023-01-31 20:40:18 --> +CSCON: 1
2023-01-31 20:40:18 --> I: RRC mode: Connected
2023-01-31 20:40:22 --> +CSCON: 0
2023-01-31 20:40:22 --> I: RRC mode: Idle
2023-01-31 20:40:22 --> I: Received UDP data, length: 6, data: led-on
2023-01-31 20:40:22 --> I: Handling UDP data, data: led-on
2023-01-31 20:40:48 --> I: Transmitting sensor data to IoTCreators cloud
2023-01-31 20:40:48 --> I: Sending UDP payload, length: 36, data: {"Temp":24.684901,"Humid":12.389022}
2023-01-31 20:40:48 --> +CSCON: 1
2023-01-31 20:40:48 --> I: RRC mode: Connected
2023-01-31 20:40:52 --> +CSCON: 0
2023-01-31 20:40:52 --> I: RRC mode: Idle
2023-01-31 20:41:18 --> I: Transmitting sensor data to IoTCreators cloud
2023-01-31 20:41:18 --> I: Sending UDP payload, length: 36, data: {"Temp":24.679560,"Humid":12.350875}
2023-01-31 20:41:18 --> +CSCON: 1
2023-01-31 20:41:18 --> I: RRC mode: Connected
2023-01-31 20:41:22 --> +CSCON: 0
2023-01-31 20:41:22 --> I: RRC mode: Idle
2023-01-31 20:41:27 --> I: Received UDP data, length: 7, data: led-off
2023-01-31 20:41:27 --> I: Handling UDP data, data: led-off

```

