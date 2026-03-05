#ifndef CPE_CONTROL_H
#define CPE_CONTROL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#define MAX_BUFFER_SIZE     4096
#define MAX_APN_COUNT      10
#define AT_COMMAND_TIMEOUT  3000

typedef enum {
    CPE_CTRL_MODE_AT,        
    CPE_CTRL_MODE_API,        
    CPE_CTRL_MODE_MAX
} CpeControlMode;

typedef enum {
    APN_AUTH_NONE = 0,
    APN_AUTH_PAP,
    APN_AUTH_CHAP
} ApnAuthType;

typedef enum {
    BEARER_IP = 0,
    BEARER_IPV4V6,
    BEARER_ETHERNET
} BearerType;

typedef enum {
    NETWORK_AUTO = 0,
    NETWORK_5G_SA,
    NETWORK_5G_SA_NSA,
    NETWORK_LTE
} NetworkMode;

typedef enum {
    CTRL_OK = 0,
    CTRL_ERROR = -1,
    CTRL_TIMEOUT = -2,
    CTRL_NO_DEVICE = -3,
    CTRL_INVALID_PARAM = -4
} CpeCtrlResult;

typedef struct {
    int     id;
    char    name[64];
    char    username[64];
    char    password[64];
    ApnAuthType auth_type;
    BearerType bearer_type;
    bool    is_default;
    bool    is_active;
} ApnConfig;

typedef struct {
    char    ip[16];
    char    netmask[16];
    bool    dhcp_enabled;
    char    dhcp_start[16];
    char    dhcp_end[16];
    int     dhcp_lease;
    char    dns1[32];
    char    dns2[32];
} LanConfig;

typedef struct {
    bool    enabled_2g4;
    bool    enabled_5g;
    char    ssid_2g4[64];
    char    password_2g4[64];
    char    ssid_5g[64];
    char    password_5g[64];
    int     channel_2g4;
    int     channel_5g;
    int     encryption;
} WlanApConfig;

typedef struct {
    bool    sta_enabled;
    char    target_ssid[64];
    char    target_password[64];
    int     security_type;
    int     band;
    int     wan_type;
    char    wan_mac[18];
    bool    nat_enabled;
} WlanStaConfig;

typedef struct {
    bool    enabled;
    bool    spi_enabled;
    bool    dos_enabled;
} FirewallConfig;

typedef struct {
    char    device_model[64];
    char    firmware_version[32];
    char    imei[20];
    char    iccid[24];
    char    lan_mac[18];
    char    wan_ip[16];
    char    uptime[64];
    int     signal_strength;
    char    network_type[16];
    char    operator_name[32];
    int     rsrp;
    int     sinr;
} DeviceStatus;

typedef struct {
    char    tx_bytes[32];
    char    rx_bytes[32];
    int     tx_rate;
    int     rx_rate;
} TrafficStats;

typedef struct {
    char    ssid[64];
    char    bssid[18];
    int     signal;
    int     channel;
    char    security[32];
} WifiScanResult;

typedef struct CpeControlOps {
    CpeCtrlResult (*init)(const char *device);
    void (*close)(void);

    CpeCtrlResult (*get_signal_strength)(int *rssi);
    CpeCtrlResult (*get_operator)(char *operator_name, int len);
    CpeCtrlResult (*get_imei)(char *imei, int len);
    CpeCtrlResult (*get_iccid)(char *iccid, int len);
    CpeCtrlResult (*get_imsi)(char *imsi, int len);
    CpeCtrlResult (*get_network_mode)(int *mode);
    CpeCtrlResult (*set_network_mode)(int mode);
    CpeCtrlResult (*get_network_registration)(int *reg_status);

    CpeCtrlResult (*set_apn)(const char *apn, const char *username, const char *password, int auth_type);
    CpeCtrlResult (*activate_pdp)(int cid);
    CpeCtrlResult (*deactivate_pdp)(int cid);
    CpeCtrlResult (*get_apn_info)(char *apn, int len);

    CpeCtrlResult (*get_device_status)(DeviceStatus *status);
    CpeCtrlResult (*get_traffic_stats)(TrafficStats *stats);

    CpeCtrlResult (*wlan_init)(void);
    CpeCtrlResult (*wlan_deinit)(void);
    CpeCtrlResult (*wlan_set_ap)(const char *ssid, const char *password, int channel, int encryption);
    CpeCtrlResult (*wlan_set_sta)(const char *ssid, const char *password);
    CpeCtrlResult (*wlan_sta_connect)(void);
    CpeCtrlResult (*wlan_sta_disconnect)(void);
    CpeCtrlResult (*wlan_scan)(WifiScanResult *results, int *count);

    CpeCtrlResult (*reboot)(void);
    CpeCtrlResult (*factory_reset)(void);
    CpeCtrlResult (*set_airplane_mode)(int enable);
    CpeCtrlResult (*get_temperature)(int *temp);

    void *context;
} CpeControlOps;

typedef struct {
    CpeControlMode mode;
    int            module_type;
    char           at_device[128];
    char           api_socket[128];
    int            api_port;
    bool           debug_enabled;
} CpeConfig;

CpeControlOps* cpe_control_create(CpeControlMode mode);
void cpe_control_destroy(CpeControlOps *ops);
CpeCtrlResult cpe_control_init(CpeControlOps *ops, const char *config_file);
const char* cpe_control_get_mode_name(CpeControlMode mode);
CpeControlMode cpe_control_get_mode_from_string(const char *mode_str);

int cpe_config_load(const char *config_file, CpeConfig *config);
int cpe_config_save(const char *config_file, const CpeConfig *config);
int config_save(const char *config_file, const CpeConfig *config);

#endif
