/*
 * Copyright (c) 2026 Conexio Technologies, Inc.
 * Author: Rajeev Piyare <rajeev@conexiotech.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * NVS (Non-Volatile Storage) sample for Conexio Stratus Pro (nRF9151)
 *
 * NVS is a lightweight key-value store that lives in a dedicated flash
 * partition and survives power cycles and reboots.  It is the recommended
 * way to persist application configuration on nRF91 devices.
 *
 * Storage location:
 *   storage_partition — flash offset 0xfa000, size 24 KiB
 *   Sector size: 4 KiB (one flash erase block)
 *   Sector count: 3 — NVS uses these for wear-levelling and GC
 *
 * ─────────────────────────────────────────────────────────────────────
 * API summary (all in <zephyr/fs/nvs.h>)
 * ─────────────────────────────────────────────────────────────────────
 *
 *   nvs_mount(fs)
 *     Mount the NVS file system.  Must be called once before any
 *     read/write operations.  Safe to call on every boot — NVS
 *     recovers existing data automatically.
 *
 *   nvs_write(fs, id, data, len)
 *     Write `len` bytes from `data` to entry with key `id`.
 *     Returns bytes written (== len) on success, negative errno on error.
 *     If the same data is already stored under `id`, nothing is written
 *     to flash (returns 0) — avoids unnecessary wear.
 *
 *   nvs_read(fs, id, data, len)
 *     Read up to `len` bytes from entry `id` into `data`.
 *     Returns bytes read on success, negative errno if not found (-ENOENT).
 *     If the stored value is larger than `len`, returns the stored size
 *     so you know how much was available.
 *
 *   nvs_delete(fs, id)
 *     Delete entry `id`.  Subsequent reads return -ENOENT.
 *
 *   nvs_calc_free_space(fs)
 *     Returns available bytes remaining in the NVS partition.
 *
 * ─────────────────────────────────────────────────────────────────────
 * Key ID assignments (application-defined uint16_t)
 * ─────────────────────────────────────────────────────────────────────
 *   IDs are arbitrary 16-bit unsigned integers.  Define them as
 *   constants or an enum so they never accidentally collide.
 *   IDs 0x0000 and 0xFFFF are reserved by NVS — do not use them.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/fs/nvs.h>
#include <zephyr/storage/flash_map.h>
#include <string.h>

LOG_MODULE_REGISTER(nvs_storage, LOG_LEVEL_DBG);

/* -----------------------------------------------------------------------
 * NVS key IDs — never reuse or renumber these once deployed
 * ----------------------------------------------------------------------- */
enum nvs_key_id {
	NVS_ID_BOOT_COUNT  = 1,   /* uint32_t — increments on every boot     */
	NVS_ID_DEVICE_NAME = 2,   /* char[]   — human-readable device name   */
	NVS_ID_THRESHOLD   = 3,   /* int32_t  — configurable threshold value */
	NVS_ID_CONFIG      = 4,   /* struct   — packed application config    */
	NVS_ID_SECRET      = 5,   /* uint8_t[]— example of a byte blob       */
};

/* -----------------------------------------------------------------------
 * Application config struct — shows how to persist structured data
 * ----------------------------------------------------------------------- */
struct app_config {
	uint32_t sample_interval_ms;
	int32_t  alert_threshold;
	uint8_t  flags;           /* bit 0 = LED enabled, bit 1 = verbose   */
	uint8_t  reserved[3];     /* pad to 4-byte alignment                 */
};

/* -----------------------------------------------------------------------
 * NVS filesystem instance
 * ----------------------------------------------------------------------- */
static struct nvs_fs nvs;

/* -----------------------------------------------------------------------
 * nvs_init — mount NVS and print partition info
 * ----------------------------------------------------------------------- */
static int nvs_init(void)
{
	int ret;

	/* Locate the storage partition using the FIXED_PARTITION_ID macro.
	 * This resolves the flash device and byte offset from the DT label. */
	const struct flash_area *fa;

	ret = flash_area_open(FIXED_PARTITION_ID(storage_partition), &fa);
	if (ret < 0) {
		LOG_ERR("Cannot open storage_partition: %d", ret);
		return ret;
	}

	nvs.flash_device = flash_area_get_device(fa);
	if (!device_is_ready(nvs.flash_device)) {
		LOG_ERR("Flash device not ready");
		flash_area_close(fa);
		return -ENODEV;
	}

	nvs.offset       = fa->fa_off;
	nvs.sector_size  = 0x1000; /* 4 KiB — one nRF9151 flash erase block  */
	nvs.sector_count = 3;      /* 3 × 4 KiB = 12 KiB for NVS             */

	flash_area_close(fa);

	ret = nvs_mount(&nvs);
	if (ret < 0) {
		LOG_ERR("nvs_mount failed: %d", ret);
		return ret;
	}

	LOG_INF("NVS mounted successfully");
	LOG_INF("  Flash device : %s", nvs.flash_device->name);
	LOG_INF("  Offset       : 0x%08lX", (unsigned long)nvs.offset);
	LOG_INF("  Sector size  : %u bytes", nvs.sector_size);
	LOG_INF("  Sector count : %u", nvs.sector_count);
	LOG_INF("  Total NVS    : %u bytes",
		nvs.sector_size * nvs.sector_count);

	ssize_t free_space = nvs_calc_free_space(&nvs);

	if (free_space >= 0) {
		LOG_INF("  Free space   : %zd bytes", free_space);
	}

	return 0;
}

