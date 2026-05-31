# Shell Commands Sample

**Board:** Conexio Stratus Pro (nRF9151)  
**Author:** Rajeev Piyare \<rajeev@conexiotech.com\>  
**License:** Apache-2.0

Demonstrates the Zephyr Shell subsystem on the Conexio Stratus Pro,
including nested subcommands, argument parsing, GPIO control, stateful
commands, and safe system reboot from a shell handler.

---

## Hardware

| Peripheral | GPIO     | Pin |
|-----------|----------|-----|
| LED0      | gpio0    | 25  |
| BUTTON0   | gpio0    | 31  | (pull-up, active-low — for reference)

---

## Building

```bash
west build -b conexio_stratus_pro/nrf9151/ns -- \
  -DBOARD_ROOT=/opt/nordic/ncs/v3.2.1/conexio-firmware-sdk
```

## Flashing (MCUboot via newtmgr)

```bash
newtmgr -c serial image upload \
  build/shell_commands/zephyr/zephyr.signed.bin
```

---

## Connecting to the Shell

Use any serial terminal at **115200 8N1**.  Examples:

```bash
# screen
screen /dev/ttyACM0 115200

# minicom
minicom -D /dev/ttyACM0 -b 115200

# picocom
picocom /dev/ttyACM0 -b 115200
```

Press **Enter** after connecting to get the `uart:~$` prompt.  
Use **Tab** to autocomplete commands and subcommands.

---

## Available Commands

### `stratus info`

Prints board name, NCS kernel version string, and device uptime.

```
uart:~$ stratus info
Board      : conexio_stratus_pro
NCS version: 3.7.99
Uptime     : 4321 ms
```

### `stratus led on | off | toggle`

Controls LED0 (gpio0 pin 25).

```
uart:~$ stratus led on
LED turned ON (gpio0 pin 25)

uart:~$ stratus led toggle
LED toggled (gpio0 pin 25)

uart:~$ stratus led off
LED turned OFF (gpio0 pin 25)
```

### `stratus gpio read <pin>`

Reads any gpio0 pin (0–31).  The pin is configured as input with an
internal pull-up before sampling.

```
uart:~$ stratus gpio read 31
gpio0 pin 31 = 1 (HIGH)

uart:~$ stratus gpio read 25
gpio0 pin 25 = 0 (LOW)

uart:~$ stratus gpio read 99
error: Invalid pin number '99'. Expected 0-31.
```

Missing or non-numeric arguments produce a usage error:

```
uart:~$ stratus gpio read
error: Usage: stratus gpio read <pin>
```

### `stratus counter`

Prints and increments a persistent counter (resets on reboot).

```
uart:~$ stratus counter
Counter: 1 (incremented from 0)

uart:~$ stratus counter
Counter: 2 (incremented from 1)
```

### `stratus reset`

Issues a warning and performs a cold system reboot after 1 second.

```
uart:~$ stratus reset
warning: Rebooting device in 1 second...
[device reboots]
```

### `version`

Standalone root command that prints firmware version details.

```
uart:~$ version
Firmware version info
  Board      : conexio_stratus_pro
  Zephyr     : 3.7.99
  Build date : Jan  1 2025 12:00:00
```

### Built-in shell commands

| Command | Description |
|---------|-------------|
| `help`  | List all registered commands |
| `clear` | Clear terminal screen |
| `log`   | Runtime log level control (from shell log backend) |

---

## Key Concepts

### `SHELL_CMD_REGISTER` vs `SHELL_CMD_ARG_REGISTER`

**`SHELL_CMD_REGISTER(name, subcmds, help, handler)`**  
Registers a root command whose handler delegates work to subcommands.
Use when the root command itself does not process positional arguments —
the shell dispatches to a child in `subcmds` automatically.

```c
SHELL_CMD_REGISTER(stratus, &sub_stratus, "Stratus commands", NULL);
```

**`SHELL_CMD_ARG_REGISTER(name, subcmds, help, handler, mandatory, optional)`**  
Registers a root command that *itself* accepts positional arguments.
The shell validates that at least `mandatory` extra arguments are
present before calling `handler`.

```c
/* "mycommand <required_arg> [optional_arg]" */
SHELL_CMD_ARG_REGISTER(mycommand, NULL, "Help text",
                       cmd_handler, 1, 1);
```

Use `SHELL_CMD_ARG` (inside a `SHELL_STATIC_SUBCMD_SET_CREATE`) for
the same effect on nested subcommands — see `cmd_gpio_read`.

### `shell_print` vs `printk`

| Function | When to use |
|----------|-------------|
| `shell_print(sh, ...)` | Inside a shell command handler. Output is routed through the active shell backend and is flushed atomically with the command response. |
| `shell_error(sh, ...)` | Report an error from within a handler. Prefixes the message with `error:` in the terminal. |
| `shell_warn(sh, ...)`  | Report a non-fatal warning. Prefixes with `warning:`. |
| `printk(...)`          | Early boot messages or code paths that run outside any shell handler context. Output always goes to the UART console. |

`shell_print` ensures the message appears in the right shell instance
(useful when multiple shell backends are active, e.g., UART + USB CDC).
Prefer it over `printk` inside handlers.

### Nested subcommands

```c
/* Innermost level */
SHELL_STATIC_SUBCMD_SET_CREATE(sub_led,
    SHELL_CMD(on,     NULL, "Turn LED on",  cmd_led_on),
    SHELL_CMD(off,    NULL, "Turn LED off", cmd_led_off),
    SHELL_CMD(toggle, NULL, "Toggle LED",   cmd_led_toggle),
    SHELL_SUBCMD_SET_END
);

/* Mid level references &sub_led */
SHELL_STATIC_SUBCMD_SET_CREATE(sub_stratus,
    SHELL_CMD(led, &sub_led, "LED control", NULL),
    ...
);

/* Root registration */
SHELL_CMD_REGISTER(stratus, &sub_stratus, "Stratus commands", NULL);
```

The shell engine builds a depth-first command tree.  Tab-completion
works at every level.

---

## Configuration (`prj.conf`)

| Option | Purpose |
|--------|---------|
| `CONFIG_SHELL=y` | Enable the shell subsystem |
| `CONFIG_SHELL_BACKEND_SERIAL=y` | Route shell I/O over UART |
| `CONFIG_SHELL_LOG_BACKEND=y` | Display log messages inside the shell terminal |
| `CONFIG_LOG=y` | Enable the logging subsystem |
| `CONFIG_GPIO=y` | Enable GPIO driver |
| `CONFIG_BOOTLOADER_MCUBOOT=y` | Generate MCUboot-compatible image |
