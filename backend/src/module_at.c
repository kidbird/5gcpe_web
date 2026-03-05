#include "module_at.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <strings.h>

static const char *module_type_names[] = {
    [MODULE_TYPE_UNKNOWN] = "unknown",
    [MODULE_TYPE_QUECTEL_RG500U] = "quectel_rg500u",
    [MODULE_TYPE_QUECTEL_RM500U] = "quectel_rm500u",
    [MODULE_TYPE_QUECTEL_RG200U] = "quectel_rg200u",
    [MODULE_TYPE_QUECTEL_RM500Q] = "quectel_rm500q",
    [MODULE_TYPE_FIBOCOM_FM150] = "fibocom_fm150",
    [MODULE_TYPE_FIBOCOM_FM350] = "fibocom_fm350",
    [MODULE_TYPE_SIMCOM_SIM8200] = "simcom_sim8200",
    [MODULE_TYPE_ZTE_MG8250] = "zte_mg8250",
};

static const char *command_type_names[] = {
    [CMD_TEST] = "test",
    [CMD_GET_IMEI] = "get_imei",
    [CMD_GET_ICCID] = "get_iccid",
    [CMD_GET_IMSI] = "get_imsi",
    [CMD_GET_SIGNAL] = "get_signal",
    [CMD_GET_OPERATOR] = "get_operator",
    [CMD_GET_NETMODE] = "get_netmode",
    [CMD_SET_NETMODE_AUTO] = "set_netmode_auto",
    [CMD_SET_NETMODE_5G_SA] = "set_netmode_5g_sa",
    [CMD_SET_NETMODE_5G_NSA] = "set_netmode_5g_nsa",
    [CMD_SET_NETMODE_LTE] = "set_netmode_lte",
    [CMD_SET_APN] = "set_apn",
    [CMD_ACTIVATE_PDP] = "activate_pdp",
    [CMD_DEACTIVATE_PDP] = "deactivate_pdp",
    [CMD_GET_CELL_INFO] = "get_cell_info",
    [CMD_REBOOT] = "reboot",
    [CMD_FACTORY_RESET] = "factory_reset",
    [CMD_WLAN_ENABLE] = "wlan_enable",
    [CMD_WLAN_DISABLE] = "wlan_disable",
    [CMD_WLAN_SET_AP] = "wlan_set_ap",
    [CMD_WLAN_SET_STA] = "wlan_set_sta",
    [CMD_WLAN_SCAN] = "wlan_scan",
    [CMD_GET_TEMP] = "get_temp",
    [CMD_GET_VERSION] = "get_version",
    [CMD_GET_RSRP] = "get_rsrp",
    [CMD_GET_SINR] = "get_sinr",
    [CMD_SET_BAND] = "set_band",
    [CMD_GET_BAND] = "get_band",
};

#define CMD_ENTRY(cmd_str, has_param, prefix) \
    { .name = command_type_names[__COUNTER__ - 1], .at_cmd = cmd_str, .has_params = has_param, .response_prefix = prefix }

#define CMD_SIMPLE(cmd_str)         { .at_cmd = cmd_str, .has_params = false, .response_prefix = NULL }
#define CMD_PARAM(cmd_str, prefix)  { .at_cmd = cmd_str, .has_params = true, .response_prefix = prefix }

