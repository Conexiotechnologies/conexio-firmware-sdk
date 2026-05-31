# Logging Levels Sample

**Board:** Conexio Stratus Pro (nRF9151)  
**Author:** Rajeev Piyare \<rajeev@conexiotech.com\>  
**License:** Apache-2.0

Demonstrates the Zephyr Logging subsystem, covering all four severity
levels, hexdump helpers, runtime log level filtering, deferred vs
immediate mode, and the `LOG_MODULE_DECLARE` pattern.

---

## Building

```bash
west build -b conexio_stratus_pro/nrf9151/ns -- \
  -DBOARD_ROOT=/opt/nordic/ncs/v3.2.1/conexio-firmware-sdk
```

## Flashing (MCUboot via newtmgr)

```bash
newtmgr -c serial image upload \
  build/logging_levels/zephyr/zephyr.signed.bin
```

---

## Log Levels

Zephyr defines four severity levels (lowest to highest verbosity):

| Level | Macro | Value | When to use |
|-------|-------|-------|-------------|
| Error | `LOG_ERR(...)` | 1 | Unrecoverable failures that need developer attention immediately. E.g., hardware init failure, assertion violation. |
| Warning | `LOG_WRN(...)` | 2 | Unexpected but non-fatal situations. E.g., sensor value out of calibrated range, retry attempt. |
| Info | `LOG_INF(...)` | 3 | Normal operational status. E.g., boot complete, connection established, periodic heartbeat. |
| Debug | `LOG_DBG(...)` | 4 | Fine-grained developer detail. E.g., raw register values, internal state transitions. Disable in production. |

---

## Setting the Default Log Level

`CONFIG_LOG_DEFAULT_LEVEL` in `prj.conf` controls the compile-time
floor for all modules that do not set their own level:

```kconfig
# prj.conf
CONFIG_LOG_DEFAULT_LEVEL=4   # show everything (DBG and above)
CONFIG_LOG_DEFAULT_LEVEL=3   # show INF, WRN, ERR
CONFIG_LOG_DEFAULT_LEVEL=2   # show WRN, ERR only
CONFIG_LOG_DEFAULT_LEVEL=1   # show ERR only
CONFIG_LOG_DEFAULT_LEVEL=0   # logging off
```

A module can override this with its second argument to
`LOG_MODULE_REGISTER`:

```c
/* This module always emits DBG regardless of CONFIG_LOG_DEFAULT_LEVEL */
LOG_MODULE_REGISTER(my_driver, LOG_LEVEL_DBG);
```

---

## Runtime Filtering

When `CONFIG_LOG_RUNTIME_FILTERING=y`, you can change the active log
level for any registered module at runtime without recompiling:

```c
#include <zephyr/logging/log_ctrl.h>

/* Suppress all DBG and INF messages from app_main at runtime */
log_filter_set(
    NULL,                    /* NULL = apply to all backends */
    Z_LOG_LOCAL_DOMAIN_ID,   /* local domain (single-core) */
    log_source_id_get("app_main"),
    LOG_LEVEL_WRN            /* new minimum level */
);
```

This is useful for reducing verbosity in production without losing the
ability to re-enable debug output in the field.

---

## Hexdump Helpers

Log a raw byte buffer with both a text label and hex/ASCII columns:

```c
uint8_t buf[] = {0xDE, 0xAD, 0xBE, 0xEF};

LOG_HEXDUMP_INF(buf, sizeof(buf), "Received frame:");
LOG_HEXDUMP_DBG(buf, sizeof(buf), "Same frame at debug:");
LOG_HEXDUMP_WRN(buf, sizeof(buf), "Unexpected payload:");
LOG_HEXDUMP_ERR(buf, sizeof(buf), "Corrupt header:");
```

Sample output:

```
[00:00:00.123,456] <inf> app_main: Received frame:
                                   de ad be ef                      |....|
```

---

## Deferred vs Immediate Logging

### Deferred mode (`CONFIG_LOG_MODE_DEFERRED`, default)

Log calls enqueue messages into a ring buffer and return immediately.
A dedicated background thread drains the buffer and writes to backends.

- Low latency impact on the calling thread
- Small delay before messages appear on the terminal
- Suitable for production use and normal debugging

### Immediate mode (`CONFIG_LOG_MODE_IMMEDIATE`)

Log calls block until the message has been fully written to all
active backends before returning.

- Higher per-call overhead
- Zero buffering — messages appear synchronously
- Useful for debugging boot failures or hard faults where deferred
  output might be lost before the crash

To enable immediate mode:

```kconfig
# prj.conf
CONFIG_LOG_MODE_IMMEDIATE=y
```

---

## Backend Configuration

This sample uses the UART backend:

```kconfig
CONFIG_LOG_BACKEND_UART=y
CONFIG_LOG_BACKEND_UART_OUTPUT_TEXT=y   # human-readable text format
```

Other available backends:

| Backend | Kconfig option | Notes |
|---------|---------------|-------|
| UART | `CONFIG_LOG_BACKEND_UART` | Default; outputs to the debug UART |
| RTT  | `CONFIG_LOG_BACKEND_RTT`  | Segger J-Link RTT; very low overhead |
| Shell | `CONFIG_SHELL_LOG_BACKEND` | Displays logs inside the shell terminal |
| Flash | `CONFIG_LOG_BACKEND_FLASH` | Persists logs to flash for post-mortem |

---

## `LOG_MODULE_DECLARE` Pattern

`LOG_MODULE_REGISTER` creates a new log source and must appear once
per `.c` file.  `LOG_MODULE_DECLARE` references an already-registered
source — use it in header files or to group functions within the same
file under a shared log source:

```c
/* subsystem_a.h — shared across multiple .c files */
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(subsystem_a, LOG_LEVEL_DBG);
```

For truly independent log sources with separate runtime-filterable
identities, create separate compilation units and call
`LOG_MODULE_REGISTER` in each.

---

## Expected Output

### At `CONFIG_LOG_DEFAULT_LEVEL=4` (full verbosity)

```
=========================================
  Conexio Stratus Pro — Logging Levels Demo
  Board: conexio_stratus_pro
=========================================

Log level guide:
  LOG_LEVEL_ERR (1) — critical failures only
  LOG_LEVEL_WRN (2) — warnings + errors
  LOG_LEVEL_INF (3) — normal status + above
  LOG_LEVEL_DBG (4) — all messages (verbose)

[00:00:00.012,000] <inf> app_main: === Phase 1: Running at LOG_LEVEL_DBG ===
[00:00:00.013,000] <inf> app_main: System started, uptime: 13 ms
[00:00:00.014,000] <dbg> app_main: Raw ADC value: 0x0800, converted: 652 mV
[00:00:00.015,000] <inf> app_main: Iteration 0: sensor value = 50, uptime = 15 ms
[00:00:00.215,000] <dbg> app_main: Raw ADC value: 0x0810, converted: 659 mV
[00:00:00.216,000] <inf> app_main: Iteration 1: sensor value = 65, uptime = 216 ms
...
[00:00:00.814,000] <wrn> app_main: Sensor value 95 out of expected range [0-90]
[00:00:00.815,000] <err> app_main: Failed to initialise sensor on iteration 4, err -5
[00:00:00.816,000] <inf> app_main: --- Hexdump demo (INF level) ---
[00:00:00.817,000] <inf> app_main: Sensor frame bytes:
                                   aa bb 01 02 03 ff 7f 00  de ad be ef ca fe ba be |................|
[00:00:00.818,000] <dbg> app_main: Same frame at DBG:
                                   aa bb 01 02 03 ff 7f 00  de ad be ef ca fe ba be |................|
[00:00:00.819,000] <wrn> app_main: === Phase 2: Switching runtime level to LOG_LEVEL_WRN ===
[00:00:00.820,000] <inf> app_main: (This INF message is the last you will see until level is restored)
```

### After `log_filter_set(..., LOG_LEVEL_WRN)` — Phase 2

Only WRN and ERR messages appear (DBG and INF are suppressed):

```
[00:00:01.220,000] <wrn> app_main: Sensor value 95 out of expected range [0-90]
[00:00:01.620,000] <err> app_main: Failed to initialise sensor on iteration 4, err -5
```

### After level restored to DBG — Phase 3

```
[00:00:02.021,000] <inf> app_main: === Phase 3: Level restored to LOG_LEVEL_DBG ===
[00:00:02.022,000] <dbg> app_main: DBG messages are visible again
[00:00:02.023,000] <inf> app_main: Total uptime: 2023 ms
[00:00:02.024,000] <inf> app_main: Logging demo complete. Halting.
```

### At `CONFIG_LOG_DEFAULT_LEVEL=1` (errors only)

Recompile with `CONFIG_LOG_DEFAULT_LEVEL=1` to see only:

```
[00:00:00.815,000] <err> app_main: Failed to initialise sensor on iteration 4, err -5
[00:00:01.620,000] <err> app_main: Failed to initialise sensor on iteration 4, err -5
```

---

## Configuration (`prj.conf`)

| Option | Purpose |
|--------|---------|
| `CONFIG_LOG=y` | Enable the logging subsystem |
| `CONFIG_LOG_DEFAULT_LEVEL=4` | Default level for all modules (4=DBG) |
| `CONFIG_LOG_RUNTIME_FILTERING=y` | Allow `log_filter_set()` at runtime |
| `CONFIG_LOG_BACKEND_UART=y` | Send log output to UART |
| `CONFIG_LOG_BACKEND_UART_OUTPUT_TEXT=y` | Human-readable text format |
| `CONFIG_BOOTLOADER_MCUBOOT=y` | Generate MCUboot-compatible image |
