/*
 * Copyright (c) 2026 Conexio Technologies, Inc.
 * Author: Rajeev Piyare <rajeev@conexiotech.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * I2C Direct Read/Write sample for Conexio Stratus Pro (nRF9151)
 *
 * This sample demonstrates the four fundamental I2C operations available
 * in the Zephyr I2C API using two real onboard devices on I2C2:
 *
 *   Bus:  I2C2  (SDA = P0.08, SCL = P0.07, 400 kHz fast mode)
 *
 *   Device 1 — ST LIS2DH accelerometer (address 0x18)
 *     Demonstrates: i2c_write_dt(), i2c_read_dt(), i2c_write_read_dt()
 *     Reads the WHO_AM_I register (0x0F) — always returns 0x33.
 *     Reads the STATUS_REG (0x27) to check for new data.
 *     Reads a burst of 6 raw accelerometer bytes (OUT_X_L..OUT_Z_H).
 *
 *   Device 2 — Microchip 24CW160 EEPROM (address 0x50)
 *     Demonstrates: i2c_write_dt(), i2c_read_dt()
 *     Writes a 4-byte pattern to address 0x0010 and reads it back.
 *
 * API summary (all functions return 0 on success, negative errno on error):
 *
 *   i2c_write_dt(spec, buf, len)
 *     Sends `len` bytes from `buf` to the device. First bytes are typically
 *     a register address followed by data.
 *
 *   i2c_read_dt(spec, buf, len)
 *     Reads `len` bytes from the device into `buf`. The device's internal
 *     pointer must already be positioned (use i2c_write_dt first, or use
 *     i2c_write_read_dt for a combined operation).
 *
 *   i2c_write_read_dt(spec, wbuf, wlen, rbuf, rlen)
 *     Sends `wlen` bytes (typically a register address), then immediately
 *     issues a repeated START and reads `rlen` bytes. This is the most
 *     common pattern for reading a specific register on any I2C sensor.
 *
 *   i2c_transfer_dt(spec, msgs, num_msgs)
 *     Low-level function for building custom multi-message transactions
 *     using struct i2c_msg. Use this when the helper functions above
 *     don't cover your use case.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(i2c_rw, LOG_LEVEL_INF);

/* -----------------------------------------------------------------------
 * I2C bus and device addresses
 * ----------------------------------------------------------------------- */

/* I2C2 bus — defined in board DTSI as &i2c2 */
#define I2C_BUS_NODE   DT_NODELABEL(i2c2)

/* ST LIS2DH accelerometer — I2C address 0x18 */
#define LIS2DH_ADDR    0x18U

/* Microchip 24CW160 EEPROM — I2C address 0x50 */
#define EEPROM_ADDR    0x50U

/* -----------------------------------------------------------------------
 * LIS2DH register map (partial)
 * Datasheet: https://www.st.com/resource/en/datasheet/lis2dh.pdf
 * ----------------------------------------------------------------------- */
#define LIS2DH_WHO_AM_I   0x0FU   /* Identity register, always reads 0x33 */
#define LIS2DH_CTRL_REG1  0x20U   /* ODR and axis enable                   */
#define LIS2DH_STATUS_REG 0x27U   /* Data-ready flags                      */
#define LIS2DH_OUT_X_L    0x28U   /* First of 6 raw output bytes           */

/* WHO_AM_I expected value */
#define LIS2DH_WHO_AM_I_VAL 0x33U

/* CTRL_REG1: 10 Hz ODR, all axes enabled, normal power mode */
#define LIS2DH_CTRL_REG1_VAL 0x27U

/* OUT_X_L with MSB set — auto-increment address for burst read */
#define LIS2DH_OUT_X_L_AUTO  (LIS2DH_OUT_X_L | 0x80U)

/* -----------------------------------------------------------------------
 * EEPROM write address (2-byte addressing for 24CW160)
 * We use page 0, offset 0x10 — safely away from address 0x00.
 * ----------------------------------------------------------------------- */
#define EEPROM_TEST_ADDR_HI  0x00U
#define EEPROM_TEST_ADDR_LO  0x10U

static const struct i2c_dt_spec lis2dh = I2C_DT_SPEC_GET(DT_NODELABEL(lis2dh));
static const struct i2c_dt_spec eeprom  = I2C_DT_SPEC_GET(DT_NODELABEL(eeprom0));

/* -----------------------------------------------------------------------
 * Helper: print a buffer as hex
 * ----------------------------------------------------------------------- */
static void hex_dump(const char *label, const uint8_t *buf, size_t len)
{
	printk("%s:", label);
	for (size_t i = 0; i < len; i++) {
		printk(" %02X", buf[i]);
	}
	printk("\n");
}

/* -----------------------------------------------------------------------
 * Demo 1: i2c_write_read_dt — read a single register
 *
 * This is the most common I2C pattern: write the register address,
 * then read back the value in one combined transaction.
 * ----------------------------------------------------------------------- */
