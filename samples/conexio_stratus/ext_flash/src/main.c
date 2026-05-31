/*
 * Copyright (c) 2026 Conexio Technologies, Inc
 * Author: Rajeev Piyare <rajeev@conexiotech.com>
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 *
 * External EEPROM (Microchip 24CW160) read/write sample
 * for Conexio Stratus Pro (nRF9151).
 *
 * Hardware:
 *   - 24CW160: 2 KiB (16 kbit) I2C EEPROM, 32-byte pages
 *   - Connected on I2C2 (SDA=P0.08, SCL=P0.07) at address 0x50
 *   - Powered via the onboard sensor power rail (GPIO 26)
 *
 * The sample demonstrates:
 *   1. Querying EEPROM size with eeprom_get_size().
 *   2. Writing a test pattern with eeprom_write().
 *   3. Reading it back with eeprom_read().
 *   4. Verifying byte-by-byte that read data matches written data.
 *   5. Erasing (zeroing) the test region to leave the EEPROM clean.
 *
 * The test offset and pattern size are configurable via Kconfig:
 *   CONFIG_EEPROM_TEST_OFFSET        (default: 0)
 *   CONFIG_EEPROM_TEST_PATTERN_SIZE  (default: 32)
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/eeprom.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(ext_flash, LOG_LEVEL_INF);

/* Get the EEPROM device from the devicetree node label defined in the board DTSI */
#define EEPROM_NODE DT_NODELABEL(eeprom0)

/* Test parameters from Kconfig */
#define TEST_OFFSET  CONFIG_EEPROM_TEST_OFFSET
#define TEST_SIZE    CONFIG_EEPROM_TEST_PATTERN_SIZE

static const struct device *eeprom = DEVICE_DT_GET(EEPROM_NODE);

/**
 * @brief Fill a buffer with a simple rolling test pattern.
 *
 * Pattern: buf[i] = (uint8_t)(start_val + i)
 * This makes it easy to spot corruption or offset errors visually.
 */
static void fill_test_pattern(uint8_t *buf, size_t len, uint8_t start_val)
{
	for (size_t i = 0; i < len; i++) {
		buf[i] = (uint8_t)(start_val + i);
	}
}

/**
 * @brief Print a hex dump of a buffer for inspection.
 */
static void hex_dump(const char *label, const uint8_t *buf, size_t len)
{
	printk("%s (%zu bytes):\n", label, len);
	for (size_t i = 0; i < len; i++) {
		printk("%02X ", buf[i]);
		if ((i + 1) % 16 == 0) {
			printk("\n");
		}
	}
	if (len % 16 != 0) {
		printk("\n");
	}
}

int main(void)
{
	printk("EEPROM sample running on %s\n", CONFIG_BOARD);

	if (!device_is_ready(eeprom)) {
		LOG_ERR("EEPROM device not ready");
		return -ENODEV;
	}

	/* Query and report EEPROM size */
	size_t eeprom_size = eeprom_get_size(eeprom);

	LOG_INF("EEPROM device: %s", eeprom->name);
	LOG_INF("EEPROM size: %zu bytes", eeprom_size);

	/* Validate test parameters fit within the EEPROM */
	if ((TEST_OFFSET + TEST_SIZE) > eeprom_size) {
		LOG_ERR("Test region [%d..%d] exceeds EEPROM size (%zu bytes)",
			TEST_OFFSET, TEST_OFFSET + TEST_SIZE, eeprom_size);
		return -EINVAL;
	}

	uint8_t write_buf[TEST_SIZE];
	uint8_t read_buf[TEST_SIZE];

	/* --- Step 1: Write test pattern --- */
	fill_test_pattern(write_buf, TEST_SIZE, 0xA0);
	hex_dump("Write buffer", write_buf, TEST_SIZE);

	LOG_INF("Writing %d bytes to EEPROM offset 0x%04X...", TEST_SIZE, TEST_OFFSET);

	int ret = eeprom_write(eeprom, TEST_OFFSET, write_buf, TEST_SIZE);

	if (ret < 0) {
		LOG_ERR("Write FAILED (err %d)", ret);
		return ret;
	}
	LOG_INF("Write OK");

	/* Small delay to ensure the EEPROM write cycle completes.
	 * The 24CW160 datasheet specifies a max write cycle time of 5 ms.
	 */
	k_msleep(10);

	/* --- Step 2: Read back --- */
	memset(read_buf, 0, sizeof(read_buf));

	LOG_INF("Reading %d bytes from EEPROM offset 0x%04X...", TEST_SIZE, TEST_OFFSET);

	ret = eeprom_read(eeprom, TEST_OFFSET, read_buf, TEST_SIZE);
	if (ret < 0) {
		LOG_ERR("Read FAILED (err %d)", ret);
		return ret;
	}
	LOG_INF("Read OK");
	hex_dump("Read buffer", read_buf, TEST_SIZE);

	/* --- Step 3: Verify --- */
	if (memcmp(write_buf, read_buf, TEST_SIZE) == 0) {
		LOG_INF("Verify PASSED — read data matches written data");
	} else {
		LOG_ERR("Verify FAILED — mismatch detected");
		for (size_t i = 0; i < TEST_SIZE; i++) {
			if (write_buf[i] != read_buf[i]) {
				LOG_ERR("  byte[%zu]: wrote 0x%02X, read 0x%02X",
					i, write_buf[i], read_buf[i]);
			}
		}
		return -EIO;
	}

	/* --- Step 4: Erase (zero) the test region --- */
	LOG_INF("Erasing test region (writing zeros)...");

	uint8_t zero_buf[TEST_SIZE];

	memset(zero_buf, 0x00, TEST_SIZE);

	ret = eeprom_write(eeprom, TEST_OFFSET, zero_buf, TEST_SIZE);
	if (ret < 0) {
		LOG_ERR("Erase write FAILED (err %d)", ret);
		return ret;
	}
	k_msleep(10);

	/* Verify erase */
	memset(read_buf, 0xFF, sizeof(read_buf));
	ret = eeprom_read(eeprom, TEST_OFFSET, read_buf, TEST_SIZE);
	if (ret < 0) {
		LOG_ERR("Erase verify read FAILED (err %d)", ret);
		return ret;
	}

	if (memcmp(zero_buf, read_buf, TEST_SIZE) == 0) {
		LOG_INF("Erase verify PASSED — region is zeroed");
	} else {
		LOG_ERR("Erase verify FAILED");
		return -EIO;
	}

	LOG_INF("EEPROM sample complete");

	return 0;
}