static ModuleAtTable module_tables[] = {
    [MODULE_TYPE_QUECTEL_RG500U] = {
        .type = MODULE_TYPE_QUECTEL_RG500U,
        .name = "RG500U-CN",
        .manufacturer = "Quectel",
        .supports_5g = true,
        .supports_wifi = true,
        .supports_gps = false,
        .commands = {
            [CMD_TEST]           = CMD_SIMPLE("AT"),
            [CMD_GET_IMEI]       = CMD_SIMPLE("AT+GSN"),
            [CMD_GET_ICCID]      = CMD_SIMPLE("AT+QCCID"),
            [CMD_GET_IMSI]       = CMD_SIMPLE("AT+CIMI"),
            [CMD_GET_SIGNAL]     = CMD_SIMPLE("AT+CSQ"),
            [CMD_GET_OPERATOR]   = CMD_SIMPLE("AT+COPS?"),
            [CMD_GET_NETMODE]    = CMD_SIMPLE("AT+CNMP?"),
            [CMD_SET_NETMODE_AUTO]    = CMD_SIMPLE("AT+CNMP=2"),
            [CMD_SET_NETMODE_5G_SA]   = CMD_SIMPLE("AT+CNMP=71"),
            [CMD_SET_NETMODE_5G_NSA]  = CMD_SIMPLE("AT+CNMP=70"),
            [CMD_SET_NETMODE_LTE]     = CMD_SIMPLE("AT+CNMP=61"),
            [CMD_SET_APN]        = CMD_PARAM("AT+CGDCONT=1,\"IP\",\"%s\"", NULL),
            [CMD_ACTIVATE_PDP]   = CMD_SIMPLE("AT+CGACT=1,1"),
            [CMD_DEACTIVATE_PDP] = CMD_SIMPLE("AT+CGACT=0,1"),
            [CMD_GET_CELL_INFO]  = CMD_SIMPLE("AT+QENG=\"servingcell\""),
            [CMD_REBOOT]         = CMD_SIMPLE("AT+CFUN=1,1"),
            [CMD_FACTORY_RESET]  = CMD_SIMPLE("AT&F"),
            [CMD_WLAN_ENABLE]    = CMD_SIMPLE("AT+QWIFI=1"),
            [CMD_WLAN_DISABLE]   = CMD_SIMPLE("AT+QWIFI=0"),
            [CMD_WLAN_SET_AP]    = CMD_PARAM("AT+QAPCONFIG=1,\"%s\",%d,4,\"%s\"", NULL),
            [CMD_WLAN_SET_STA]   = CMD_PARAM("AT+QWSTACONF=1,\"%s\",\"%s\"", NULL),
            [CMD_WLAN_SCAN]      = CMD_SIMPLE("AT+QSCAN"),
            [CMD_GET_TEMP]       = CMD_SIMPLE("AT+QTEMP"),
            [CMD_GET_VERSION]    = CMD_SIMPLE("AT+CGMR"),
            [CMD_GET_RSRP]       = CMD_SIMPLE("AT+QRSRP"),
            [CMD_GET_SINR]       = CMD_SIMPLE("AT+QSINR"),
            [CMD_SET_BAND]       = CMD_PARAM("AT+QCFG=\"band\",%s", NULL),
            [CMD_GET_BAND]       = CMD_SIMPLE("AT+QCFG=\"band\""),
        }
    },
    [MODULE_TYPE_QUECTEL_RM500U] = {
        .type = MODULE_TYPE_QUECTEL_RM500U,
        .name = "RM500U-CN",
        .manufacturer = "Quectel",
        .supports_5g = true,
        .supports_wifi = false,
        .supports_gps = true,
        .commands = {
            [CMD_TEST]           = CMD_SIMPLE("AT"),
            [CMD_GET_IMEI]       = CMD_SIMPLE("AT+GSN"),
            [CMD_GET_ICCID]      = CMD_SIMPLE("AT+QCCID"),
            [CMD_GET_IMSI]       = CMD_SIMPLE("AT+CIMI"),
            [CMD_GET_SIGNAL]     = CMD_SIMPLE("AT+CSQ"),
            [CMD_GET_OPERATOR]   = CMD_SIMPLE("AT+COPS?"),
            [CMD_GET_NETMODE]    = CMD_SIMPLE("AT+CNMP?"),
            [CMD_SET_NETMODE_AUTO]    = CMD_SIMPLE("AT+CNMP=2"),
            [CMD_SET_NETMODE_5G_SA]   = CMD_SIMPLE("AT+CNMP=71"),
            [CMD_SET_NETMODE_5G_NSA]  = CMD_SIMPLE("AT+CNMP=70"),
            [CMD_SET_NETMODE_LTE]     = CMD_SIMPLE("AT+CNMP=61"),
            [CMD_SET_APN]        = CMD_PARAM("AT+CGDCONT=1,\"IP\",\"%s\"", NULL),
            [CMD_ACTIVATE_PDP]   = CMD_SIMPLE("AT+CGACT=1,1"),
            [CMD_DEACTIVATE_PDP] = CMD_SIMPLE("AT+CGACT=0,1"),
            [CMD_GET_CELL_INFO]  = CMD_SIMPLE("AT+QENG=\"servingcell\""),
            [CMD_REBOOT]         = CMD_SIMPLE("AT+CFUN=1,1"),
            [CMD_FACTORY_RESET]  = CMD_SIMPLE("AT&F"),
            [CMD_WLAN_ENABLE]    = { .at_cmd = NULL },
            [CMD_WLAN_DISABLE]   = { .at_cmd = NULL },
            [CMD_WLAN_SET_AP]    = { .at_cmd = NULL },
            [CMD_WLAN_SET_STA]   = { .at_cmd = NULL },
            [CMD_WLAN_SCAN]      = { .at_cmd = NULL },
            [CMD_GET_TEMP]       = CMD_SIMPLE("AT+QTEMP"),
            [CMD_GET_VERSION]    = CMD_SIMPLE("AT+CGMR"),
            [CMD_GET_RSRP]       = CMD_SIMPLE("AT+QRSRP"),
            [CMD_GET_SINR]       = CMD_SIMPLE("AT+QSINR"),
            [CMD_SET_BAND]       = CMD_PARAM("AT+QCFG=\"band\",%s", NULL),
            [CMD_GET_BAND]       = CMD_SIMPLE("AT+QCFG=\"band\""),
        }
    },
    [MODULE_TYPE_QUECTEL_RG200U] = {
        .type = MODULE_TYPE_QUECTEL_RG200U,
        .name = "RG200U-CN",
        .manufacturer = "Quectel",
        .supports_5g = true,
        .supports_wifi = true,
        .supports_gps = false,
        .commands = {
            [CMD_TEST]           = CMD_SIMPLE("AT"),
            [CMD_GET_IMEI]       = CMD_SIMPLE("AT+GSN"),
            [CMD_GET_ICCID]      = CMD_SIMPLE("AT+QCCID"),
            [CMD_GET_IMSI]       = CMD_SIMPLE("AT+CIMI"),
            [CMD_GET_SIGNAL]     = CMD_SIMPLE("AT+CSQ"),
            [CMD_GET_OPERATOR]   = CMD_SIMPLE("AT+COPS?"),
            [CMD_GET_NETMODE]    = CMD_SIMPLE("AT+CNMP?"),
            [CMD_SET_NETMODE_AUTO]    = CMD_SIMPLE("AT+CNMP=2"),
            [CMD_SET_NETMODE_5G_SA]   = CMD_SIMPLE("AT+CNMP=71"),
            [CMD_SET_NETMODE_5G_NSA]  = CMD_SIMPLE("AT+CNMP=70"),
            [CMD_SET_NETMODE_LTE]     = CMD_SIMPLE("AT+CNMP=61"),
            [CMD_SET_APN]        = CMD_PARAM("AT+CGDCONT=1,\"IP\",\"%s\"", NULL),
            [CMD_ACTIVATE_PDP]   = CMD_SIMPLE("AT+CGACT=1,1"),
            [CMD_DEACTIVATE_PDP] = CMD_SIMPLE("AT+CGACT=0,1"),
            [CMD_GET_CELL_INFO]  = CMD_SIMPLE("AT+QENG=\"servingcell\""),
            [CMD_REBOOT]         = CMD_SIMPLE("AT+CFUN=1,1"),
            [CMD_FACTORY_RESET]  = CMD_SIMPLE("AT&F"),
            [CMD_WLAN_ENABLE]    = CMD_SIMPLE("AT+QWIFI=1"),
            [CMD_WLAN_DISABLE]   = CMD_SIMPLE("AT+QWIFI=0"),
            [CMD_WLAN_SET_AP]    = CMD_PARAM("AT+QAPCONFIG=1,\"%s\",%d,4,\"%s\"", NULL),
            [CMD_WLAN_SET_STA]   = CMD_PARAM("AT+QWSTACONF=1,\"%s\",\"%s\"", NULL),
            [CMD_WLAN_SCAN]      = CMD_SIMPLE("AT+QSCAN"),
            [CMD_GET_TEMP]       = CMD_SIMPLE("AT+QTEMP"),
            [CMD_GET_VERSION]    = CMD_SIMPLE("AT+CGMR"),
            [CMD_GET_RSRP]       = CMD_SIMPLE("AT+QRSRP"),
            [CMD_GET_SINR]       = CMD_SIMPLE("AT+QSINR"),
            [CMD_SET_BAND]       = CMD_PARAM("AT+QCFG=\"band\",%s", NULL),
            [CMD_GET_BAND]       = CMD_SIMPLE("AT+QCFG=\"band\""),
        }
    },
    [MODULE_TYPE_QUECTEL_RM500Q] = {
        .type = MODULE_TYPE_QUECTEL_RM500Q,
        .name = "RM500Q-CN",
        .manufacturer = "Quectel",
        .supports_5g = true,
        .supports_wifi = false,
        .supports_gps = true,
        .commands = {
            [CMD_TEST]           = CMD_SIMPLE("AT"),
            [CMD_GET_IMEI]       = CMD_SIMPLE("AT+GSN"),
            [CMD_GET_ICCID]      = CMD_SIMPLE("AT+QCCID"),
            [CMD_GET_IMSI]       = CMD_SIMPLE("AT+CIMI"),
            [CMD_GET_SIGNAL]     = CMD_SIMPLE("AT+CSQ"),
            [CMD_GET_OPERATOR]   = CMD_SIMPLE("AT+COPS?"),
            [CMD_GET_NETMODE]    = CMD_SIMPLE("AT+CNMP?"),
            [CMD_SET_NETMODE_AUTO]    = CMD_SIMPLE("AT+CNMP=2"),
            [CMD_SET_NETMODE_5G_SA]   = CMD_SIMPLE("AT+CNMP=71"),
            [CMD_SET_NETMODE_5G_NSA]  = CMD_SIMPLE("AT+CNMP=70"),
            [CMD_SET_NETMODE_LTE]     = CMD_SIMPLE("AT+CNMP=61"),
            [CMD_SET_APN]        = CMD_PARAM("AT+CGDCONT=1,\"IP\",\"%s\"", NULL),
            [CMD_ACTIVATE_PDP]   = CMD_SIMPLE("AT+CGACT=1,1"),
            [CMD_DEACTIVATE_PDP] = CMD_SIMPLE("AT+CGACT=0,1"),
            [CMD_GET_CELL_INFO]  = CMD_SIMPLE("AT+QENG=\"servingcell\""),
            [CMD_REBOOT]         = CMD_SIMPLE("AT+CFUN=1,1"),
            [CMD_FACTORY_RESET]  = CMD_SIMPLE("AT&F"),
            [CMD_WLAN_ENABLE]    = { .at_cmd = NULL },
            [CMD_WLAN_DISABLE]   = { .at_cmd = NULL },
            [CMD_WLAN_SET_AP]    = { .at_cmd = NULL },
            [CMD_WLAN_SET_STA]   = { .at_cmd = NULL },
            [CMD_WLAN_SCAN]      = { .at_cmd = NULL },
            [CMD_GET_TEMP]       = CMD_SIMPLE("AT+QTEMP"),
            [CMD_GET_VERSION]    = CMD_SIMPLE("AT+CGMR"),
            [CMD_GET_RSRP]       = CMD_SIMPLE("AT+QRSRP"),
            [CMD_GET_SINR]       = CMD_SIMPLE("AT+QSINR"),
            [CMD_SET_BAND]       = CMD_PARAM("AT+QCFG=\"band\",%s", NULL),
            [CMD_GET_BAND]       = CMD_SIMPLE("AT+QCFG=\"band\""),
        }
    },
    [MODULE_TYPE_FIBOCOM_FM150] = {
        .type = MODULE_TYPE_FIBOCOM_FM150,
        .name = "FM150",
        .manufacturer = "Fibocom",
        .supports_5g = true,
        .supports_wifi = false,
        .supports_gps = false,
        .commands = {
            [CMD_TEST]           = CMD_SIMPLE("AT"),
            [CMD_GET_IMEI]       = CMD_SIMPLE("AT+CGSN"),
            [CMD_GET_ICCID]      = CMD_SIMPLE("AT+CCID"),
            [CMD_GET_IMSI]       = CMD_SIMPLE("AT+CIMI"),
            [CMD_GET_SIGNAL]     = CMD_SIMPLE("AT+CSQ"),
            [CMD_GET_OPERATOR]   = CMD_SIMPLE("AT+COPS?"),
            [CMD_GET_NETMODE]    = CMD_SIMPLE("AT+CNMP?"),
            [CMD_SET_NETMODE_AUTO]    = CMD_SIMPLE("AT+CNMP=2"),
            [CMD_SET_NETMODE_5G_SA]   = CMD_SIMPLE("AT+CNMP=71"),
            [CMD_SET_NETMODE_5G_NSA]  = CMD_SIMPLE("AT+CNMP=70"),
            [CMD_SET_NETMODE_LTE]     = CMD_SIMPLE("AT+CNMP=61"),
            [CMD_SET_APN]        = CMD_PARAM("AT+CGDCONT=1,\"IP\",\"%s\"", NULL),
            [CMD_ACTIVATE_PDP]   = CMD_SIMPLE("AT+CGACT=1,1"),
            [CMD_DEACTIVATE_PDP] = CMD_SIMPLE("AT+CGACT=0,1"),
            [CMD_GET_CELL_INFO]  = CMD_SIMPLE("AT+CEREG?"),
            [CMD_REBOOT]         = CMD_SIMPLE("AT+CFUN=1,1"),
            [CMD_FACTORY_RESET]  = CMD_SIMPLE("AT&F"),
            [CMD_WLAN_ENABLE]    = { .at_cmd = NULL },
            [CMD_WLAN_DISABLE]   = { .at_cmd = NULL },
            [CMD_WLAN_SET_AP]    = { .at_cmd = NULL },
            [CMD_WLAN_SET_STA]   = { .at_cmd = NULL },
            [CMD_WLAN_SCAN]      = { .at_cmd = NULL },
            [CMD_GET_TEMP]       = CMD_SIMPLE("AT+MTSM=1,1"),
            [CMD_GET_VERSION]    = CMD_SIMPLE("AT+CGMR"),
            [CMD_GET_RSRP]       = CMD_SIMPLE("AT+CESQ"),
            [CMD_GET_SINR]       = { .at_cmd = NULL },
            [CMD_SET_BAND]       = CMD_PARAM("AT+CBAND=%s", NULL),
            [CMD_GET_BAND]       = CMD_SIMPLE("AT+CBAND?"),
        }
    },
    [MODULE_TYPE_FIBOCOM_FM350] = {
        .type = MODULE_TYPE_FIBOCOM_FM350,
        .name = "FM350",
        .manufacturer = "Fibocom",
        .supports_5g = true,
        .supports_wifi = false,
        .supports_gps = false,
        .commands = {
            [CMD_TEST]           = CMD_SIMPLE("AT"),
            [CMD_GET_IMEI]       = CMD_SIMPLE("AT+CGSN"),
            [CMD_GET_ICCID]      = CMD_SIMPLE("AT+CCID"),
            [CMD_GET_IMSI]       = CMD_SIMPLE("AT+CIMI"),
            [CMD_GET_SIGNAL]     = CMD_SIMPLE("AT+CSQ"),
            [CMD_GET_OPERATOR]   = CMD_SIMPLE("AT+COPS?"),
            [CMD_GET_NETMODE]    = CMD_SIMPLE("AT+CNMP?"),
            [CMD_SET_NETMODE_AUTO]    = CMD_SIMPLE("AT+CNMP=2"),
            [CMD_SET_NETMODE_5G_SA]   = CMD_SIMPLE("AT+CNMP=71"),
            [CMD_SET_NETMODE_5G_NSA]  = CMD_SIMPLE("AT+CNMP=70"),
            [CMD_SET_NETMODE_LTE]     = CMD_SIMPLE("AT+CNMP=61"),
            [CMD_SET_APN]        = CMD_PARAM("AT+CGDCONT=1,\"IP\",\"%s\"", NULL),
            [CMD_ACTIVATE_PDP]   = CMD_SIMPLE("AT+CGACT=1,1"),
            [CMD_DEACTIVATE_PDP] = CMD_SIMPLE("AT+CGACT=0,1"),
            [CMD_GET_CELL_INFO]  = CMD_SIMPLE("AT+CEREG?"),
            [CMD_REBOOT]         = CMD_SIMPLE("AT+CFUN=1,1"),
            [CMD_FACTORY_RESET]  = CMD_SIMPLE("AT&F"),
            [CMD_WLAN_ENABLE]    = { .at_cmd = NULL },
            [CMD_WLAN_DISABLE]   = { .at_cmd = NULL },
            [CMD_WLAN_SET_AP]    = { .at_cmd = NULL },
            [CMD_WLAN_SET_STA]   = { .at_cmd = NULL },
            [CMD_WLAN_SCAN]      = { .at_cmd = NULL },
            [CMD_GET_TEMP]       = CMD_SIMPLE("AT+MTSM=1,1"),
            [CMD_GET_VERSION]    = CMD_SIMPLE("AT+CGMR"),
            [CMD_GET_RSRP]       = CMD_SIMPLE("AT+CESQ"),
            [CMD_GET_SINR]       = { .at_cmd = NULL },
            [CMD_SET_BAND]       = CMD_PARAM("AT+CBAND=%s", NULL),
            [CMD_GET_BAND]       = CMD_SIMPLE("AT+CBAND?"),
        }
    },
    [MODULE_TYPE_SIMCOM_SIM8200] = {
        .type = MODULE_TYPE_SIMCOM_SIM8200,
        .name = "SIM8200",
        .manufacturer = "SIMCom",
        .supports_5g = true,
        .supports_wifi = false,
        .supports_gps = true,
        .commands = {
            [CMD_TEST]           = CMD_SIMPLE("AT"),
            [CMD_GET_IMEI]       = CMD_SIMPLE("AT+CGSN"),
            [CMD_GET_ICCID]      = CMD_SIMPLE("AT+CIMI"),
            [CMD_GET_IMSI]       = CMD_SIMPLE("AT+CIMI"),
            [CMD_GET_SIGNAL]     = CMD_SIMPLE("AT+CSQ"),
            [CMD_GET_OPERATOR]   = CMD_SIMPLE("AT+COPS?"),
            [CMD_GET_NETMODE]    = CMD_SIMPLE("AT+CNMP?"),
            [CMD_SET_NETMODE_AUTO]    = CMD_SIMPLE("AT+CNMP=2"),
            [CMD_SET_NETMODE_5G_SA]   = CMD_SIMPLE("AT+CNMP=71"),
            [CMD_SET_NETMODE_5G_NSA]  = CMD_SIMPLE("AT+CNMP=70"),
            [CMD_SET_NETMODE_LTE]     = CMD_SIMPLE("AT+CNMP=61"),
            [CMD_SET_APN]        = CMD_PARAM("AT+CGDCONT=1,\"IP\",\"%s\"", NULL),
            [CMD_ACTIVATE_PDP]   = CMD_SIMPLE("AT+CGACT=1,1"),
            [CMD_DEACTIVATE_PDP] = CMD_SIMPLE("AT+CGACT=0,1"),
            [CMD_GET_CELL_INFO]  = CMD_SIMPLE("AT+CEREG?"),
            [CMD_REBOOT]         = CMD_SIMPLE("AT+CFUN=1,1"),
            [CMD_FACTORY_RESET]  = CMD_SIMPLE("AT&F"),
            [CMD_WLAN_ENABLE]    = { .at_cmd = NULL },
            [CMD_WLAN_DISABLE]   = { .at_cmd = NULL },
            [CMD_WLAN_SET_AP]    = { .at_cmd = NULL },
            [CMD_WLAN_SET_STA]   = { .at_cmd = NULL },
            [CMD_WLAN_SCAN]      = { .at_cmd = NULL },
            [CMD_GET_TEMP]       = CMD_SIMPLE("AT+CPMUTEMP?"),
            [CMD_GET_VERSION]    = CMD_SIMPLE("AT+CGMR"),
            [CMD_GET_RSRP]       = CMD_SIMPLE("AT+CESQ"),
            [CMD_GET_SINR]       = { .at_cmd = NULL },
            [CMD_SET_BAND]       = CMD_PARAM("AT+CBAND=%s", NULL),
            [CMD_GET_BAND]       = CMD_SIMPLE("AT+CBAND?"),
        }
    },
    [MODULE_TYPE_ZTE_MG8250] = {
        .type = MODULE_TYPE_ZTE_MG8250,
        .name = "MG8250",
        .manufacturer = "ZTE",
        .supports_5g = true,
        .supports_wifi = false,
        .supports_gps = false,
        .commands = {
            [CMD_TEST]           = CMD_SIMPLE("AT"),
            [CMD_GET_IMEI]       = CMD_SIMPLE("AT+CGSN"),
            [CMD_GET_ICCID]      = CMD_SIMPLE("AT+CIMI"),
            [CMD_GET_IMSI]       = CMD_SIMPLE("AT+CIMI"),
            [CMD_GET_SIGNAL]     = CMD_SIMPLE("AT+CSQ"),
            [CMD_GET_OPERATOR]   = CMD_SIMPLE("AT+COPS?"),
            [CMD_GET_NETMODE]    = CMD_SIMPLE("AT+ZSNT?"),
            [CMD_SET_NETMODE_AUTO]    = CMD_SIMPLE("AT+ZSNT=0,0,0"),
            [CMD_SET_NETMODE_5G_SA]   = CMD_SIMPLE("AT+ZSNT=1,0,2"),
            [CMD_SET_NETMODE_5G_NSA]  = CMD_SIMPLE("AT+ZSNT=1,0,1"),
            [CMD_SET_NETMODE_LTE]     = CMD_SIMPLE("AT+ZSNT=0,0,1"),
            [CMD_SET_APN]        = CMD_PARAM("AT+CGDCONT=1,\"IP\",\"%s\"", NULL),
            [CMD_ACTIVATE_PDP]   = CMD_SIMPLE("AT+CGACT=1,1"),
            [CMD_DEACTIVATE_PDP] = CMD_SIMPLE("AT+CGACT=0,1"),
            [CMD_GET_CELL_INFO]  = CMD_SIMPLE("AT+ZSRSRP?"),
            [CMD_REBOOT]         = CMD_SIMPLE("AT+CFUN=1,1"),
            [CMD_FACTORY_RESET]  = CMD_SIMPLE("AT&F"),
            [CMD_WLAN_ENABLE]    = { .at_cmd = NULL },
            [CMD_WLAN_DISABLE]   = { .at_cmd = NULL },
            [CMD_WLAN_SET_AP]    = { .at_cmd = NULL },
            [CMD_WLAN_SET_STA]   = { .at_cmd = NULL },
            [CMD_WLAN_SCAN]      = { .at_cmd = NULL },
            [CMD_GET_TEMP]       = CMD_SIMPLE("AT+ZTST?"),
            [CMD_GET_VERSION]    = CMD_SIMPLE("AT+CGMR"),
            [CMD_GET_RSRP]       = CMD_SIMPLE("AT+ZSRSRP?"),
            [CMD_GET_SINR]       = CMD_SIMPLE("AT+ZSSINR?"),
            [CMD_SET_BAND]       = CMD_PARAM("AT+ZBAND=%s", NULL),
            [CMD_GET_BAND]       = CMD_SIMPLE("AT+ZBAND?"),
        }
    },
};

