#include "casio_ble_central.h"

#include <furi.h>
#include <string.h>

#if defined(__has_include)
#if __has_include("ble_gap_aci.h")
#define CASIO_BLE_ACI_AVAILABLE 1
#include "ble_gap_aci.h"
#include "ble_gatt_aci.h"
#include "ble_hci_le.h"
#include "ble_types.h"
#endif
#endif
#ifndef CASIO_BLE_ACI_AVAILABLE
#define CASIO_BLE_ACI_AVAILABLE 0
#endif

#define TAG "CasioBleCentral"

/* Maximum number of GATT attributes we track per connection. */
#define CASIO_BLE_MAX_SERVICES 8
#define CASIO_BLE_MAX_CHARS    32

typedef struct {
    uint16_t handle;
    uint16_t uuid;       /* 16-bit UUID (short form) */
} CasioBleAttr;

typedef struct {
    uint16_t start_handle;
    uint16_t end_handle;
    uint16_t uuid;
} CasioBleService;

struct CasioBleCentralInternal {
    CasioBleCentral pub;
    CasioBleService services[CASIO_BLE_MAX_SERVICES];
    uint8_t service_count;
    CasioBleAttr chars[CASIO_BLE_MAX_CHARS];
    uint8_t char_count;
    uint16_t notify_cccd_handle;
    bool discovering;
};

/* We keep a single static instance to avoid malloc on the 4 KB FAP stack. */
static struct CasioBleCentralInternal g_central;

void casio_ble_central_init(CasioBleCentral* c) {
    if(!c) return;
    memset(c, 0, sizeof(*c));
    c->state = CasioBleIdle;
    c->conn_handle = 0xFFFF;
    g_central.service_count = 0;
    g_central.char_count = 0;
    g_central.notify_cccd_handle = 0;
    g_central.discovering = false;
}

void casio_ble_central_deinit(CasioBleCentral* c) {
    if(!c) return;
    if(c->state == CasioBleConnected || c->state == CasioBleReady) {
        casio_ble_disconnect(c);
    }
    c->state = CasioBleIdle;
}

int casio_ble_scan_start(CasioBleCentral* c) {
    if(!c) return -1;
    if(c->state != CasioBleIdle) return -2;

#if CASIO_BLE_ACI_AVAILABLE
    tBleStatus st = aci_gap_start_observation_proc(
        0x0010, 0x0010, 0x00, 0x01, 0x00, 0x00);
    if(st != BLE_STATUS_SUCCESS) {
        c->last_status = st;
        c->state = CasioBleError;
        FURI_LOG_E(TAG, "scan start failed: 0x%02X", st);
        return -3;
    }
#endif

    c->state = CasioBleScanning;
    FURI_LOG_I(TAG, "scan started");
    return 0;
}

int casio_ble_scan_stop(CasioBleCentral* c) {
    if(!c) return -1;
    if(c->state != CasioBleScanning) return -2;

#if CASIO_BLE_ACI_AVAILABLE
    tBleStatus st = aci_gap_terminate_gap_proc(0x00);
    if(st != BLE_STATUS_SUCCESS) {
        c->last_status = st;
        FURI_LOG_E(TAG, "scan stop failed: 0x%02X", st);
        return -3;
    }
#endif

    c->state = CasioBleIdle;
    FURI_LOG_I(TAG, "scan stopped");
    return 0;
}

int casio_ble_connect(CasioBleCentral* c, const uint8_t* addr, uint8_t addr_type) {
    if(!c || !addr) return -1;
    if(c->state != CasioBleScanning && c->state != CasioBleIdle) return -2;

    memcpy(c->peer_addr, addr, 6);
    c->state = CasioBleConnecting;

#if CASIO_BLE_ACI_AVAILABLE
    tBleStatus st = aci_gap_create_connection(
        0x0010, 0x0010, addr_type, (const uint8_t*)addr,
        0x00, 0x0006, 0x000C, 0x0000, 0x00C8, 0x0000, 0x0000);
    if(st != BLE_STATUS_SUCCESS) {
        c->last_status = st;
        c->state = CasioBleError;
        FURI_LOG_E(TAG, "connect failed: 0x%02X", st);
        return -3;
    }
#endif

    FURI_LOG_I(TAG, "connect requested");
    return 0;
}

