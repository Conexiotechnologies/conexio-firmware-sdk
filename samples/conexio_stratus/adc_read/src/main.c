/*
 * Copyright (c) 2025 Conexio Technologies, Inc.
 * Author: Rajeev Piyare <rajeev@conexiotech.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * ADC Read Sample — Conexio Stratus Pro (nRF9151)
 *
 * Demonstrates the DT-based ADC API on the nRF9151 SAADC peripheral.
 * Reads a 0–3.3 V analog signal from header pin D13/A0 (P0.13, AIN0).
 *
 * Hardware connection:
 *   Connect a 0–3.3 V signal (potentiometer wiper, sensor output, etc.)
 *   to the stratus_header D13/A0 pin (P0.13) and GND to any GND pin.
 *   Do NOT exceed 3.3 V on this pin.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/dt-bindings/adc/nrf-saadc.h>
#include <hal/nrf_saadc.h>

LOG_MODULE_REGISTER(adc_read, LOG_LEVEL_DBG);

/* --------------------------------------------------------------------------
 * Devicetree: get the ADC channel spec from the zephyr,user node.
 * The overlay defines:
 *   / { zephyr,user { io-channels = <&adc 0>; io-channel-names = "A0"; }; };
 * -------------------------------------------------------------------------- */
#define ZEPHYR_USER_NODE DT_PATH(zephyr_user)

#if !DT_NODE_EXISTS(ZEPHYR_USER_NODE) || \
    !DT_NODE_HAS_PROP(ZEPHYR_USER_NODE, io_channels)
#error "No zephyr,user node with io-channels found — check the board overlay."
#endif

static const struct adc_dt_spec adc_chan =
    ADC_DT_SPEC_GET_BY_NAME(ZEPHYR_USER_NODE, a0);

/* Number of samples to average for the oversampling demonstration */
#define OVERSAMPLE_COUNT 10

/* Sampling period */
#define SAMPLE_INTERVAL_MS 1000

int main(void)
{
    int ret;

    LOG_INF("=== Conexio Stratus Pro — ADC Read Sample ===");
    LOG_INF("Board: %s", CONFIG_BOARD);
    LOG_INF("Connect a 0-3.3 V signal to header D13/A0 (P0.13) and GND.");

    /* ------------------------------------------------------------------ */
    /* 1. Verify the ADC controller is ready                               */
    /* ------------------------------------------------------------------ */
    if (!adc_is_ready_dt(&adc_chan)) {
        LOG_ERR("ADC controller %s not ready", adc_chan.dev->name);
        return -ENODEV;
    }

    LOG_INF("ADC controller: %s", adc_chan.dev->name);
    LOG_INF("Channel:        %u", adc_chan.channel_id);
    LOG_INF("Resolution:     %u bits", adc_chan.resolution);
    LOG_INF("Gain:           1/6 (full-scale ~3.6 V for 3.3 V range)");
    LOG_INF("Reference:      INTERNAL (0.6 V)");
    LOG_INF("Oversampling:   SW average of %u samples", OVERSAMPLE_COUNT);

    /* ------------------------------------------------------------------ */
    /* 2. Configure the channel                                            */
    /* ------------------------------------------------------------------ */
    ret = adc_channel_setup_dt(&adc_chan);
    if (ret < 0) {
        LOG_ERR("Failed to configure ADC channel: %d", ret);
        return ret;
    }

    LOG_INF("ADC channel configured successfully.");

    /* ------------------------------------------------------------------ */
    /* 3. Prepare the sequence buffer — 16-bit raw value                  */
    /* ------------------------------------------------------------------ */
    int16_t raw_buf;
    struct adc_sequence sequence = {0};

    ret = adc_sequence_init_dt(&adc_chan, &sequence);
    if (ret < 0) {
        LOG_ERR("Failed to init ADC sequence: %d", ret);
        return ret;
    }

    sequence.buffer      = &raw_buf;
    sequence.buffer_size = sizeof(raw_buf);
    /* Disable HW oversampling — the driver would require a buffer large
     * enough to hold all (1 << oversampling) raw samples before averaging.
     * SW averaging over OVERSAMPLE_COUNT reads is done in the loop below. */
    sequence.oversampling = 0;

    /* ------------------------------------------------------------------ */
    /* 4. Main sampling loop                                               */
    /* ------------------------------------------------------------------ */
    uint32_t sample_count = 0;

    while (1) {
        sample_count++;

        /* --- Single sample ------------------------------------------- */
        ret = adc_read_dt(&adc_chan, &sequence);
        if (ret < 0) {
            LOG_ERR("ADC read failed: %d", ret);
            k_msleep(SAMPLE_INTERVAL_MS);
            continue;
        }

        int32_t mv_value = raw_buf;

        ret = adc_raw_to_millivolts_dt(&adc_chan, &mv_value);
        if (ret < 0) {
            LOG_WRN("Cannot convert raw to mV (reference not set?): %d", ret);
        }

        LOG_INF("--- Sample #%u ---", sample_count);
        LOG_INF("  Raw value : %d (12-bit)", raw_buf);
        LOG_INF("  Voltage   : %d mV", mv_value);

        /* --- Oversampling demonstration (software average of 10) ------ */
        int32_t sum_mv  = 0;
        int32_t sum_raw = 0;
        bool    avg_ok  = true;

        for (int i = 0; i < OVERSAMPLE_COUNT; i++) {
            ret = adc_read_dt(&adc_chan, &sequence);
            if (ret < 0) {
                LOG_WRN("ADC read %d failed: %d", i, ret);
                avg_ok = false;
                break;
            }

            int32_t sample_mv = raw_buf;

            ret = adc_raw_to_millivolts_dt(&adc_chan, &sample_mv);
            if (ret == 0) {
                sum_mv  += sample_mv;
                sum_raw += raw_buf;
            }
        }

        if (avg_ok) {
            int32_t avg_mv  = sum_mv  / OVERSAMPLE_COUNT;
            int32_t avg_raw = sum_raw / OVERSAMPLE_COUNT;

            LOG_INF("  SW avg (%u samples):", OVERSAMPLE_COUNT);
            LOG_INF("    Avg raw : %d", avg_raw);
            LOG_INF("    Avg mV  : %d mV", avg_mv);
            LOG_INF("  Note: SW averaging over %u samples reduces noise",
                    OVERSAMPLE_COUNT);
        }

        LOG_INF("  Ref voltage : 600 mV (internal)");
        LOG_INF("  Resolution  : %u bits", adc_chan.resolution);

        k_msleep(SAMPLE_INTERVAL_MS);
    }

    return 0;
}