const char* module_type_to_string(ModuleType type)
{
    if (type >= 0 && type < MODULE_TYPE_MAX) {
        return module_type_names[type];
    }
    return "unknown";
}

ModuleType module_type_from_string(const char *str)
{
    if (!str) return MODULE_TYPE_UNKNOWN;
    
    for (int i = 0; i < MODULE_TYPE_MAX; i++) {
        if (strcasecmp(str, module_type_names[i]) == 0) {
            return (ModuleType)i;
        }
    }
    
    if (strcasestr(str, "RG500U")) return MODULE_TYPE_QUECTEL_RG500U;
    if (strcasestr(str, "RM500U")) return MODULE_TYPE_QUECTEL_RM500U;
    if (strcasestr(str, "RG200U")) return MODULE_TYPE_QUECTEL_RG200U;
    if (strcasestr(str, "RM500Q")) return MODULE_TYPE_QUECTEL_RM500Q;
    if (strcasestr(str, "FM150")) return MODULE_TYPE_FIBOCOM_FM150;
    if (strcasestr(str, "FM350")) return MODULE_TYPE_FIBOCOM_FM350;
    if (strcasestr(str, "SIM8200")) return MODULE_TYPE_SIMCOM_SIM8200;
    if (strcasestr(str, "MG8250")) return MODULE_TYPE_ZTE_MG8250;
    
    return MODULE_TYPE_UNKNOWN;
}

