/*
 * Copyright (c) 2026 Conexio Technologies, Inc.
 * Author: Rajeev Piyare <rajeev@conexiotech.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * SPI Direct Read/Write sample for Conexio Stratus Pro (nRF9151)
 *
 * This sample demonstrates the core Zephyr SPI API functions using the
 * SPI3 bus exposed on the Stratus header connector.
 *
 * Bus:  SPI3 (nRF SPIM3)
 *   SCK  = P0.13  (D19)
 *   MOSI = P0.14  (D20)
 *   MISO = P0.15  (D21)
 *   CS   = P0.17  (D22) — hardware-controlled, active-low
 *
 * ─────────────────────────────────────────────────────────────────────
 * API summary
 * ─────────────────────────────────────────────────────────────────────
 *
 *  All SPI transfers are built around two structures:
 *
 *    struct spi_buf      { void *buf; size_t len; }
 *    struct spi_buf_set  { const struct spi_buf *buffers; size_t count; }
 *
 *  The TX and RX buffer sets are passed independently, allowing scatter-
 *  gather transfers. Either set can be NULL to indicate a direction-only
 *  transfer (TX-only or RX-only).
 *
 *  Key functions (all return 0 on success, negative errno on error):
 *
 *    spi_write_dt(spec, tx_bufs)
 *      Clocks out the TX buffer(s). MISO is ignored.
 *
 *    spi_read_dt(spec, rx_bufs)
 *      Clocks in the RX buffer(s). MOSI sends dummy bytes (0x00).
 *
 *    spi_transceive_dt(spec, tx_bufs, rx_bufs)
 *      Full-duplex: simultaneously shifts out TX and captures RX.
 *      This is the fundamental SPI operation — the others are wrappers.
 *
 * ─────────────────────────────────────────────────────────────────────
 * Demo structure
 * ─────────────────────────────────────────────────────────────────────
 *
 *  Demo 1 — spi_write_dt:        TX-only transfer (e.g. send a command)
 *  Demo 2 — spi_read_dt:         RX-only transfer (e.g. read a status)
 *  Demo 3 — spi_transceive_dt:   Full-duplex single buffer
 *  Demo 4 — spi_transceive_dt:   Split TX/RX buffers (cmd + response)
 *  Demo 5 — spi_transceive_dt:   Scatter-gather multi-buffer transaction
 *  Demo 6 — loopback self-test:  Connect MOSI→MISO on the header to run
 *                                  a verify-by-value round-trip test
 *
 * ─────────────────────────────────────────────────────────────────────
 * Testing without external hardware
 * ─────────────────────────────────────────────────────────────────────
 *
 *  Demos 1–5 show the correct API usage but cannot verify received data
 *  without a real SPI device connected.
 *
 *  Demo 6 (loopback) requires a single wire bridging D20 (MOSI) to
 *  D21 (MISO) on the Stratus header. This routes the transmitted bytes
 *  back into the receive buffer, making round-trip verification possible
 *  with no external device.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(spi_rw, LOG_LEVEL_INF);

/* -----------------------------------------------------------------------
 * SPI device from devicetree
 * The spi_dt_spec bundles the bus device pointer and the per-device
 * spi_config (frequency, mode, CS GPIO) in one struct.
 *
 * SPI_DT_SPEC_GET arguments:
 *   node_id   — DT node of the SPI *device* (child of the SPI controller)
 *   operation — base operation flags (mode bits added by DT CPOL/CPHA props)
 *   delay     — CS assert-to-clock delay in µs (0 = no delay)
 * ----------------------------------------------------------------------- */
#define SPI_DEV_NODE DT_NODELABEL(spi_dev0)

static const struct spi_dt_spec spidev = SPI_DT_SPEC_GET(
	SPI_DEV_NODE,
	SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB,
	0
);

/* -----------------------------------------------------------------------
 * Helper: print buffer as hex
 * ----------------------------------------------------------------------- */
static void hex_dump(const char *label, const uint8_t *buf, size_t len)
{
	printk("%s (%zu bytes):", label, len);
	for (size_t i = 0; i < len; i++) {
		printk(" %02X", buf[i]);
	}
	printk("\n");
}

