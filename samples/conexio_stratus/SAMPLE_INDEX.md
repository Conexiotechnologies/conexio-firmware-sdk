# Conexio Stratus Pro — Sample Index

A quick-reference guide to the educational samples included in this SDK,
grouped by topic. Each sample is a standalone, heavily commented template
you can copy and adapt for your own application.

---

## Group 1 — RTOS Foundations

| Sample | Key Concepts |
|--------|-------------|
| [`rtos_threads`](rtos_threads/) | `K_THREAD_DEFINE`, `k_thread_create`, `K_MSGQ_DEFINE`, `K_MUTEX_DEFINE`, `K_SEM_DEFINE` — sensor + consumer + heartbeat threads communicating via queue |
| [`work_queue`](work_queue/) | System vs custom work queue, `k_work`, `k_work_delayable`, ISR→workqueue pattern with GPIO button, work chaining |
| [`timers`](timers/) | One-shot, periodic, stop-callback timers, `k_timer_status_sync`, `k_timer_remaining_get`, ISR context rules documented thoroughly |

> See [`RTOS_FOUNDATIONS.md`](RTOS_FOUNDATIONS.md) for a detailed breakdown of every concept with code snippets and decision tables.

---

## Group 2 — Developer Tooling

| Sample | Key Concepts |
|--------|-------------|
| [`shell_commands`](shell_commands/) | `SHELL_CMD_REGISTER`, nested subcommands, argument parsing, GPIO from shell, `shell_print` vs `printk`, `sys_reboot` |
| [`logging_levels`](logging_levels/) | All 4 log levels, `LOG_HEXDUMP_INF/DBG`, `log_filter_set` runtime filtering, deferred vs immediate mode, `LOG_MODULE_DECLARE` |
| [`nvs_storage`](nvs_storage/) | `nvs_mount`, `nvs_write`, `nvs_read`, `nvs_delete`, boot counter, string, struct, binary blob, flash wear notes |

---

## Group 3 — Hardware Peripherals

| Sample | Key Concepts |
|--------|-------------|
| [`adc_read`](adc_read/) | `ADC_DT_SPEC_GET_BY_NAME`, `adc_channel_setup_dt`, `adc_read_dt`, `adc_raw_to_millivolts_dt`, HW + SW oversampling on AIN0/P0.13 |
| [`watchdog`](watchdog/) | `wdt_install_timeout`, `wdt_setup`, `wdt_feed`, last-gasp callback, button-triggered reset demo, `CONFIG_WDT_DEMO_TRIGGER_RESET` opt-in |
| [`rtc_alarm`](rtc_alarm/) | Counter API (RTC2), `counter_set_channel_alarm`, `counter_us_to_ticks`, one-shot + periodic + wake-from-system-off (`CONFIG_RTC_DEMO_SYSTEM_OFF` opt-in) |

---

## Group 4 — Communication Protocols

| Sample | Key Concepts |
|--------|-------------|
| [`i2c_read_write`](i2c_read_write/) | `i2c_write_read_dt`, `i2c_write_dt`, `i2c_read_dt`, `i2c_transfer_dt`, burst read, LIS2DH WHO_AM_I, EEPROM write/read |
| [`spi_read_write`](spi_read_write/) | `spi_write_dt`, `spi_read_dt`, `spi_transceive_dt`, scatter-gather, split cmd/response, loopback self-test |

---

## Group 5 — Storage

| Sample | Key Concepts |
|--------|-------------|
| [`nvs_storage`](nvs_storage/) | NVS key-value store, flash wear, `nvs_calc_free_space`, struct persistence, delete |
| [`ext_flash`](ext_flash/) | `eeprom_write`, `eeprom_read`, `eeprom_get_size`, 24CW160 EEPROM, write cycle timing, erase pattern |

---

## Group 6 — Power Management

| Sample | Key Concepts |
|--------|-------------|
| [`low_power_mode`](low_power_mode/) | `lte_lc_power_off`, `NRF_REGULATORS->SYSTEMOFF`, `sys_poweroff`, UART RX pin delete, ~8.9 µA idle |
| [`rtc_alarm`](rtc_alarm/) | Wake from system-off via RTC alarm (`CONFIG_RTC_DEMO_SYSTEM_OFF`) |

---

## Group 7 — Sensors & Actuators

