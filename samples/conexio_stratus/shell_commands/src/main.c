/*
 * Copyright (c) 2025 Conexio Technologies, Inc.
 * Author: Rajeev Piyare <rajeev@conexiotech.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Shell Commands Sample for Conexio Stratus Pro (nRF9151)
 *
 * Demonstrates:
 *  - Registering root shell commands with SHELL_CMD_REGISTER
 *  - Nested subcommands with SHELL_STATIC_SUBCMD_SET_CREATE
 *  - Argument parsing with argc/argv
 *  - GPIO control via shell commands
 *  - Stateful commands with static variables
 *  - shell_print / shell_error / shell_warn output helpers
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/shell/shell.h>
#include <ncs_version.h>
#include <version.h>
#include <stdlib.h>   /* strtol */

/* ── Board-specific definitions ─────────────────────────────────────── */

/* LED0: gpio0 pin 25 (active-high output) */
#define LED_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED_NODE, gpios);

/* GPIO0 device for generic pin reads */
#define GPIO0_NODE DT_NODELABEL(gpio0)
static const struct device *gpio0_dev;

/* ── Static state ────────────────────────────────────────────────────── */

static uint32_t counter_val; /* incremented by "stratus counter" */

/* ── Helper: ensure LED GPIO is ready ───────────────────────────────── */
static bool led_is_ready(void)
{
	return gpio_is_ready_dt(&led);
}

/* ════════════════════════════════════════════════════════════════════════
 * LED nested subcommands: stratus led {on|off|toggle}
 * ════════════════════════════════════════════════════════════════════════ */

static int cmd_led_on(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (!led_is_ready()) {
		shell_error(sh, "LED GPIO device not ready");
		return -ENODEV;
	}

	gpio_pin_set_dt(&led, 1);
	shell_print(sh, "LED turned ON (gpio0 pin %d)", led.pin);
	return 0;
}

static int cmd_led_off(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (!led_is_ready()) {
		shell_error(sh, "LED GPIO device not ready");
		return -ENODEV;
	}

	gpio_pin_set_dt(&led, 0);
	shell_print(sh, "LED turned OFF (gpio0 pin %d)", led.pin);
	return 0;
}

static int cmd_led_toggle(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (!led_is_ready()) {
		shell_error(sh, "LED GPIO device not ready");
		return -ENODEV;
	}

	gpio_pin_toggle_dt(&led);
	shell_print(sh, "LED toggled (gpio0 pin %d)", led.pin);
	return 0;
}

/* Nested sub-command set for the "led" subcommand */
SHELL_STATIC_SUBCMD_SET_CREATE(sub_led,
	SHELL_CMD(on,     NULL, "Turn LED on",    cmd_led_on),
	SHELL_CMD(off,    NULL, "Turn LED off",   cmd_led_off),
	SHELL_CMD(toggle, NULL, "Toggle LED",     cmd_led_toggle),
	SHELL_SUBCMD_SET_END
);

/* ════════════════════════════════════════════════════════════════════════
 * stratus info
 * ════════════════════════════════════════════════════════════════════════ */

static int cmd_stratus_info(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "Board      : %s", CONFIG_BOARD);
	shell_print(sh, "NCS version: %s", NCS_VERSION_STRING);
	shell_print(sh, "Uptime     : %lld ms", k_uptime_get());
	return 0;
}

/* ════════════════════════════════════════════════════════════════════════
 * stratus gpio read <pin>
 *
 * Reads a gpio0 pin number passed as the first positional argument.
 * The pin is configured as input with an internal pull-up before reading.
 * ════════════════════════════════════════════════════════════════════════ */

static int cmd_gpio_read(const struct shell *sh, size_t argc, char **argv)
{
	/* Require exactly one argument: the pin number */
	if (argc != 2) {
		shell_error(sh, "Usage: stratus gpio read <pin>");
		return -EINVAL;
	}

	char *end;
	long pin = strtol(argv[1], &end, 10);

	if (*end != '\0' || pin < 0 || pin > 31) {
		shell_error(sh, "Invalid pin number '%s'. Expected 0-31.", argv[1]);
		return -EINVAL;
	}

	if (!device_is_ready(gpio0_dev)) {
		shell_error(sh, "gpio0 device not ready");
		return -ENODEV;
	}

	/* Configure as input with pull-up (matches button0 default) */
	int ret = gpio_pin_configure(gpio0_dev, (gpio_pin_t)pin,
				     GPIO_INPUT | GPIO_PULL_UP);
	if (ret < 0) {
		shell_error(sh, "Failed to configure pin %ld as input: %d", pin, ret);
		return ret;
	}

	int val = gpio_pin_get(gpio0_dev, (gpio_pin_t)pin);
	if (val < 0) {
		shell_error(sh, "Failed to read pin %ld: %d", pin, val);
		return val;
	}

	shell_print(sh, "gpio0 pin %-2ld = %d (%s)", pin, val,
		    val ? "HIGH" : "LOW");
	return 0;
}