/* -----------------------------------------------------------------------
 * Demo 1: Boot counter
 *
 * A classic first use-case: count how many times the device has booted.
 * On first boot, nvs_read returns -ENOENT (not found) — initialise to 0.
 * On every subsequent boot, read the current value, increment, and write.
 *
 * Note: nvs_write is a no-op if the value hasn't changed (returns 0).
 * This matters for the boot counter because it always changes, so every
 * boot causes one flash write — acceptable for a low-write-frequency key.
 * ----------------------------------------------------------------------- */
static void demo_boot_counter(void)
{
	uint32_t boot_count = 0;
	ssize_t  bytes;

	LOG_INF("--- Demo 1: Boot counter ---");

	bytes = nvs_read(&nvs, NVS_ID_BOOT_COUNT, &boot_count,
			 sizeof(boot_count));
	if (bytes == sizeof(boot_count)) {
		LOG_INF("Previous boot count: %u — incrementing", boot_count);
		boot_count++;
	} else if (bytes == -ENOENT) {
		LOG_INF("No boot count found (first boot) — initialising to 1");
		boot_count = 1;
	} else {
		LOG_ERR("nvs_read boot_count error: %zd", bytes);
		return;
	}

	bytes = nvs_write(&nvs, NVS_ID_BOOT_COUNT, &boot_count,
			  sizeof(boot_count));
	if (bytes < 0) {
		LOG_ERR("nvs_write boot_count failed: %zd", bytes);
	} else if (bytes == 0) {
		LOG_DBG("boot_count unchanged — no flash write needed");
	} else {
		LOG_INF("Boot count saved: %u (%zd bytes written)",
			boot_count, bytes);
	}
}

/* -----------------------------------------------------------------------
 * Demo 2: String storage
 *
 * Store and retrieve a human-readable device name.  Strings are stored
 * as null-terminated char arrays — include the '\0' in the length.
 * ----------------------------------------------------------------------- */
static void demo_device_name(void)
{
	char name[32] = {0};
	ssize_t bytes;

	LOG_INF("--- Demo 2: String storage ---");

	/* Try to read an existing name */
	bytes = nvs_read(&nvs, NVS_ID_DEVICE_NAME, name, sizeof(name));
	if (bytes > 0) {
		LOG_INF("Stored device name: \"%s\" (%zd bytes)", name, bytes);
	} else if (bytes == -ENOENT) {
		/* First time — write a default name */
		const char *default_name = "Stratus-Pro-001";

		bytes = nvs_write(&nvs, NVS_ID_DEVICE_NAME,
				  default_name, strlen(default_name) + 1);
		if (bytes > 0) {
			LOG_INF("Device name initialised: \"%s\"", default_name);
		} else {
			LOG_ERR("Failed to write device name: %zd", bytes);
		}
	} else {
		LOG_ERR("nvs_read device_name error: %zd", bytes);
	}

	/* Demonstrate updating the name on every other boot */
	uint32_t boot_count = 0;

	nvs_read(&nvs, NVS_ID_BOOT_COUNT, &boot_count, sizeof(boot_count));

	if (boot_count > 1 && (boot_count % 2) == 0) {
		char new_name[32];

		snprintk(new_name, sizeof(new_name), "Stratus-Pro-%03u",
			 boot_count);
		bytes = nvs_write(&nvs, NVS_ID_DEVICE_NAME,
				  new_name, strlen(new_name) + 1);
		if (bytes > 0) {
			LOG_INF("Device name updated to: \"%s\"", new_name);
		} else if (bytes == 0) {
			LOG_DBG("Device name unchanged");
		}
	}
}

/* -----------------------------------------------------------------------
 * Demo 3: Structured config persistence
 *
 * Shows how to store and retrieve a C struct.  The struct is treated as
 * a raw byte blob — no serialisation library needed for simple flat
 * structures.  Avoid pointers and variable-length arrays inside structs
 * stored in NVS.
 * ----------------------------------------------------------------------- */
