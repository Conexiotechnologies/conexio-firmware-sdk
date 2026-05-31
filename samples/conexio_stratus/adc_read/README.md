# ADC Read Sample — Conexio Stratus Pro (nRF9151)

Demonstrates the Zephyr DT-based ADC API on the nRF9151 SAADC peripheral.
Reads a 0–3.3 V analog signal from the stratus_header pin **D13/A0 (P0.13, AIN0)**,
converts the raw reading to millivolts, and shows both single-shot and
software-averaged results every second.

---

## Hardware Setup

| Stratus Header Pin | nRF9151 Pin | SAADC Input | Signal            |
|--------------------|-------------|-------------|-------------------|
| D13 / A0           | P0.13       | AIN0        | Analog 0–3.3 V    |
| GND                | GND         | —           | Ground reference  |

Connect a **0–3.3 V** signal to D13/A0 and GND. Good sources:

* A 10 kΩ potentiometer wiper (ends connected to 3.3 V and GND).
* A sensor with a 0–3.3 V analog output (e.g., a light sensor, temperature
  sensor, or flex sensor).

> ⚠️ **Never exceed 3.3 V on the header analog pins.**

---

## nRF9151 SAADC Configuration

### Gain and Reference

The nRF9151 SAADC uses a fixed internal reference of **0.6 V**.
To cover the full 0–3.3 V header range, the channel is configured with:

| Parameter         | Value                | Meaning                         |
|-------------------|----------------------|---------------------------------|
| `zephyr,gain`     | `ADC_GAIN_1_6`       | Input attenuated by 1/6         |
| `zephyr,reference`| `ADC_REF_INTERNAL`   | 0.6 V internal reference        |
| Full-scale        | 0.6 V ÷ (1/6) = 3.6 V | Covers 0–3.3 V with headroom  |

The conversion formula applied by `adc_raw_to_millivolts_dt()`:

```
mV = (raw × Vref_mV × gain_denominator) / (2^resolution × gain_numerator)
   = (raw × 600 × 6) / (4096 × 1)
```

### Resolution and Oversampling

| Parameter           | Value | Effect                                      |
|---------------------|-------|---------------------------------------------|
| `zephyr,resolution` | 12    | 4096 counts (0–4095)                        |
| `zephyr,oversampling`| 4   | Hardware averages 2⁴ = 16 samples per read  |

Hardware oversampling reduces high-frequency noise without software cost.
The sample additionally averages 10 software reads, demonstrating combined
HW + SW oversampling for maximum noise rejection.

---

## Build and Flash

```bash
# Build
west build -b conexio_stratus_pro/nrf9151/ns -- \
    -DBOARD_ROOT=/opt/nordic/ncs/v3.2.1/conexio-firmware-sdk

# Flash via newtmgr (serial DFU)
newtmgr -c serial image upload build/adc_read/zephyr/zephyr.signed.bin
```

---

## Expected Output

```
*** Booting Zephyr OS build v3.7.99-ncs2 ***
[00:00:00.312,000] <inf> adc_read: === Conexio Stratus Pro — ADC Read Sample ===
[00:00:00.312,000] <inf> adc_read: Board: conexio_stratus_pro/nrf9151/ns
[00:00:00.312,000] <inf> adc_read: Connect a 0-3.3 V signal to header D13/A0 (P0.13) and GND.
[00:00:00.313,000] <inf> adc_read: ADC controller: adc@e000
[00:00:00.313,000] <inf> adc_read: Channel:        0
[00:00:00.313,000] <inf> adc_read: Resolution:     12 bits
[00:00:00.313,000] <inf> adc_read: Oversampling:   4 (hardware)
[00:00:00.313,000] <inf> adc_read: Gain:           1/6 (full-scale ~3.6 V for 3.3 V range)
[00:00:00.313,000] <inf> adc_read: Reference:      INTERNAL (0.6 V)
[00:00:00.314,000] <inf> adc_read: ADC channel configured successfully.
[00:00:01.315,000] <inf> adc_read: --- Sample #1 ---
[00:00:01.315,000] <inf> adc_read:   Raw value : 2248 (12-bit, HW oversampling x16)
[00:00:01.315,000] <inf> adc_read:   Voltage   : 1980 mV
[00:00:01.326,000] <inf> adc_read:   SW avg (10 samples):
[00:00:01.326,000] <inf> adc_read:     Avg raw : 2249
[00:00:01.326,000] <inf> adc_read:     Avg mV  : 1981 mV
[00:00:01.326,000] <inf> adc_read:   Note: HW oversampling (x16) + SW averaging (10) = effective noise reduction
[00:00:01.326,000] <inf> adc_read:   Ref voltage : 600 mV (internal)
[00:00:01.326,000] <inf> adc_read:   Resolution  : 12 bits
```

---

## Key APIs Used

| API                        | Purpose                                      |
|----------------------------|----------------------------------------------|
| `ADC_DT_SPEC_GET_BY_NAME`  | Resolve channel spec from DT at compile time |
| `adc_is_ready_dt()`        | Check ADC controller is ready                |
| `adc_channel_setup_dt()`   | Configure channel from DT parameters         |
| `adc_sequence_init_dt()`   | Initialize the read sequence from DT         |
| `adc_read_dt()`            | Take a single sample                         |
| `adc_raw_to_millivolts_dt()`| Convert raw count to mV using DT gain/ref   |

---

## Author

Copyright (c) 2025 Conexio Technologies, Inc.  
Rajeev Piyare \<rajeev@conexiotech.com\>  
SPDX-License-Identifier: Apache-2.0
