#include "protocol_state.h"

static ProtocolState current_state = PROTO_STATE_DISCONNECTED;

const char* proto_state_name(ProtocolState s) {
    switch (s) {
        case PROTO_STATE_DISCONNECTED: return "Disconnected";
        case PROTO_STATE_CONNECTING: return "Connecting";
        case PROTO_STATE_CONNECTED: return "Connected";
        case PROTO_STATE_DISCOVERING: return "Discovering";
        case PROTO_STATE_READY: return "Ready";
        case PROTO_STATE_ERROR: return "Error";
        default: return "Unknown";
    }
}

ProtocolState proto_state_get(void) { return current_state; }
void proto_state_set(ProtocolState s) { current_state = s; }