/* -----------------------------------------------------------------------
 * Demo 1: spi_write_dt — TX-only transfer
 *
 * Use case: sending a command or address byte to a device where the
 * MISO response during this phase is irrelevant (e.g. write-enable
 * command to a flash chip, display command byte).
 *
 * The CS line is asserted before the call and released after.
 * ----------------------------------------------------------------------- */
static int demo_write_only(void)
{
	/* Example: send a 3-byte command sequence */
	uint8_t tx_data[] = { 0x06, 0x00, 0xFF };   /* write-enable + addr */

	const struct spi_buf tx_buf = {
		.buf = tx_data,
		.len = sizeof(tx_data),
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf,
		.count   = 1,
	};

	int ret = spi_write_dt(&spidev, &tx);

	if (ret) {
		LOG_ERR("spi_write_dt failed (err %d)", ret);
		return ret;
	}

	hex_dump("TX (write-only)", tx_data, sizeof(tx_data));
	LOG_INF("spi_write_dt OK");
	return 0;
}

/* -----------------------------------------------------------------------
 * Demo 2: spi_read_dt — RX-only transfer
 *
 * Use case: reading N bytes from a device after it has been primed
 * (e.g. a second transaction after sending a read command). MOSI
 * shifts out 0x00 dummy bytes while MISO is captured.
 * ----------------------------------------------------------------------- */
static int demo_read_only(void)
{
	uint8_t rx_data[4] = {0};

	const struct spi_buf rx_buf = {
		.buf = rx_data,
		.len = sizeof(rx_data),
	};
	const struct spi_buf_set rx = {
		.buffers = &rx_buf,
		.count   = 1,
	};

	int ret = spi_read_dt(&spidev, &rx);

	if (ret) {
		LOG_ERR("spi_read_dt failed (err %d)", ret);
		return ret;
	}

	hex_dump("RX (read-only)", rx_data, sizeof(rx_data));
	LOG_INF("spi_read_dt OK (data valid only with real device connected)");
	return 0;
}

/* -----------------------------------------------------------------------
 * Demo 3: spi_transceive_dt — full-duplex, single shared buffer
 *
 * Use case: protocols where every byte sent has a corresponding byte
 * received simultaneously (e.g. reading a sensor register on SPI).
 *
 * Here TX and RX share the same buffer length. After the call the
 * rx buffer holds whatever MISO produced during the transaction.
 * ----------------------------------------------------------------------- */
static int demo_transceive_single(void)
{
	/* First byte is typically a command/register address.
	 * Subsequent bytes are dummy (0x00) for the device to fill in.
	 */
	uint8_t tx_data[] = { 0x8F, 0x00, 0x00 };   /* read WHO_AM_I (example) */
	uint8_t rx_data[sizeof(tx_data)] = {0};

	const struct spi_buf tx_buf = { .buf = tx_data, .len = sizeof(tx_data) };
	const struct spi_buf rx_buf = { .buf = rx_data, .len = sizeof(rx_data) };

	const struct spi_buf_set tx = { .buffers = &tx_buf, .count = 1 };
	const struct spi_buf_set rx = { .buffers = &rx_buf, .count = 1 };

	int ret = spi_transceive_dt(&spidev, &tx, &rx);

	if (ret) {
		LOG_ERR("spi_transceive_dt failed (err %d)", ret);
		return ret;
	}

	hex_dump("TX", tx_data, sizeof(tx_data));
	hex_dump("RX", rx_data, sizeof(rx_data));
	LOG_INF("spi_transceive_dt (single buffer) OK");
	return 0;
}

/* -----------------------------------------------------------------------
 * Demo 4: spi_transceive_dt — split command + response buffers
 *
 * Use case: send a command header, then receive a response payload,
 * all in one CS assertion. This is the most common real-world SPI
 * read pattern: [CMD | ADDR] → [DATA0 DATA1 ... DATAn]
 *
 * By using separate TX and RX buffers of different lengths, the
 * first spi_buf in each set can be used for the command phase and the
 * second for the data phase.
 *
 * Important: TX and RX buffer sets are traversed independently. The
 * total bytes clocked on the bus equals max(total_tx, total_rx).
 * Shorter side gets padded with 0x00 (TX) or discarded (RX).
 * ----------------------------------------------------------------------- */