/* Sub-command set for "gpio" — only "read" for now */
SHELL_STATIC_SUBCMD_SET_CREATE(sub_gpio,
	SHELL_CMD_ARG(read, NULL,
		      "Read a gpio0 pin\nUsage: stratus gpio read <pin 0-31>",
		      cmd_gpio_read, 2, 0),
	SHELL_SUBCMD_SET_END
);

/* ════════════════════════════════════════════════════════════════════════
 * stratus counter
 *
 * Prints and increments a static counter — shows stateful commands.
 * ════════════════════════════════════════════════════════════════════════ */

static int cmd_counter(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "Counter: %u (incremented from %u)", counter_val + 1,
		    counter_val);
	counter_val++;
	return 0;
}

/* ════════════════════════════════════════════════════════════════════════
 * stratus reset
 *
 * Issues a cold reboot after printing a warning.
 * ════════════════════════════════════════════════════════════════════════ */

static int cmd_reset(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_warn(sh, "Rebooting device in 1 second...");
	k_sleep(K_SECONDS(1));
	sys_reboot(SYS_REBOOT_COLD);

	/* Never reached */
	return 0;
}

/* ════════════════════════════════════════════════════════════════════════
 * "stratus" root command with all subcommands
 *
 * SHELL_STATIC_SUBCMD_SET_CREATE builds a compile-time table of
 * subcommands that the shell engine uses for tab-completion and dispatch.
 * ════════════════════════════════════════════════════════════════════════ */

SHELL_STATIC_SUBCMD_SET_CREATE(sub_stratus,
	SHELL_CMD(info,    NULL,      "Show board info, NCS version and uptime",
		  cmd_stratus_info),
	SHELL_CMD(led,     &sub_led,  "LED control: on | off | toggle",
		  NULL),
	SHELL_CMD(gpio,    &sub_gpio, "GPIO operations: read <pin>",
		  NULL),
	SHELL_CMD(counter, NULL,      "Print and increment a persistent counter",
		  cmd_counter),
	SHELL_CMD(reset,   NULL,      "Perform a cold system reboot",
		  cmd_reset),
	SHELL_SUBCMD_SET_END
);

/*
 * SHELL_CMD_REGISTER vs SHELL_CMD_ARG_REGISTER
 * ─────────────────────────────────────────────
 * SHELL_CMD_REGISTER(name, subcmds, help, handler)
 *   Registers a root command that takes NO positional arguments beyond
 *   its subcommand tree.  Good for commands whose children do the work.
 *
 * SHELL_CMD_ARG_REGISTER(name, subcmds, help, handler, mandatory, optional)
 *   Registers a root command that itself accepts positional arguments.
 *   The shell validates that at least `mandatory` args are present.
 *   Use when the handler function directly needs argv[1], argv[2], etc.
 */
SHELL_CMD_REGISTER(stratus, &sub_stratus,
		   "Conexio Stratus Pro commands", NULL);

/* ════════════════════════════════════════════════════════════════════════
 * "version" root command — no subcommands, prints firmware version info
 * ════════════════════════════════════════════════════════════════════════ */

static int cmd_version(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "Firmware version info");
	shell_print(sh, "  Board      : %s", CONFIG_BOARD);
	shell_print(sh, "  Zephyr     : %s", KERNEL_VERSION_STRING);
	shell_print(sh, "  Build date : " __DATE__ " " __TIME__);
	return 0;
}

SHELL_CMD_REGISTER(version, NULL, "Print firmware version information",
		   cmd_version);

/* ════════════════════════════════════════════════════════════════════════
 * main — initialise hardware and announce the shell
 * ════════════════════════════════════════════════════════════════════════ */

int main(void)
{
	int ret;

	printk("\n");
	printk("=========================================\n");
	printk("  Conexio Stratus Pro — Shell Commands\n");
	printk("  Board: %s\n", CONFIG_BOARD);
	printk("=========================================\n");

	/* Initialise gpio0 device handle used by "stratus gpio read" */
	gpio0_dev = DEVICE_DT_GET(GPIO0_NODE);
	if (!device_is_ready(gpio0_dev)) {
		printk("ERROR: gpio0 device not ready\n");
	}

	/* Configure LED as output, initially off */
	if (!led_is_ready()) {
		printk("ERROR: LED GPIO device not ready\n");
	} else {
		ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			printk("ERROR: Failed to configure LED pin: %d\n", ret);
		} else {
			printk("LED initialised on gpio0 pin %d\n", led.pin);
		}
	}

	printk("\nShell is ready. Type 'stratus' or 'help' to get started.\n");
	printk("  stratus info          — board/version/uptime\n");
	printk("  stratus led on/off/toggle\n");
	printk("  stratus gpio read <pin>\n");
	printk("  stratus counter\n");
	printk("  stratus reset\n");
	printk("  version\n\n");

	/*
	 * The Zephyr shell runs in its own thread started automatically when
	 * CONFIG_SHELL=y.  main() can return here; the shell keeps running.
	 */
	return 0;
}
