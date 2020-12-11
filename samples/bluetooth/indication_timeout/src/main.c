/* main.c - Application main entry point */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <sys/printk.h>
#include <sys/byteorder.h>
#include <zephyr.h>

#include <settings/settings.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#define SERVICE_UUID 0xAAAA

static ssize_t ccc_written(struct bt_conn *conn,
			   const struct bt_gatt_attr *attr,
			   uint16_t value);
static void indicate_cb(struct bt_conn *conn,
			struct bt_gatt_indicate_params *params,
			uint8_t err);

/* GATT Definitions */
static struct bt_uuid_16 service_uuid = BT_UUID_INIT_16(SERVICE_UUID);

static struct bt_uuid_128 characteristic_uuid = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0x9ac91116, 0xc657, 0x1261, 0x5895, 0x0d5000009597));

BT_GATT_SERVICE_DEFINE(service,
		       BT_GATT_PRIMARY_SERVICE(&service_uuid),
		       BT_GATT_CHARACTERISTIC(&characteristic_uuid.uuid,
					      BT_GATT_CHRC_INDICATE,
					      BT_GATT_PERM_READ | BT_GATT_PERM_READ_AUTHEN,
					      NULL, NULL, NULL),
		       BT_GATT_CCC_MANAGED(((struct _bt_gatt_ccc[])
					    { BT_GATT_CCC_INITIALIZER(NULL, ccc_written, NULL) }),
					   BT_GATT_PERM_READ | BT_GATT_PERM_WRITE | BT_GATT_PERM_WRITE_AUTHEN)
		       );

static ssize_t ccc_written(struct bt_conn *conn,
			   const struct bt_gatt_attr *attr,
			   uint16_t value)
{
	printk("CCC written: %d\n", value);
	return sizeof(value);
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		printk("Connection failed (err 0x%02x)\n", err);
	} else {
		printk("Connected\n");
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected (reason 0x%02x)\n", reason);
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL,
		      BT_UUID_16_ENCODE(SERVICE_UUID)),
};

static void bt_ready(void)
{
	int err;

	printk("Bluetooth initialized\n");

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");
}

/* Big slab of indications so we can queue many indications */
K_MEM_SLAB_DEFINE(indication_params_slab,
		  sizeof(struct bt_gatt_indicate_params),
		  20,
		  _Alignof(struct bt_gatt_indicate_params));

static void indicate_cb(struct bt_conn *conn,
			struct bt_gatt_indicate_params *params, uint8_t err)
{
	if (err) {
		printk("Indication failed %d\n", err);
	}
}

static void indicate_destroy(struct bt_gatt_indicate_params *params)
{
	k_mem_slab_free(&indication_params_slab, (void **)&params);
}

void main(void)
{
	const char *test_data = "INDICATION";
	struct bt_gatt_indicate_params *params;
	int err;

	/* Setup Bluetooth */
	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}
	bt_ready();
	bt_conn_cb_register(&conn_callbacks);
	k_sleep(K_SECONDS(2));

	while (1) {
		/* Continuously queue as many indications as we can */
		k_mem_slab_alloc(&indication_params_slab,
				 (void **)&params, K_FOREVER);

		params->attr = &service.attrs[2];
		params->uuid = NULL;
		params->data = test_data;
		params->len = 10;
		params->func = indicate_cb;
		params->destroy = indicate_destroy;

		err = bt_gatt_indicate(NULL, params);
		if (err < 0) {
			/* ENOTCONN is expected before we connect */
			if (err != -ENOTCONN) {
				printk("Indication not started: %d\n", err);
			}
			/* Free parameter struct on error */
			k_mem_slab_free(&indication_params_slab,
					(void **)&params);
		}
	}
}