static int demo_transceive_split(void)
{
	/* TX: 1-byte command + 1-byte address, nothing in byte 3+ */
	uint8_t cmd[]  = { 0x03, 0x00 };    /* read command + address */
	uint8_t dummy  = 0x00;

	/* RX: ignore first 2 bytes (during command phase), capture 4 bytes */
	uint8_t rx_ignore[2] = {0};
	uint8_t rx_data[4]   = {0};

	const struct spi_buf tx_bufs[] = {
		{ .buf = cmd,   .len = sizeof(cmd)  },
		{ .buf = &dummy,.len = sizeof(rx_data) }, /* clock out dummy while reading */
	};
	const struct spi_buf rx_bufs[] = {
		{ .buf = rx_ignore, .len = sizeof(rx_ignore) }, /* discard cmd echo */
		{ .buf = rx_data,   .len = sizeof(rx_data)   }, /* capture payload  */
	};

	const struct spi_buf_set tx = { .buffers = tx_bufs, .count = ARRAY_SIZE(tx_bufs) };
	const struct spi_buf_set rx = { .buffers = rx_bufs, .count = ARRAY_SIZE(rx_bufs) };

	int ret = spi_transceive_dt(&spidev, &tx, &rx);

	if (ret) {
		LOG_ERR("spi_transceive_dt split failed (err %d)", ret);
		return ret;
	}

	hex_dump("TX cmd", cmd, sizeof(cmd));
	hex_dump("RX payload", rx_data, sizeof(rx_data));
	LOG_INF("spi_transceive_dt (split cmd/response) OK");
	return 0;
}

/* -----------------------------------------------------------------------
 * Demo 5: spi_transceive_dt — scatter-gather multi-buffer
 *
 * Use case: building a packet from non-contiguous memory regions
 * without copying them into a single flat buffer first.
 * Each spi_buf entry in the set is transmitted/received sequentially
 * within one CS assertion.
 * ----------------------------------------------------------------------- */
static int demo_scatter_gather(void)
{
	uint8_t header[]  = { 0xAA, 0x55 };
	uint8_t payload[] = { 0x01, 0x02, 0x03, 0x04 };
	uint8_t footer[]  = { 0xFF };

	uint8_t rx_buf[sizeof(header) + sizeof(payload) + sizeof(footer)] = {0};

	/* TX: three non-contiguous buffers sent in sequence */
	const struct spi_buf tx_bufs[] = {
		{ .buf = header,  .len = sizeof(header)  },
		{ .buf = payload, .len = sizeof(payload) },
		{ .buf = footer,  .len = sizeof(footer)  },
	};

	/* RX: one flat buffer capturing the full transaction */
	const struct spi_buf rx_buf_set = { .buf = rx_buf, .len = sizeof(rx_buf) };

	const struct spi_buf_set tx = { .buffers = tx_bufs,    .count = ARRAY_SIZE(tx_bufs) };
	const struct spi_buf_set rx = { .buffers = &rx_buf_set,.count = 1 };

	int ret = spi_transceive_dt(&spidev, &tx, &rx);

	if (ret) {
		LOG_ERR("Scatter-gather failed (err %d)", ret);
		return ret;
	}

	LOG_INF("TX: header[%zu] + payload[%zu] + footer[%zu] — scatter-gather",
		sizeof(header), sizeof(payload), sizeof(footer));
	hex_dump("RX", rx_buf, sizeof(rx_buf));
	LOG_INF("spi_transceive_dt (scatter-gather) OK");
	return 0;
}

/* -----------------------------------------------------------------------
 * Demo 6: loopback self-test
 *
 * Requires: bridge D20 (MOSI, P0.14) → D21 (MISO, P0.15) on the
 * Stratus header with a jumper wire.
 *
 * Sends a known pattern and verifies that what is received on MISO
 * exactly matches what was sent on MOSI. This proves the SPI bus,
 * pinctrl, and CS GPIO are all working correctly end-to-end without
 * needing a real device.
 * ----------------------------------------------------------------------- */
