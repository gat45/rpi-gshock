#include "command_catalog.h"

/*
 * Hand-written safety policy. Ids and names follow the reference clients
 * (gshock_api Protocol enum, GShockAPI CasioConstants.CHARACTERISTICS) and are
 * cross-checked against command_observed.h: everything listed here either was
 * seen on the wire or is a documented read request.
 */
static const CmdEntry catalog[] = {
    {0x09, "CURRENT_TIME", CMD_SAFETY_SAFE, "Read current time"},
    {0x0A, "ALERT/FIND_PHONE", CMD_SAFETY_CAUTION, "Alert / find phone"},
    {0x10, "BLE_FEATURES", CMD_SAFETY_SAFE, "BLE feature report"},
    {0x11, "SETTING_FOR_BLE", CMD_SAFETY_DANGEROUS, "BLE settings (advertising)"},
    {0x13, "SETTING_FOR_BASIC", CMD_SAFETY_DANGEROUS, "Basic settings"},
    {0x15, "SETTING_FOR_ALM", CMD_SAFETY_DANGEROUS, "Alarm settings"},
    {0x16, "SETTING_FOR_ALM2", CMD_SAFETY_DANGEROUS, "Alarm 2 settings"},
    {0x18, "TIMER", CMD_SAFETY_CAUTION, "Timer / stopwatch"},
    {0x1D, "DST_WATCH_STATE", CMD_SAFETY_SAFE, "DST state of the watch"},
    {0x1E, "DST_SETTING", CMD_SAFETY_DANGEROUS, "DST setting"},
    {0x1F, "WORLD_CITIES", CMD_SAFETY_CAUTION, "World cities / home city"},
    {0x20, "VERSION", CMD_SAFETY_SAFE, "Firmware version"},
    {0x22, "APP_INFO", CMD_SAFETY_SAFE, "Application information"},
    {0x23, "WATCH_NAME", CMD_SAFETY_SAFE, "Watch model name (ASCII)"},
    {0x24, "HOME_TIME", CMD_SAFETY_SAFE, "Home city time"},
    {0x26, "ACTIVITY_RECORD", CMD_SAFETY_SAFE, "Step counter / activity record"},
    {0x28, "WATCH_CONDITION", CMD_SAFETY_SAFE, "Battery level and temperature"},
    {0x2D, "SETTING_FOR_EXERCISE", CMD_SAFETY_DANGEROUS, "Exercise profile"},
    {0x30, "REMINDER_TITLE", CMD_SAFETY_DANGEROUS, "Reminder title"},
    {0x31, "REMINDER_TIME", CMD_SAFETY_DANGEROUS, "Reminder time"},
    {0x39, "CURRENT_TIME_MANAGER", CMD_SAFETY_CAUTION, "Time manager"},
    {0x3A, "CONNECTION_PARAMETER", CMD_SAFETY_DANGEROUS, "Connection parameters"},
    {0x3B, "ADVERTISE_PARAMETER", CMD_SAFETY_DANGEROUS, "Advertising parameters"},
    {0x43, "SETTING_FOR_TARGET", CMD_SAFETY_DANGEROUS, "Target values"},
    {0x45, "SETTING_FOR_USER_PROFILE", CMD_SAFETY_DANGEROUS, "User profile"},
    {0x47, "SERVICE_DISCOVERY", CMD_SAFETY_SAFE, "Service discovery manager"},
    {0xFF, "ERROR", CMD_SAFETY_BLOCKED, "Error frame from the watch"},
};

const CmdEntry* cmd_lookup(uint8_t id) {
    for(size_t i = 0; i < sizeof(catalog) / sizeof(catalog[0]); i++) {
        if(catalog[i].id == id) return &catalog[i];
    }
    return NULL;
}

uint32_t cmd_count(void) {
    return (uint32_t)(sizeof(catalog) / sizeof(catalog[0]));
}

const CasioObservedCommand* cmd_observed(uint8_t id) {
    for(uint32_t i = 0; i < CASIO_OBSERVED_COUNT; i++) {
        if(kObservedCommands[i].id == id) return &kObservedCommands[i];
    }
    return NULL;
}

uint32_t cmd_observed_count(void) {
    return CASIO_OBSERVED_COUNT;
}

const char* cmd_safety_name(CmdSafety safety) {
    switch(safety) {
    case CMD_SAFETY_SAFE:
        return "SAFE";
    case CMD_SAFETY_CAUTION:
        return "CAUTION";
    case CMD_SAFETY_DANGEROUS:
        return "DANGEROUS";
    case CMD_SAFETY_BLOCKED:
        return "BLOCKED";
    default:
        return "?";
    }
}