static void demo_config(void)
{
	struct app_config cfg;
	ssize_t bytes;

	LOG_INF("--- Demo 3: Structured config ---");

	bytes = nvs_read(&nvs, NVS_ID_CONFIG, &cfg, sizeof(cfg));
	if (bytes == sizeof(cfg)) {
		LOG_INF("Loaded config:");
		LOG_INF("  sample_interval_ms : %u", cfg.sample_interval_ms);
		LOG_INF("  alert_threshold    : %d", cfg.alert_threshold);
		LOG_INF("  flags              : 0x%02X (LED=%s, verbose=%s)",
			cfg.flags,
			(cfg.flags & BIT(0)) ? "on" : "off",
			(cfg.flags & BIT(1)) ? "yes" : "no");
	} else if (bytes == -ENOENT) {
		/* First boot — write factory defaults */
		cfg.sample_interval_ms = 5000;
		cfg.alert_threshold    = 75;
		cfg.flags              = BIT(0); /* LED enabled */
		memset(cfg.reserved, 0, sizeof(cfg.reserved));

		bytes = nvs_write(&nvs, NVS_ID_CONFIG, &cfg, sizeof(cfg));
		if (bytes > 0) {
			LOG_INF("Config initialised with factory defaults");
		} else {
			LOG_ERR("Failed to write config: %zd", bytes);
		}
	} else {
		LOG_ERR("nvs_read config error: %zd", bytes);
	}
}

/* -----------------------------------------------------------------------
 * Demo 4: Delete an entry
 *
 * nvs_delete removes an entry.  The next nvs_read for that ID returns
 * -ENOENT.  NVS marks the sector as deleted and reclaims the space
 * during the next garbage-collection cycle.
 * ----------------------------------------------------------------------- */
static void demo_delete(void)
{
	int32_t threshold = 42;
	ssize_t bytes;

	LOG_INF("--- Demo 4: Delete ---");

	/* Write a value */
	bytes = nvs_write(&nvs, NVS_ID_THRESHOLD, &threshold,
			  sizeof(threshold));
	LOG_INF("Wrote threshold=%d (%zd bytes)", threshold, bytes);

	/* Read it back */
	int32_t readback = 0;

	bytes = nvs_read(&nvs, NVS_ID_THRESHOLD, &readback, sizeof(readback));
	LOG_INF("Read back threshold=%d (%zd bytes)", readback, bytes);

	/* Delete it */
	int ret = nvs_delete(&nvs, NVS_ID_THRESHOLD);

	LOG_INF("Deleted threshold entry (ret=%d)", ret);

	/* Verify deletion */
	bytes = nvs_read(&nvs, NVS_ID_THRESHOLD, &readback, sizeof(readback));
	if (bytes == -ENOENT) {
		LOG_INF("Confirmed: threshold entry is gone (-ENOENT)");
	} else {
		LOG_WRN("Unexpected read result after delete: %zd", bytes);
	}
}

/* -----------------------------------------------------------------------
 * Demo 5: Binary blob storage (e.g. a device secret or crypto key)
 * ----------------------------------------------------------------------- */
static void demo_blob(void)
{
	const uint8_t secret_write[] = {
		0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE
	};
	uint8_t secret_read[sizeof(secret_write)] = {0};
	ssize_t bytes;

	LOG_INF("--- Demo 5: Binary blob ---");

	bytes = nvs_read(&nvs, NVS_ID_SECRET, secret_read,
			 sizeof(secret_read));
	if (bytes == sizeof(secret_read)) {
		LOG_HEXDUMP_INF(secret_read, sizeof(secret_read),
				"Loaded secret:");
	} else if (bytes == -ENOENT) {
		bytes = nvs_write(&nvs, NVS_ID_SECRET, secret_write,
				  sizeof(secret_write));
		if (bytes > 0) {
			LOG_HEXDUMP_INF(secret_write, sizeof(secret_write),
					"Secret written:");
		} else {
			LOG_ERR("Failed to write secret: %zd", bytes);
		}
	} else {
		LOG_ERR("nvs_read secret error: %zd", bytes);
	}
}

/* -----------------------------------------------------------------------
 * main
 * ----------------------------------------------------------------------- */
int main(void)
{
	int ret;

	printk("\n========================================\n");
	printk("  Conexio Stratus Pro — NVS Storage\n");
	printk("  Board: %s\n", CONFIG_BOARD);
	printk("========================================\n\n");

	/* Mount NVS — must be done before any read/write */
	ret = nvs_init();
	if (ret < 0) {
		LOG_ERR("NVS init failed: %d", ret);
		return ret;
	}

	/* Run all demos */
	demo_boot_counter();
	demo_device_name();
	demo_config();
	demo_delete();
	demo_blob();

	/* Final free space report */
	ssize_t free_space = nvs_calc_free_space(&nvs);

	LOG_INF("--- Summary ---");
	LOG_INF("NVS free space remaining: %zd bytes", free_space);
	LOG_INF("NVS sample complete. Reset the board to see boot count increment.");

	return 0;
}
