/*
 * Copyright (c) 2025 Conexio Technologies, Inc.
 * Author: Rajeev Piyare <rajeev@conexiotech.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Watchdog Sample — Conexio Stratus Pro (nRF9151)
 *
 * Demonstrates the Zephyr WDT API:
 *   - Installing a 5-second timeout with a last-gasp callback
 *   - Periodic feeding from a dedicated thread (every 2 s)
 *   - Button-triggered feed pause to demonstrate the watchdog reset
 *   - Optional automatic reset demo via CONFIG_WDT_DEMO_TRIGGER_RESET
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/drivers/gpio.h>

LOG_MODULE_REGISTER(watchdog_sample, LOG_LEVEL_DBG);

/* --------------------------------------------------------------------------
 * Watchdog device — alias watchdog0 → wdt0 is defined in the board DTSI
 * -------------------------------------------------------------------------- */
#define WDT_NODE DT_ALIAS(watchdog0)

/* --------------------------------------------------------------------------
 * Button — sw0 alias → gpio0 pin 31, active-low, pull-up (board DTSI)
 * -------------------------------------------------------------------------- */
#define BTN_NODE DT_ALIAS(sw0)
static const struct gpio_dt_spec button =
    GPIO_DT_SPEC_GET(BTN_NODE, gpios);
static struct gpio_callback btn_cb_data;

/* --------------------------------------------------------------------------
 * Watchdog feed thread
 * -------------------------------------------------------------------------- */
#define FEED_THREAD_STACK_SIZE 1024
#define FEED_THREAD_PRIORITY   5
#define FEED_INTERVAL_MS       2000   /* Feed every 2 s (window max = 5 s) */

K_THREAD_STACK_DEFINE(feed_thread_stack, FEED_THREAD_STACK_SIZE);
static struct k_thread feed_thread_data;

/* Shared flag — set by button ISR to pause feeding */
static volatile bool feed_paused;

/* WDT channel ID returned by wdt_install_timeout() */
static int wdt_channel_id = -1;

/* WDT device pointer — filled in main() */
static const struct device *wdt_dev;

/* --------------------------------------------------------------------------
 * WDT expiry callback (last-gasp, runs in ISR context)
 *
 * On most SoCs the callback fires shortly before the hard reset.
 * Keep it brief — no blocking calls, no dynamic allocation.
 * -------------------------------------------------------------------------- */
static void wdt_callback(const struct device *dev, int channel_id)
{
    ARG_UNUSED(dev);
    ARG_UNUSED(channel_id);

    /* printk() is safe from ISR context; LOG_* macros are not always */
    printk("WDT expiry callback! System will reset.\n");
}

/* --------------------------------------------------------------------------
 * Button ISR — pauses watchdog feeding so the WDT expires in ~5 s
 * -------------------------------------------------------------------------- */
static void button_isr(const struct device *dev, struct gpio_callback *cb,
                       gpio_port_pins_t pins)
{
    ARG_UNUSED(dev);
    ARG_UNUSED(cb);
    ARG_UNUSED(pins);

    if (!feed_paused) {
        feed_paused = true;
        /* ISR context — use printk, not LOG_INF */
        printk("WDT feed paused — system will reset in ~5s\n");
    }
}

/* --------------------------------------------------------------------------
 * Watchdog feed thread
 * -------------------------------------------------------------------------- */
static void watchdog_feed_thread(void *arg1, void *arg2, void *arg3)
{
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);
    ARG_UNUSED(arg3);

    uint32_t feed_count = 0;

    LOG_INF("Watchdog feed thread started (feed interval: %d ms, "
            "WDT timeout: 5000 ms)", FEED_INTERVAL_MS);

    while (1) {
        k_msleep(FEED_INTERVAL_MS);

        if (feed_paused) {
            /* Countdown to reset (approximate — 1 s resolution) */
            for (int remaining = 5; remaining > 0; remaining--) {
                LOG_WRN("WDT feed paused — reset in ~%d s", remaining);
                k_msleep(1000);
            }
            /* The WDT will have fired by now; if we reach here the
             * callback fired but the reset hasn't happened yet —
             * just spin. */
            while (1) {
                k_msleep(100);
            }
        }

        feed_count++;
        wdt_feed(wdt_dev, wdt_channel_id);
        LOG_INF("WDT fed (count: %u, uptime: %u ms)",
                feed_count, (uint32_t)k_uptime_get_32());

#if defined(CONFIG_WDT_DEMO_TRIGGER_RESET)
        /* After 5 feeds (10 s) deliberately stop feeding */
        if (feed_count >= 5U) {
            LOG_WRN("Demo mode: stopping WDT feed after %u feeds.", feed_count);
            LOG_WRN("System will reset in ~5 seconds...");
            feed_paused = true;
        }
#endif /* CONFIG_WDT_DEMO_TRIGGER_RESET */
    }
}

/* --------------------------------------------------------------------------
 * Main
 * -------------------------------------------------------------------------- */
