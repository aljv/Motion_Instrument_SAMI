#pragma once

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <bluetooth/gatt_dm.h>
#include <bluetooth/scan.h>

#include <zephyr/bluetooth/services/bas.h>
#include <zephyr/bluetooth/services/hrs.h>
#include <bluetooth/services/hrs_client.h>

#include <zephyr/settings/settings.h>

#include <zephyr/kernel.h>

#define STACKSIZE 1024
#define PRIORITY 7

#define RUN_STATUS_LED             DK_LED1
#define CENTRAL_CON_STATUS_LED	   DK_LED2
#define PERIPHERAL_CONN_STATUS_LED DK_LED3

#define RUN_LED_BLINK_INTERVAL 1000

#define HRS_QUEUE_SIZE 16

static const struct bt_data ad[] = {
        BT_DATA_BYTES(BT_DATA_GAP_APPEARANCE,
                (CONFIG_BT_DEVICE_APPEARANCE >> 0) & 0xff,
                (CONFIG_BT_DEVICE_APPEARANCE >> 8) & 0xff),
        BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
        BT_DATA_BYTES(BT_DATA_UUID16_ALL,
                      BT_UUID_16_ENCODE(BT_UUID_HRS_VAL)), /* Heart Rate Service */
};

static void hrs_sensor_location_read_cb(struct bt_hrs_client *hrs_c,
                                        enum bt_hrs_client_sensor_location location,
                                        int err);

static void hrs_measurement_notify_cb(struct bt_hrs_client *hrs_c,
                                      const struct bt_hrs_client_measurement *meas,
                                      int err);

static void discovery_completed_cb(struct bt_gatt_dm *dm, void *context);
static void discovery_not_found_cb(struct bt_conn *conn, void *context);
static void discovery_error_found_cb(struct bt_conn *conn, int err, void *context);

static void gatt_discover(struct bt_conn *conn);
static void auth_cancel(struct bt_conn *conn);
static void pairing_complete(struct bt_conn *conn, bool bonded);
static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason);
static int scan_start(void);
static void connected(struct bt_conn *conn, uint8_t conn_err);
static void disconnected(struct bt_conn *conn, uint8_t reason);
static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err);

static void scan_filter_match(struct bt_scan_device_info *device_info,
                              struct bt_scan_filter_match *filter_match,
                              bool connectable);

static void scan_connecting_error(struct bt_scan_device_info *device_info);
static void scan_connecting(struct bt_scan_device_info *device_info,
                            struct bt_conn *conn);

static void scan_init(void);
static void hrs_notify_thread(void);

// callbacks
static const struct bt_conn_auth_cb auth_callbacks;
static struct bt_conn_auth_info_cb conn_auth_info_callbacks;