static int demo_read_register(void)
{
	uint8_t reg  = LIS2DH_WHO_AM_I;
	uint8_t val  = 0;

	int ret = i2c_write_read_dt(&lis2dh, &reg, sizeof(reg), &val, sizeof(val));

	if (ret < 0) {
		LOG_ERR("i2c_write_read_dt failed (err %d)", ret);
		return ret;
	}

	LOG_INF("LIS2DH WHO_AM_I = 0x%02X (expected 0x%02X) — %s",
		val, LIS2DH_WHO_AM_I_VAL,
		val == LIS2DH_WHO_AM_I_VAL ? "PASS" : "FAIL");

	return (val == LIS2DH_WHO_AM_I_VAL) ? 0 : -EIO;
}

/* -----------------------------------------------------------------------
 * Demo 2: i2c_write_dt — write a configuration register
 *
 * Enable the LIS2DH at 10 Hz with all axes active by writing CTRL_REG1.
 * This shows how to write a register address + value in one call.
 * ----------------------------------------------------------------------- */
static int demo_write_register(void)
{
	/* buf[0] = register address, buf[1] = value to write */
	uint8_t buf[2] = { LIS2DH_CTRL_REG1, LIS2DH_CTRL_REG1_VAL };

	int ret = i2c_write_dt(&lis2dh, buf, sizeof(buf));

	if (ret < 0) {
		LOG_ERR("i2c_write_dt CTRL_REG1 failed (err %d)", ret);
		return ret;
	}

	LOG_INF("LIS2DH CTRL_REG1 written (ODR=10Hz, XYZ enabled)");
	return 0;
}

/* -----------------------------------------------------------------------
 * Demo 3: i2c_write_read_dt — burst read multiple registers
 *
 * Read 6 consecutive output registers (OUT_X_L through OUT_Z_H) in one
 * transaction. The MSB of the register address enables auto-increment on
 * the LIS2DH so all 6 bytes are fetched in a single read.
 * ----------------------------------------------------------------------- */
static int demo_burst_read(void)
{
	uint8_t reg = LIS2DH_OUT_X_L_AUTO;
	uint8_t raw[6] = {0};

	/* Wait for data-ready: poll STATUS_REG bit 3 (ZYXDA) */
	uint8_t status_reg = LIS2DH_STATUS_REG;
	uint8_t status = 0;
	int retries = 20;

	do {
		int ret = i2c_write_read_dt(&lis2dh, &status_reg,
					    sizeof(status_reg), &status, sizeof(status));
		if (ret < 0) {
			LOG_ERR("STATUS_REG read failed (err %d)", ret);
			return ret;
		}
		if (status & BIT(3)) {
			break;
		}
		k_msleep(10);
	} while (--retries > 0);

	if (!(status & BIT(3))) {
		LOG_WRN("LIS2DH: data not ready after timeout, reading anyway");
	}

	int ret = i2c_write_read_dt(&lis2dh, &reg, sizeof(reg), raw, sizeof(raw));

	if (ret < 0) {
		LOG_ERR("Burst read failed (err %d)", ret);
		return ret;
	}

	/* Reconstruct 16-bit signed values (little-endian) */
	int16_t x = (int16_t)((raw[1] << 8) | raw[0]);
	int16_t y = (int16_t)((raw[3] << 8) | raw[2]);
	int16_t z = (int16_t)((raw[5] << 8) | raw[4]);

	LOG_INF("LIS2DH raw accel — X: %6d  Y: %6d  Z: %6d (LSB)", x, y, z);
	hex_dump("  raw bytes", raw, sizeof(raw));

	return 0;
}

/* -----------------------------------------------------------------------
 * Demo 4: i2c_write_dt + i2c_read_dt — EEPROM write then read back
 *
 * The 24CW160 uses 2-byte addressing. A write transaction is:
 *   [ADDR_HI, ADDR_LO, data0, data1, ...]
 * A read requires first writing the address, then reading separately.
 * This is the explicit two-step version — contrast with i2c_write_read_dt
 * which combines both into one transaction with a repeated START.
 * ----------------------------------------------------------------------- */