static int demo_loopback(void)
{
	const uint8_t tx_pattern[] = {
		0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF
	};
	uint8_t rx_pattern[sizeof(tx_pattern)] = {0};

	const struct spi_buf tx_buf = { .buf = (void *)tx_pattern, .len = sizeof(tx_pattern) };
	const struct spi_buf rx_buf = { .buf = rx_pattern,         .len = sizeof(rx_pattern) };

	const struct spi_buf_set tx = { .buffers = &tx_buf, .count = 1 };
	const struct spi_buf_set rx = { .buffers = &rx_buf, .count = 1 };

	int ret = spi_transceive_dt(&spidev, &tx, &rx);

	if (ret) {
		LOG_ERR("Loopback transceive failed (err %d)", ret);
		return ret;
	}

	hex_dump("TX (sent)", tx_pattern, sizeof(tx_pattern));
	hex_dump("RX (received)", rx_pattern, sizeof(rx_pattern));

	if (memcmp(tx_pattern, rx_pattern, sizeof(tx_pattern)) == 0) {
		LOG_INF("Loopback PASSED — TX matches RX");
		return 0;
	}

	LOG_ERR("Loopback FAILED — mismatch (is MOSI bridged to MISO on the header?)");
	for (size_t i = 0; i < sizeof(tx_pattern); i++) {
		if (tx_pattern[i] != rx_pattern[i]) {
			LOG_ERR("  byte[%zu]: sent 0x%02X, got 0x%02X",
				i, tx_pattern[i], rx_pattern[i]);
		}
	}
	return -EIO;
}

/* -----------------------------------------------------------------------
 * main
 * ----------------------------------------------------------------------- */
int main(void)
{
	printk("SPI read/write sample running on %s\n", CONFIG_BOARD);

	if (!spi_is_ready_dt(&spidev)) {
		LOG_ERR("SPI bus not ready");
		return -ENODEV;
	}

	LOG_INF("SPI bus ready");
	LOG_INF("  Bus:  SPI3 (nRF SPIM3)");
	LOG_INF("  SCK   P0.13 (D19)");
	LOG_INF("  MOSI  P0.14 (D20)");
	LOG_INF("  MISO  P0.15 (D21)");
	LOG_INF("  CS    P0.17 (D22) — active-low, hardware-controlled");
	LOG_INF("  Speed %u Hz, Mode 0 (CPOL=0, CPHA=0)", spidev.config.frequency);

	int err;

	/* --- Demo 1: TX-only --- */
	LOG_INF("--- Demo 1: spi_write_dt (TX-only) ---");
	err = demo_write_only();
	if (err) {
		LOG_ERR("Demo 1 failed");
	}

	/* --- Demo 2: RX-only --- */
	LOG_INF("--- Demo 2: spi_read_dt (RX-only) ---");
	err = demo_read_only();
	if (err) {
		LOG_ERR("Demo 2 failed");
	}

	/* --- Demo 3: Full-duplex single buffer --- */
	LOG_INF("--- Demo 3: spi_transceive_dt (full-duplex, single buffer) ---");
	err = demo_transceive_single();
	if (err) {
		LOG_ERR("Demo 3 failed");
	}

	/* --- Demo 4: Split command/response buffers --- */
	LOG_INF("--- Demo 4: spi_transceive_dt (split cmd + response) ---");
	err = demo_transceive_split();
	if (err) {
		LOG_ERR("Demo 4 failed");
	}

	/* --- Demo 5: Scatter-gather --- */
	LOG_INF("--- Demo 5: spi_transceive_dt (scatter-gather) ---");
	err = demo_scatter_gather();
	if (err) {
		LOG_ERR("Demo 5 failed");
	}

	/* --- Demo 6: Loopback self-test --- */
	LOG_INF("--- Demo 6: Loopback self-test (bridge D20→D21 required) ---");
	err = demo_loopback();
	if (err) {
		LOG_WRN("Demo 6: loopback wire not present or bus issue — see above");
	}

	LOG_INF("SPI sample complete");
	return 0;
}