int main(void)
{
    int ret;

    LOG_INF("=== Conexio Stratus Pro — Watchdog Sample ===");
    LOG_INF("System restarted. Uptime: %u ms", (uint32_t)k_uptime_get_32());
    LOG_INF("Board: %s", CONFIG_BOARD);

    /* ------------------------------------------------------------------ */
    /* 1. Get and validate the WDT device                                 */
    /* ------------------------------------------------------------------ */
    wdt_dev = DEVICE_DT_GET(WDT_NODE);
    if (!device_is_ready(wdt_dev)) {
        LOG_ERR("WDT device %s not ready", wdt_dev->name);
        return -ENODEV;
    }
    LOG_INF("WDT device: %s", wdt_dev->name);

    /* ------------------------------------------------------------------ */
    /* 2. Install the WDT timeout                                         */
    /*                                                                    */
    /*    window.min = 0    — no minimum window (feed any time)           */
    /*    window.max = 5000 — 5-second hard timeout                       */
    /*    callback          — last-gasp notification before reset         */
    /*    flags             — WDT_FLAG_RESET_SOC resets the whole chip    */
    /* ------------------------------------------------------------------ */
    struct wdt_timeout_cfg wdt_cfg = {
        .window = {
            .min = 0U,
            .max = 5000U,   /* ms */
        },
        .callback = wdt_callback,
        .flags    = WDT_FLAG_RESET_SOC,
    };

    wdt_channel_id = wdt_install_timeout(wdt_dev, &wdt_cfg);
    if (wdt_channel_id < 0) {
        LOG_ERR("Failed to install WDT timeout: %d", wdt_channel_id);
        return wdt_channel_id;
    }
    LOG_INF("WDT timeout installed: channel %d, max=%u ms",
            wdt_channel_id, wdt_cfg.window.max);

    /* ------------------------------------------------------------------ */
    /* 3. Start the watchdog                                              */
    /*                                                                    */
    /*    WDT_OPT_PAUSE_IN_SLEEP     — pause WDT counter during CPU sleep */
    /*    WDT_OPT_PAUSE_HALTED_BY_DBG — pause when debugger halts the CPU */
    /* ------------------------------------------------------------------ */
    ret = wdt_setup(wdt_dev,
                    WDT_OPT_PAUSE_IN_SLEEP | WDT_OPT_PAUSE_HALTED_BY_DBG);
    if (ret < 0) {
        /* Some targets don't support all options — try with no options */
        LOG_WRN("wdt_setup with pause options failed (%d), retrying with "
                "WDT_OPT_PAUSE_HALTED_BY_DBG only", ret);
        ret = wdt_setup(wdt_dev, WDT_OPT_PAUSE_HALTED_BY_DBG);
        if (ret < 0) {
            LOG_WRN("wdt_setup with debug-pause failed (%d), "
                    "starting with no options", ret);
            ret = wdt_setup(wdt_dev, 0);
        }
    }
    if (ret < 0) {
        LOG_ERR("WDT setup failed: %d", ret);
        return ret;
    }
    LOG_INF("WDT started (5 s timeout, pause-in-sleep, pause-halted-by-dbg).");

    /* ------------------------------------------------------------------ */
    /* 4. Configure button GPIO interrupt                                 */
    /* ------------------------------------------------------------------ */
    if (!gpio_is_ready_dt(&button)) {
        LOG_ERR("Button GPIO not ready");
        return -ENODEV;
    }

    ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
    if (ret < 0) {
        LOG_ERR("Failed to configure button pin: %d", ret);
        return ret;
    }

    ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);
    if (ret < 0) {
        LOG_ERR("Failed to configure button interrupt: %d", ret);
        return ret;
    }

    gpio_init_callback(&btn_cb_data, button_isr, BIT(button.pin));
    gpio_add_callback(button.port, &btn_cb_data);

    LOG_INF("Button configured (gpio0 pin %u). Press to pause WDT feed.",
            button.pin);

    /* ------------------------------------------------------------------ */
    /* 5. Start the feed thread                                           */
    /* ------------------------------------------------------------------ */
    k_thread_create(&feed_thread_data,
                    feed_thread_stack,
                    K_THREAD_STACK_SIZEOF(feed_thread_stack),
                    watchdog_feed_thread,
                    NULL, NULL, NULL,
                    FEED_THREAD_PRIORITY, 0, K_NO_WAIT);
    k_thread_name_set(&feed_thread_data, "watchdog_feed_thread");

#if defined(CONFIG_WDT_DEMO_TRIGGER_RESET)
    LOG_INF("Demo mode enabled: WDT feed will stop after 5 feeds (~10 s).");
    LOG_INF("Watch for the system reset and restart message.");
#else
    LOG_INF("Normal mode: WDT fed indefinitely every %d ms.", FEED_INTERVAL_MS);
    LOG_INF("Press the MODE button (gpio0 pin 31) to pause feeding "
            "and trigger a reset.");
#endif

    /* Main thread — nothing left to do; the feed thread handles the WDT */
    while (1) {
        k_msleep(10000);
    }

    return 0;
}
