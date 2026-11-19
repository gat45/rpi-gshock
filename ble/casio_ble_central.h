#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/*
 * casio_ble_central — Flipper-as-Central driver for the DW-H5600.
 *
 * Uses the ST ACI commands exported by the Momentum firmware (M3/M5).
 * The firmware must be built with the Central+Observer GAP roles (M3) and the
 * Full BLE stack (M4); otherwise aci_gap_* return BLE_STATUS_FAILED.
 *
 * Status: skeleton. Scan + connection entry points are implemented; GATT
 * discovery, read/write and pairing are stubs pending the dispatcher wiring.
 */

#define CASIO_BLE_MAX_CONN 1

typedef enum {
    CasioBleIdle = 0,
    CasioBleScanning,
    CasioBleConnecting,
    CasioBleConnected,
    CasioBleDiscovering,
    CasioBleReady,
    CasioBleError,
} CasioBleState;

typedef struct {
    uint16_t conn_handle;
    uint8_t peer_addr[6];
    CasioBleState state;
    int last_status;
} CasioBleCentral;

/* Lifecycle. */
void casio_ble_central_init(CasioBleCentral* c);
void casio_ble_central_deinit(CasioBleCentral* c);

/* Scan for the DW-H5600 (filter on name prefix "CASIO"). */
int casio_ble_scan_start(CasioBleCentral* c);
int casio_ble_scan_stop(CasioBleCentral* c);

/* Connect to the peer found during scan (addr_type 0 = public). */
int casio_ble_connect(CasioBleCentral* c, const uint8_t* addr, uint8_t addr_type);

/* Disconnect. */
int casio_ble_disconnect(CasioBleCentral* c);

/* State accessor. */
CasioBleState casio_ble_state(const CasioBleCentral* c);

/* GATT discovery. */
int casio_ble_discover_services(CasioBleCentral* c);
int casio_ble_discover_chars(CasioBleCentral* c, uint16_t svc_start, uint16_t svc_end);

/* Read / Write. */
int casio_ble_read_char(CasioBleCentral* c, uint16_t char_handle);
int casio_ble_read_long_char(CasioBleCentral* c, uint16_t char_handle, uint16_t offset);
int casio_ble_write_char(CasioBleCentral* c, uint16_t char_handle,
                         const uint8_t* data, uint8_t len);
int casio_ble_write_without_resp(CasioBleCentral* c, uint16_t char_handle,
                                 const uint8_t* data, uint8_t len);

/* Notifications. */
int casio_ble_subscribe(CasioBleCentral* c, uint16_t cccd_handle);
int casio_ble_unsubscribe(CasioBleCentral* c);

/* Pairing (Legacy Just Works). */
int casio_ble_pair(CasioBleCentral* c);

/* MTU exchange. */
int casio_ble_exchange_mtu(CasioBleCentral* c);
