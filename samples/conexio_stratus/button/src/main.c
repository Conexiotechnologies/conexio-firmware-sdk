/*
 * Copyright (c) 2024 Conexio Technologies, Inc
 *
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
/* Include the dk button library */
#include <dk_buttons_and_leds.h>

#define GPIO_NODE	DT_NODELABEL(gpio0)

/* The devicetree node identifier for the "led0" alias. */
#define LED DT_ALIAS(led0)

static struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED, gpios);
static const struct device *gpio_dev;

LOG_MODULE_REGISTER(app);

/* Set the sleep time */
#define SLEEP_TIME_MS 	1000
#define LED_ON_TIME_MS 	2000

#define LED_OFF 0
#define LED_ON !LED_OFF

/* Define the button handler function */
static void button_handler(uint32_t button_states, uint32_t has_changed)
{
	/* Check for the button state */
	if (has_changed & button_states & BIT(CONFIG_BUTTON_EVENT_BTN_NUM - 1)) {
		printk("Button pressed\n");
		/* Blink the LED on Stratus board */
		gpio_pin_set_dt(&led, LED_ON);
		k_msleep(LED_ON_TIME_MS);
		gpio_pin_set_dt(&led, LED_OFF);
	}
}

void init_led(void)
{
	gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
}

int main(void)
{
	printk("Button and Blink sample app for %s\n", CONFIG_BOARD);

	gpio_dev = DEVICE_DT_GET(GPIO_NODE);
	if (!gpio_dev) {
		printk("Error getting GPIO device binding\r\n");
		return false;
	}

	/* Initialize the LED pin */
	init_led();

	/* Initialize the button handler */
	dk_buttons_init(button_handler);

	while (1) {
		k_msleep(SLEEP_TIME_MS);
	}

	return 0;
}
