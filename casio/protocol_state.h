#pragma once
#include <furi.h>

typedef enum {
    PROTO_STATE_DISCONNECTED = 0,
    PROTO_STATE_CONNECTING,
    PROTO_STATE_CONNECTED,
    PROTO_STATE_DISCOVERING,
    PROTO_STATE_READY,
    PROTO_STATE_ERROR,
} ProtocolState;

const char* proto_state_name(ProtocolState s);
ProtocolState proto_state_get(void);
void proto_state_set(ProtocolState s);