const char* module_get_name(ModuleType type)
{
    if (type > MODULE_TYPE_UNKNOWN && type < MODULE_TYPE_MAX) {
        return module_tables[type].name;
    }
    return "Unknown";
}

const char* module_get_at_command(ModuleType type, CommandType cmd)
{
    if (type <= MODULE_TYPE_UNKNOWN || type >= MODULE_TYPE_MAX) {
        type = MODULE_TYPE_QUECTEL_RG500U;
    }
    if (cmd >= 0 && cmd < CMD_MAX) {
        return module_tables[type].commands[cmd].at_cmd;
    }
    return NULL;
}

const AtCommandEntry* module_get_command_entry(ModuleType type, CommandType cmd)
{
    if (type <= MODULE_TYPE_UNKNOWN || type >= MODULE_TYPE_MAX) {
        type = MODULE_TYPE_QUECTEL_RG500U;
    }
    if (cmd >= 0 && cmd < CMD_MAX) {
        return &module_tables[type].commands[cmd];
    }
    return NULL;
}

int module_build_command(ModuleType type, CommandType cmd, char *buf, int len, ...)
{
    const char *at_cmd = module_get_at_command(type, cmd);
    if (!at_cmd || !buf) return -1;
    
    AtCommandEntry *entry = (AtCommandEntry*)&module_tables[type].commands[cmd];
    
    if (entry->has_params) {
        va_list args;
        va_start(args, len);
        vsnprintf(buf, len, at_cmd, args);
        va_end(args);
    } else {
        strncpy(buf, at_cmd, len - 1);
        buf[len - 1] = '\0';
    }
    
    return 0;
}

bool module_supports_5g(ModuleType type)
{
    if (type > MODULE_TYPE_UNKNOWN && type < MODULE_TYPE_MAX) {
        return module_tables[type].supports_5g;
    }
    return false;
}

bool module_supports_wifi(ModuleType type)
{
    if (type > MODULE_TYPE_UNKNOWN && type < MODULE_TYPE_MAX) {
        return module_tables[type].supports_wifi;
    }
    return false;
}

bool module_supports_gps(ModuleType type)
{
    if (type > MODULE_TYPE_UNKNOWN && type < MODULE_TYPE_MAX) {
        return module_tables[type].supports_gps;
    }
    return false;
}

const char* command_type_to_string(CommandType cmd)
{
    if (cmd >= 0 && cmd < CMD_MAX) {
        return command_type_names[cmd];
    }
    return "unknown";
}
