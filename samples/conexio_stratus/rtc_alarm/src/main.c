/*
 * Copyright (c) 2026 Conexio Technologies, Inc.
 * Author: Rajeev Piyare <rajeev@conexiotech.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * RTC Alarm Sample — Conexio Stratus Pro (nRF9151)
 *
 * Demonstrates Zephyr's counter API as an RTC abstraction:
 *
 *   Demo 1 — One-shot alarm (5 seconds)
 *   Demo 2 — Periodic alarm (3-second interval, 5 repetitions)
 *   Demo 3 — Wake from system-off (opt-in via CONFIG_RTC_DEMO_SYSTEM_OFF)
 *
 * The counter API is the correct Zephyr abstraction for the nRF RTC
 * peripherals. RTC0 is used because RTC1 is reserved for the Zephyr
 * kernel system clock (CONFIG_NRF_RTC_TIMER uses NRF_RTC1).
 * Note: the nRF91 series only has RTC0 and RTC1 (no RTC2 like nRF52/53).
 *
 * Hardware: No external hardware required.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/sys/poweroff.h>

LOG_MODULE_REGISTER(rtc_alarm, LOG_LEVEL_DBG);

/* --------------------------------------------------------------------------
 * RTC device — RTC0, enabled in the board overlay.
 *
 * RTC assignment on nRF9151:
 *   RTC0 — available for application use  ← this sample uses RTC0
 *   RTC1 — used by the Zephyr kernel system clock (NRF_RTC_TIMER)
 *
 * Note: the nRF91 series only has RTC0 and RTC1 (no RTC2 like nRF52/53).
 * -------------------------------------------------------------------------- */
#define RTC_NODE DT_NODELABEL(rtc0)

/* --------------------------------------------------------------------------
 * Semaphore signalled from the alarm callback ISR to unblock main thread
 * -------------------------------------------------------------------------- */
static K_SEM_DEFINE(alarm_sem, 0, 1);

/* --------------------------------------------------------------------------
 * Periodic demo state
 * -------------------------------------------------------------------------- */
#define PERIODIC_ALARM_INTERVAL_S 3U
#define PERIODIC_ALARM_COUNT      5U

static volatile uint32_t periodic_count;
static const struct device *rtc_dev;   /* filled in main() */

/* --------------------------------------------------------------------------
 * Alarm callback
 *
 * Runs in ISR context — keep it brief.
 * Signal the semaphore so the main thread can log and re-arm.
 * -------------------------------------------------------------------------- */
static void alarm_callback(const struct device *dev,
                            uint8_t chan_id,
                            uint32_t ticks,
                            void *user_data)
{
    ARG_UNUSED(dev);
    ARG_UNUSED(chan_id);
    ARG_UNUSED(ticks);
    ARG_UNUSED(user_data);

    k_sem_give(&alarm_sem);
}

/* --------------------------------------------------------------------------
 * Helper: set an alarm N seconds from now
 * Returns 0 on success, negative errno on failure.
 * -------------------------------------------------------------------------- */
static int set_alarm_seconds(uint32_t seconds)
{
    uint32_t ticks = counter_us_to_ticks(rtc_dev,
                                          (uint64_t)seconds * 1000000ULL);

    struct counter_alarm_cfg alarm_cfg = {
        .callback  = alarm_callback,
        .ticks     = ticks,
        .user_data = NULL,
        .flags     = 0,
    };

    return counter_set_channel_alarm(rtc_dev, 0, &alarm_cfg);
}

/* --------------------------------------------------------------------------
 * Main
 * -------------------------------------------------------------------------- */
