#ifndef MODULE_AT_H
#define MODULE_AT_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    MODULE_TYPE_UNKNOWN = 0,
    MODULE_TYPE_QUECTEL_RG500U_SERIES,
    MODULE_TYPE_QUECTEL_RM500Q,
    MODULE_TYPE_FIBOCOM_FM150,
    MODULE_TYPE_FIBOCOM_FM350,
    MODULE_TYPE_SIMCOM_SIM8200,
    MODULE_TYPE_ZTE_MG8250,
    MODULE_TYPE_MAX
} ModuleType;

typedef enum {
    CMD_TEST = 0,
    CMD_GET_IMEI,
    CMD_GET_ICCID,
    CMD_GET_IMSI,
    CMD_GET_SIGNAL,
    CMD_GET_OPERATOR,
    CMD_GET_NETMODE,
    CMD_SET_NETMODE_AUTO,
    CMD_SET_NETMODE_5G_SA,
    CMD_SET_NETMODE_5G_NSA,
    CMD_SET_NETMODE_LTE,
    CMD_SET_APN,
    CMD_ACTIVATE_PDP,
    CMD_DEACTIVATE_PDP,
    CMD_GET_CELL_INFO,
    CMD_REBOOT,
    CMD_FACTORY_RESET,
    CMD_WLAN_ENABLE,
    CMD_WLAN_DISABLE,
    CMD_WLAN_SET_AP,
    CMD_WLAN_SET_STA,
    CMD_WLAN_SCAN,
    CMD_GET_TEMP,
    CMD_GET_VERSION,
    CMD_GET_RSRP,
    CMD_GET_SINR,
    CMD_SET_BAND,
    CMD_GET_BAND,
    CMD_MAX
} CommandType;

typedef struct {
    const char *name;
    const char *at_cmd;
    bool has_params;
    const char *response_prefix;
} AtCommandEntry;

typedef struct {
    ModuleType type;
    const char *name;
    const char *manufacturer;
    AtCommandEntry commands[CMD_MAX];
    bool supports_5g;
    bool supports_wifi;
    bool supports_gps;
} ModuleAtTable;

const char* module_type_to_string(ModuleType type);
ModuleType module_type_from_string(const char *str);
const char* module_get_name(ModuleType type);
const char* module_get_at_command(ModuleType type, CommandType cmd);
const AtCommandEntry* module_get_command_entry(ModuleType type, CommandType cmd);
int module_build_command(ModuleType type, CommandType cmd, char *buf, int len, ...);
bool module_supports_5g(ModuleType type);
bool module_supports_wifi(ModuleType type);
bool module_supports_gps(ModuleType type);

const char* command_type_to_string(CommandType cmd);

#endif