static int demo_eeprom_write_read(void)
{
	const uint8_t pattern[4] = { 0xDE, 0xAD, 0xBE, 0xEF };
	uint8_t readback[4]      = { 0 };

	/* --- Write: [addr_hi, addr_lo, data...] --- */
	uint8_t write_buf[2 + sizeof(pattern)];

	write_buf[0] = EEPROM_TEST_ADDR_HI;
	write_buf[1] = EEPROM_TEST_ADDR_LO;
	memcpy(&write_buf[2], pattern, sizeof(pattern));

	int ret = i2c_write_dt(&eeprom, write_buf, sizeof(write_buf));

	if (ret < 0) {
		LOG_ERR("EEPROM write failed (err %d)", ret);
		return ret;
	}

	hex_dump("EEPROM wrote", pattern, sizeof(pattern));

	/* 24CW160 write cycle time: 5 ms max */
	k_msleep(10);

	/* --- Seek: write just the address (positions internal pointer) --- */
	uint8_t addr_buf[2] = { EEPROM_TEST_ADDR_HI, EEPROM_TEST_ADDR_LO };

	ret = i2c_write_dt(&eeprom, addr_buf, sizeof(addr_buf));
	if (ret < 0) {
		LOG_ERR("EEPROM address seek failed (err %d)", ret);
		return ret;
	}

	/* --- Read back --- */
	ret = i2c_read_dt(&eeprom, readback, sizeof(readback));
	if (ret < 0) {
		LOG_ERR("EEPROM read failed (err %d)", ret);
		return ret;
	}

	hex_dump("EEPROM read ", readback, sizeof(readback));

	if (memcmp(pattern, readback, sizeof(pattern)) == 0) {
		LOG_INF("EEPROM write/read PASSED");
	} else {
		LOG_ERR("EEPROM write/read FAILED — data mismatch");
		return -EIO;
	}

	/* Clean up — zero the test location */
	uint8_t zero_buf[2 + sizeof(pattern)];

	zero_buf[0] = EEPROM_TEST_ADDR_HI;
	zero_buf[1] = EEPROM_TEST_ADDR_LO;
	memset(&zero_buf[2], 0x00, sizeof(pattern));
	i2c_write_dt(&eeprom, zero_buf, sizeof(zero_buf));
	k_msleep(10);

	return 0;
}

/* -----------------------------------------------------------------------
 * Demo 5: i2c_transfer_dt — low-level multi-message transaction
 *
 * Shows how to build a custom transaction with struct i2c_msg directly.
 * This is equivalent to i2c_write_read_dt but written explicitly so you
 * can see exactly what happens at the bus level.
 *
 * Transaction: WRITE(reg_addr) + RESTART + READ(1 byte)
 * ----------------------------------------------------------------------- */
static int demo_raw_transfer(void)
{
	uint8_t reg  = LIS2DH_WHO_AM_I;
	uint8_t val  = 0;

	struct i2c_msg msgs[2] = {
		{
			/* Message 0: write the register address */
			.buf   = &reg,
			.len   = 1,
			.flags = I2C_MSG_WRITE,
		},
		{
			/* Message 1: read the value (RESTART + STOP) */
			.buf   = &val,
			.len   = 1,
			.flags = I2C_MSG_RESTART | I2C_MSG_READ | I2C_MSG_STOP,
		},
	};

	int ret = i2c_transfer_dt(&lis2dh, msgs, ARRAY_SIZE(msgs));

	if (ret < 0) {
		LOG_ERR("i2c_transfer_dt failed (err %d)", ret);
		return ret;
	}

	LOG_INF("i2c_transfer_dt WHO_AM_I = 0x%02X (raw transfer — same result)", val);
	return 0;
}

/* -----------------------------------------------------------------------
 * main
 * ----------------------------------------------------------------------- */
int main(void)
{
	printk("I2C read/write sample running on %s\n", CONFIG_BOARD);

	/* Validate I2C bus */
	if (!i2c_is_ready_dt(&lis2dh)) {
		LOG_ERR("I2C bus not ready");
		return -ENODEV;
	}
	LOG_INF("I2C bus ready (SDA=P0.08, SCL=P0.07, 400 kHz)");

	int err = 0;

	/* --- Demo 1: Read a register with i2c_write_read_dt --- */
	LOG_INF("--- Demo 1: i2c_write_read_dt (read WHO_AM_I) ---");
	err = demo_read_register();
	if (err) {
		LOG_ERR("Demo 1 failed");
	}

	/* --- Demo 2: Write a register with i2c_write_dt --- */
	LOG_INF("--- Demo 2: i2c_write_dt (configure LIS2DH) ---");
	err = demo_write_register();
	if (err) {
		LOG_ERR("Demo 2 failed");
	}

	/* Allow the sensor to take its first sample after being enabled */
	k_msleep(200);

	/* --- Demo 3: Burst read with i2c_write_read_dt --- */
	LOG_INF("--- Demo 3: i2c_write_read_dt burst (6-byte accel data) ---");
	err = demo_burst_read();
	if (err) {
		LOG_ERR("Demo 3 failed");
	}

	/* --- Demo 4: EEPROM write then read with i2c_write_dt / i2c_read_dt --- */
	LOG_INF("--- Demo 4: i2c_write_dt + i2c_read_dt (EEPROM) ---");
	err = demo_eeprom_write_read();
	if (err) {
		LOG_ERR("Demo 4 failed");
	}

	/* --- Demo 5: Raw multi-message transfer with i2c_transfer_dt --- */
	LOG_INF("--- Demo 5: i2c_transfer_dt (raw struct i2c_msg) ---");
	err = demo_raw_transfer();
	if (err) {
		LOG_ERR("Demo 5 failed");
	}

	LOG_INF("I2C sample complete");
	return 0;
}