int main(void)
{
    int ret;

    LOG_INF("=== Conexio Stratus Pro — RTC Alarm Sample ===");
    LOG_INF("Board: %s", CONFIG_BOARD);

    /* ------------------------------------------------------------------ */
    /* 1. Get and validate the RTC/counter device                         */
    /* ------------------------------------------------------------------ */
    rtc_dev = DEVICE_DT_GET(RTC_NODE);
    if (!device_is_ready(rtc_dev)) {
        LOG_ERR("RTC device %s not ready", rtc_dev->name);
        return -ENODEV;
    }
    LOG_INF("RTC device: %s", rtc_dev->name);

    /* ------------------------------------------------------------------ */
    /* 2. Print counter characteristics                                   */
    /* ------------------------------------------------------------------ */
    uint32_t freq = counter_get_frequency(rtc_dev);

    LOG_INF("Counter frequency : %u Hz", freq);
    LOG_INF("Counter top value : %u ticks", counter_get_top_value(rtc_dev));

    if (freq > 0U) {
        LOG_INF("Tick period       : ~%u us", 1000000U / freq);
    }

    /* ------------------------------------------------------------------ */
    /* 3. Start the counter                                               */
    /* ------------------------------------------------------------------ */
    ret = counter_start(rtc_dev);
    if (ret < 0) {
        LOG_ERR("Failed to start counter: %d", ret);
        return ret;
    }
    LOG_INF("Counter started.");

    /* ==================================================================
     * DEMO 1 — One-shot alarm, 5 seconds
     * ================================================================== */
    LOG_INF("--- Demo 1: One-shot alarm in 5 seconds ---");

    ret = set_alarm_seconds(5U);
    if (ret < 0) {
        LOG_ERR("Failed to set one-shot alarm: %d", ret);
        counter_stop(rtc_dev);
        return ret;
    }
    LOG_INF("One-shot alarm set. Waiting...");

    /* Block until the alarm callback signals the semaphore */
    k_sem_take(&alarm_sem, K_FOREVER);

    LOG_INF("RTC alarm fired! Uptime: %u ms", (uint32_t)k_uptime_get_32());
    LOG_INF("Demo 1 complete.");

    /* ==================================================================
     * DEMO 2 — Periodic alarm, 3-second interval, 5 repetitions
     * ================================================================== */
    LOG_INF("--- Demo 2: Periodic alarm every %u s, %u times ---",
            PERIODIC_ALARM_INTERVAL_S, PERIODIC_ALARM_COUNT);

    periodic_count = 0U;

    while (periodic_count < PERIODIC_ALARM_COUNT) {
        ret = set_alarm_seconds(PERIODIC_ALARM_INTERVAL_S);
        if (ret < 0) {
            LOG_ERR("Failed to set periodic alarm (count=%u): %d",
                    periodic_count, ret);
            break;
        }

        /* Wait for the alarm to fire */
        k_sem_take(&alarm_sem, K_FOREVER);

        periodic_count++;
        LOG_INF("Periodic alarm #%u fired! Uptime: %u ms",
                periodic_count, (uint32_t)k_uptime_get_32());
    }

    LOG_INF("Demo 2 complete (%u alarms fired).", periodic_count);

    /* Stop the counter — not strictly necessary before poweroff,
     * but good practice to release the peripheral. */
    counter_stop(rtc_dev);
    LOG_INF("Counter stopped.");

    /* ==================================================================
     * DEMO 3 — Wake from system-off (opt-in)
     *
     * Pattern:
     *   1. Restart counter
     *   2. Set alarm N seconds from now
     *   3. Call sys_poweroff()  ← system enters deepest sleep (SYSTEM OFF)
     *   4. RTC keeps running; counter triggers the alarm
     *   5. nRF9151 wakes from SYSTEM OFF → cold boot → main() runs again
     *
     * Important notes:
     *   - The alarm MUST be set BEFORE sys_poweroff() is called.
     *   - There is no "resume" — the system cold-boots on wake.
     *   - All SRAM content is lost in system-off.
     *   - Use retained RAM (GPREGRET / UICR) if you need to pass state
     *     across the system-off boundary.
     *   - MCUboot will run again on wake; ensure the signed image is valid.
     * ================================================================== */
#if defined(CONFIG_RTC_DEMO_SYSTEM_OFF)
    {
#define WAKEUP_DELAY_S 10U

        LOG_INF("--- Demo 3: Wake from system-off in %u seconds ---",
                WAKEUP_DELAY_S);
        LOG_INF("The RTC alarm will be set, then the system enters SYSTEM OFF.");
        LOG_INF("On wake, the nRF9151 cold-boots and main() runs from the start.");
        LOG_INF("All SRAM is lost; use retained RAM for cross-boot state.");

        /* Re-start the counter for the poweroff alarm */
        ret = counter_start(rtc_dev);
        if (ret < 0) {
            LOG_ERR("Failed to restart counter: %d", ret);
            return ret;
        }

        /* Set the wakeup alarm BEFORE entering system-off */
        ret = set_alarm_seconds(WAKEUP_DELAY_S);
        if (ret < 0) {
            LOG_ERR("Failed to set wakeup alarm: %d", ret);
            counter_stop(rtc_dev);
            return ret;
        }

        LOG_INF("Wakeup alarm set for %u s from now.", WAKEUP_DELAY_S);
        LOG_INF("Entering system-off now...");

        /* Small delay to flush the log buffer before power-off */
        k_msleep(100);

        /* Enter system-off — this call does not return */
        sys_poweroff();
    }
#else
    LOG_INF("Demo 3 (wake from system-off) is disabled.");
    LOG_INF("Build with -DCONFIG_RTC_DEMO_SYSTEM_OFF=y to enable it.");
    LOG_INF("All demos complete. Spinning.");

    while (1) {
        k_msleep(10000);
    }
#endif /* CONFIG_RTC_DEMO_SYSTEM_OFF */

    return 0;
}