int casio_ble_disconnect(CasioBleCentral* c) {
    if(!c) return -1;
    if(c->conn_handle == 0xFFFF) return -2;

#if CASIO_BLE_ACI_AVAILABLE
    tBleStatus st = hci_disconnect(c->conn_handle, 0x13);
    if(st != BLE_STATUS_SUCCESS) {
        c->last_status = st;
        FURI_LOG_E(TAG, "disconnect failed: 0x%02X", st);
        return -3;
    }
#endif

    c->conn_handle = 0xFFFF;
    c->state = CasioBleIdle;
    g_central.service_count = 0;
    g_central.char_count = 0;
    FURI_LOG_I(TAG, "disconnect requested");
    return 0;
}

CasioBleState casio_ble_state(const CasioBleCentral* c) {
    if(!c) return CasioBleIdle;
    return c->state;
}

/* ---- GATT discovery ---- */

int casio_ble_discover_services(CasioBleCentral* c) {
    if(!c || c->conn_handle == 0xFFFF) return -1;
    g_central.service_count = 0;
    g_central.discovering = true;
    c->state = CasioBleDiscovering;

#if CASIO_BLE_ACI_AVAILABLE
    tBleStatus st = aci_gatt_disc_all_primary_services(c->conn_handle);
    if(st != BLE_STATUS_SUCCESS) {
        c->last_status = st;
        c->state = CasioBleError;
        return -2;
    }
#endif
    return 0;
}

int casio_ble_discover_chars(CasioBleCentral* c, uint16_t svc_start, uint16_t svc_end) {
    if(!c || c->conn_handle == 0xFFFF) return -1;

#if CASIO_BLE_ACI_AVAILABLE
    tBleStatus st = aci_gatt_disc_all_char_of_service(
        c->conn_handle, svc_start, svc_end);
    if(st != BLE_STATUS_SUCCESS) {
        c->last_status = st;
        return -2;
    }
#endif
    return 0;
}

/* ---- Read / Write ---- */

int casio_ble_read_char(CasioBleCentral* c, uint16_t char_handle) {
    if(!c || c->conn_handle == 0xFFFF) return -1;

#if CASIO_BLE_ACI_AVAILABLE
    tBleStatus st = aci_gatt_read_char_value(c->conn_handle, char_handle);
    if(st != BLE_STATUS_SUCCESS) {
        c->last_status = st;
        return -2;
    }
#endif
    return 0;
}

int casio_ble_read_long_char(CasioBleCentral* c, uint16_t char_handle, uint16_t offset) {
    if(!c || c->conn_handle == 0xFFFF) return -1;

#if CASIO_BLE_ACI_AVAILABLE
    tBleStatus st = aci_gatt_read_long_char_value(c->conn_handle, char_handle, offset);
    if(st != BLE_STATUS_SUCCESS) {
        c->last_status = st;
        return -2;
    }
#endif
    return 0;
}

int casio_ble_write_char(CasioBleCentral* c, uint16_t char_handle,
                         const uint8_t* data, uint8_t len) {
    if(!c || !data || c->conn_handle == 0xFFFF) return -1;

#if CASIO_BLE_ACI_AVAILABLE
    tBleStatus st = aci_gatt_write_char_value(c->conn_handle, char_handle, len, (const uint8_t*)data);
    if(st != BLE_STATUS_SUCCESS) {
        c->last_status = st;
        return -2;
    }
#endif
    return 0;
}