| Sample | Key Concepts |
|--------|-------------|
| [`accelerometer`](accelerometer/) | LIS2DH via Zephyr sensor API, `sensor_sample_fetch`, `sensor_channel_get` |
| [`bme280`](bme280/) | BME280 temp/pressure/humidity over I2C |
| [`bme680`](bme680/) | BME680 environmental sensor |
| [`dps310`](dps310/) | DPS310 barometric pressure and temperature |
| [`sht3xd`](sht3xd/) | SHT3x humidity and temperature |
| [`sht4x`](sht4x/) | SHT4x humidity and temperature |
| [`sht4x_power_gate`](sht4x_power_gate/) | SHT4x with GPIO power gating (GPIO 26 / SENSOR_PWR_EN) |
| [`multi_sensor`](multi_sensor/) | LIS2DH + SHT4x combined, selective enabling via Kconfig |
| [`adc_read`](adc_read/) | Raw ADC on SAADC AIN0, `adc_raw_to_millivolts_dt` |
| [`battery_adc`](battery_adc/) | Battery voltage via ADC + voltage divider, discharge curve |
| [`npm1300_fuel_gauge`](npm1300_fuel_gauge/) | nPM1300 PMIC fuel gauge, SoC bar indicator, TTE/TTF, VBUS events |

---

## Group 8 — GPIO & PWM

| Sample | Key Concepts |
|--------|-------------|
| [`led_blink`](led_blink/) | GPIO output, `gpio_pin_toggle_dt`, `led0` alias |
| [`button`](button/) | GPIO input, button interrupt, LED response |
| [`digital_read`](digital_read/) | `gpio_pin_get_dt`, `pininput` alias, pull-up configuration |
| [`digital_write`](digital_write/) | `gpio_pin_set_dt`, `gpiopin` alias |
| [`digital_read_write`](digital_read_write/) | Combined input + output, mirroring input state to output pin |
| [`pwm`](pwm/) | `pwm_set_dt`, duty cycle, `pwm-led0` alias |

---

## Group 9 — Cellular & Connectivity

| Sample | Key Concepts |
|--------|-------------|
| [`cellular_test`](cellular_test/) | 9-phase AT command test suite, `nrf_modem_lib`, network registration, signal quality |
| [`at_client`](at_client/) | Transparent AT proxy over UART, nRF Connect Serial Terminal compatible |
| [`udp`](udp/) | UDP socket, PSM, eDRX, RAI, current measurement patterns |
| [`mqtt_control`](mqtt_control/) | MQTT publish/subscribe, LED control over MQTT, button events |
| [`datacake`](datacake/) | Datacake cloud integration over MQTT |
| [`gnss`](gnss/) | GNSS fix acquisition, NMEA output, A-GPS |
| [`wifi_scan`](wifi_scan/) | Wi-Fi scan on nRF9151, SSID/RSSI reporting |
| [`cellular_modem_battery_monitor`](cellular_modem_battery_monitor/) | Combined cellular + battery ADC monitoring |

---

## Group 10 — System & OTA

| Sample | Key Concepts |
|--------|-------------|
| [`device_info`](device_info/) | IMEI, modem firmware version, RSRP, reset reason |
| [`full_modem_firmware_update`](full_modem_firmware_update/) | SMP server, full modem DFU via UART, `update_modem.py` |
| [`http_update/application_update`](http_update/application_update/) | HTTPS FOTA from GitHub, two-step version check (`version.txt` → `zephyr.signed.bin`), `download_client` + `fota_download`, MCUboot image header version comparison, ISRG Root X1 TLS cert provisioning |
| [`empty_template`](empty_template/) | Bare-bones starting point for new applications |
| [`npm_leds`](npm_leds/) | nPM1300 LED driver, `charging`/`error`/`host` modes, `user_led_set()` |

---

## Build & Flash Reference

```bash
# Build (replace <folder> and <project> with the sample names above)
west build -b conexio_stratus_pro/nrf9151/ns \
    samples/conexio_stratus/<folder> \
    -- -DBOARD_ROOT=/opt/nordic/ncs/v2.9.0/conexio-firmware-sdk

# Flash
newtmgr -c serial image upload \
    build/<project>/zephyr/zephyr.signed.bin
```

The `<project>` name matches the `project()` call in each sample's
`CMakeLists.txt` and is listed in the build output path shown in each
sample's README.