int casio_ble_write_without_resp(CasioBleCentral* c, uint16_t char_handle,
                                 const uint8_t* data, uint8_t len) {
    if(!c || !data || c->conn_handle == 0xFFFF) return -1;

#if CASIO_BLE_ACI_AVAILABLE
    tBleStatus st = aci_gatt_write_without_resp(c->conn_handle, char_handle, len, (const uint8_t*)data);
    if(st != BLE_STATUS_SUCCESS) {
        c->last_status = st;
        return -2;
    }
#endif
    return 0;
}

/* ---- Notifications ---- */

int casio_ble_subscribe(CasioBleCentral* c, uint16_t cccd_handle) {
    if(!c || cccd_handle == 0 || c->conn_handle == 0xFFFF) return -1;
    g_central.notify_cccd_handle = cccd_handle;

#if CASIO_BLE_ACI_AVAILABLE
    uint8_t cccd[2] = {0x01, 0x00}; /* enable notifications */
    tBleStatus st = aci_gatt_write_char_desc(c->conn_handle, cccd_handle, 2, cccd);
    if(st != BLE_STATUS_SUCCESS) {
        c->last_status = st;
        return -2;
    }
#endif
    return 0;
}

int casio_ble_unsubscribe(CasioBleCentral* c) {
    if(!c || g_central.notify_cccd_handle == 0 || c->conn_handle == 0xFFFF) return -1;

#if CASIO_BLE_ACI_AVAILABLE
    uint8_t cccd[2] = {0x00, 0x00}; /* disable */
    tBleStatus st = aci_gatt_write_char_desc(
        c->conn_handle, g_central.notify_cccd_handle, 2, cccd);
    if(st != BLE_STATUS_SUCCESS) {
        c->last_status = st;
        return -2;
    }
#endif
    g_central.notify_cccd_handle = 0;
    return 0;
}

/* ---- Pairing (Legacy Just Works) ---- */

int casio_ble_pair(CasioBleCentral* c) {
    if(!c || c->conn_handle == 0xFFFF) return -1;

#if CASIO_BLE_ACI_AVAILABLE
    /* IO capability: no input, no output → Just Works. */
    tBleStatus st = aci_gap_set_io_capability(IO_CAP_NO_INPUT_NO_OUTPUT);
    if(st != BLE_STATUS_SUCCESS) {
        c->last_status = st;
        return -2;
    }
    /* Auth: bonding, no MITM, Legacy (SC not supported), key size 7. */
    st = aci_gap_set_authentication_requirement(
        0x01,  /* bonding */
        0x00,  /* MITM not required */
        0x00,  /* SC not supported (Legacy) */
        0x00,  /* key press not supported */
        7,     /* max encryption key size */
        0x00,  /* initiator key distribution: none */
        0x01,  /* responder key distribution: EncKey */
        0x00000000,
        0x00);
    if(st != BLE_STATUS_SUCCESS) {
        c->last_status = st;
        return -3;
    }
    /* Start pairing. */
    st = aci_gap_send_pairing_req(c->conn_handle, 0x00);
    if(st != BLE_STATUS_SUCCESS) {
        c->last_status = st;
        return -4;
    }
#endif

    FURI_LOG_I(TAG, "pairing requested (Legacy Just Works)");
    return 0;
}

/* ---- MTU exchange ---- */

int casio_ble_exchange_mtu(CasioBleCentral* c) {
    if(!c || c->conn_handle == 0xFFFF) return -1;

#if CASIO_BLE_ACI_AVAILABLE
    tBleStatus st = aci_gatt_exchange_config(c->conn_handle);
    if(st != BLE_STATUS_SUCCESS) {
        c->last_status = st;
        return -2;
    }
#endif
    return 0;
}

/*
 * NOTE: The event dispatcher hook (register/unregister) and the ACI event
 * handler are NOT implemented here because they require the Momentum-specific
 * ble_event_dispatcher API, which is not part of the CasioLab FAP build yet.
 * They will be added when the FAP links against the exported symbols (M5).
 *
 * Until then, this module compiles (guarded by __has_include) but the ACI
 * calls are no-ops on non-Momentum builds.
 */