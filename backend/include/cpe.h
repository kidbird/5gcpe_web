#ifndef CPE_H
#define CPE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/select.h>

#define MAX_BUFFER_SIZE     4096
#define MAX_APN_COUNT       10
#define MAX_PORT_FORWARD    32
#define MAX_VPN_COUNT       4
#define MAX_IOT_DEVICES     32
#define AT_COMMAND_TIMEOUT  3000

#define CGI_OK              200
#define CGI_ERROR           500
#define CGI_NOT_FOUND       404
#define CGI_BAD_REQUEST     400
#define CGI_UNAUTHORIZED    401

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
    ENCRYPTION_NONE = 0,
    ENCRYPTION_WEP = 1,
    ENCRYPTION_WPA = 3,
    ENCRYPTION_WPA2 = 4,
    ENCRYPTION_WPA3 = 6
} EncryptionType;

typedef enum {
    VPN_TYPE_PPTP = 0,
    VPN_TYPE_L2TP,
    VPN_TYPE_GRE,
    VPN_TYPE_EOIP,
    VPN_TYPE_IPSEC
} VpnType;

typedef enum {
    IOT_PROTOCOL_MQTT = 0,
    IOT_PROTOCOL_COAP,
    IOT_PROTOCOL_HTTP
} IotProtocol;

typedef struct {
    int     id;
    char    name[64];
    char    username[64];
    char    password[64];
    ApnAuthType auth_type;
    BearerType bearer_type;
    bool    is_default;
    bool    is_active;
    char    apn_types[64];
    char    mcc[4];
    char    mnc[4];
} ApnConfig;

typedef struct {
    char    work_mode[16];
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
    int     network_mode;
    char    bands_5g[256];
    char    bands_lte[256];
    bool    airplane_mode;
    bool    data_roaming;
    bool    hw_accel;
    bool    ims_enabled;
    char    preferred_plmn[8];
} CellularConfig;

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
    bool    hidden_2g4;
    bool    hidden_5g;
    int     max_clients;
    int     bandwidth_2g4;
    int     bandwidth_5g;
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
    int     id;
    char    protocol[8];
    int     external_port;
    char    internal_ip[16];
    int     internal_port;
    char    description[64];
    bool    enabled;
} PortForwardRule;

typedef struct {
    bool    enabled;
    bool    dmz_enabled;
    char    dmz_ip[16];
    bool    spi_enabled;
    bool    dos_enabled;
    bool    arp_proxy;
    bool    ping_wan;
    PortForwardRule port_forwards[MAX_PORT_FORWARD];
    int     port_forward_count;
} FirewallConfig;

typedef struct {
    int     type;
    bool    enabled;
    char    server[128];
    char    username[64];
    char    password[64];
    bool    mppe;
    char    local_ip[16];
    char    remote_ip[16];
    int     tunnel_id;
    char    tunnel_key[64];
    int     mtu;
    bool    keepalive;
    int     keepalive_interval;
    char    local_mac[18];
    bool    arp_proxy;
    char    remote_gateway[64];
    char    preshared_key[64];
    char    local_id[64];
    char    remote_id[64];
    char    ike_encryption[16];
    char    ike_authentication[16];
    int     ike_dh_group;
    int     ike_lifetime;
    char    esp_encryption[16];
    char    esp_authentication[16];
    int     esp_lifetime;
} VpnConfig;

typedef struct {
    char    id[32];
    char    name[64];
    char    type[32];
    char    status[16];
    time_t  last_seen;
} IotDevice;

typedef struct {
    bool    enabled;
    int     protocol;
    char    server[128];
    int     port;
    char    client_id[64];
    char    username[64];
    char    password[64];
    int     keepalive;
    int     qos;
    bool    clean_session;
    char    publish_topic[128];
    char    subscribe_topic[128];
    IotDevice devices[MAX_IOT_DEVICES];
    int     device_count;
} IotConfig;

typedef struct {
    char    device_name[64];
    char    timezone[64];
    char    ntp_server[128];
    int     web_port;
    char    admin_user[32];
    char    admin_pass[128];
    bool    https_enabled;
    bool    remote_manage;
    char    language[16];
} SystemConfig;

typedef struct {
    char    device_model[64];
    char    firmware_version[32];
    char    imei[20];
    char    iccid[24];
    char    imsi[20];
    char    lan_mac[18];
    char    wan_ip[16];
    char    wan_ipv6[64];
    char    uptime[64];
    int     uptime_seconds;
    int     signal_strength;
    char    network_type[16];
    char    network_mode_str[16];
    char    operator_name[32];
    char    mcc[4];
    char    mnc[4];
    int     rsrp;
    int     rsrq;
    int     sinr;
    char    band[16];
    int     earfcn;
    int     pci;
    char    cell_id[16];
    int     cqi;
    int     temperature;
    char    sim_status[16];
    int     ant_rssi[4];
} DeviceStatus;

typedef struct {
    char    tx_bytes[32];
    char    rx_bytes[32];
    int     tx_rate;
    int     rx_rate;
    uint64_t total_tx;
    uint64_t total_rx;
    int     connected_devices;
    time_t  session_start;
} TrafficStats;

typedef struct {
    char    ssid[64];
    char    bssid[18];
    int     signal;
    int     channel;
    int     frequency;
    char    security[16];
    int     encryption;
    char    band[8];
    bool    is_5g;
} WifiScanResult;

typedef struct {
    char    ip[16];
    char    mac[18];
    char    hostname[64];
    char    interface[16];
    time_t  connected_time;
} ConnectedDevice;

typedef struct {
    char    session_id[64];
    char    username[32];
    time_t  create_time;
    time_t  expire_time;
    char    client_ip[16];
} Session;

int at_command_init(const char *device);
void at_command_close(void);
int at_command_send(const char *command, char *response, int timeout_ms);
int at_command_exec(const char *fmt, ...);
int at_get_signal_strength(void);
int at_get_network_registration(void);
int at_get_operator(char *operator_name, int len);
int at_set_apn(const char *apn, const char *username, const char *password, int auth_type);
int at_activate_pdp_context(void);
int at_deactivate_pdp_context(void);
int at_get_imsi(char *imsi, int len);
int at_get_iccid(char *iccid, int len);
int at_get_imei(char *imei, int len);
int at_set_network_mode(int mode);
int at_reboot(void);

int config_load(const char *config_file);
int config_save(const char *config_file);
int config_get_lan(LanConfig *config);
int config_set_lan(LanConfig *config);
int config_get_apn_list(ApnConfig *apns, int *count);
int config_set_apn(ApnConfig *apn);
int config_delete_apn(int apn_id);
int config_get_wlan_ap(WlanApConfig *config);
int config_set_wlan_ap(WlanApConfig *config);
int config_get_wlan_sta(WlanStaConfig *config);
int config_set_wlan_sta(WlanStaConfig *config);
int config_get_firewall(FirewallConfig *config);
int config_set_firewall(FirewallConfig *config);
int config_get_cellular(CellularConfig *config);
int config_set_cellular(CellularConfig *config);
int config_get_vpn(int type, VpnConfig *config);
int config_set_vpn(VpnConfig *config);
int config_get_iot(IotConfig *config);
int config_set_iot(IotConfig *config);
int config_get_system(SystemConfig *config);
int config_set_system(SystemConfig *config);
int config_add_port_forward(PortForwardRule *rule);
int config_edit_port_forward(PortForwardRule *rule);
int config_delete_port_forward(int rule_id);

int cgi_get_device_status(DeviceStatus *status);
int cgi_get_traffic_stats(TrafficStats *stats);
int cgi_get_lan_config(LanConfig *config);
int cgi_set_lan_config(LanConfig *config);
int cgi_get_apn_list(ApnConfig *apns, int *count);
int cgi_add_apn(ApnConfig *apn);
int cgi_edit_apn(ApnConfig *apn);
int cgi_delete_apn(int apn_id);
int cgi_activate_apn(int apn_id);
int cgi_get_wlan_ap_config(WlanApConfig *config);
int cgi_set_wlan_ap_config(WlanApConfig *config);
int cgi_get_wlan_sta_config(WlanStaConfig *config);
int cgi_set_wlan_sta_config(WlanStaConfig *config);
int cgi_scan_wifi(void);
int cgi_get_firewall_config(FirewallConfig *config);
int cgi_set_firewall_config(FirewallConfig *config);
int cgi_reboot(void);
int cgi_factory_reset(void);
int cgi_get_cellular_config(CellularConfig *config);
int cgi_set_cellular_config(CellularConfig *config);
int cgi_get_vpn_config(int type, VpnConfig *config);
int cgi_set_vpn_config(VpnConfig *config);
int cgi_vpn_connect(int type);
int cgi_vpn_disconnect(int type);
int cgi_get_iot_config(IotConfig *config);
int cgi_set_iot_config(IotConfig *config);
int cgi_get_system_config(SystemConfig *config);
int cgi_set_system_config(SystemConfig *config);
int cgi_get_connected_devices(ConnectedDevice *devices, int *count);
int cgi_send_at_command(const char *command, char *response, int timeout_ms);

void cgi_response_json(int code, const char *message, const char *data);
void cgi_response_error(int code, const char *message);

int json_parse_string(const char *json, const char *key, char *value, int len);
int json_parse_int(const char *json, const char *key, int *value);
int json_parse_bool(const char *json, const char *key, bool *value);
int json_parse_array(const char *json, const char *key, char *array, int len);

char* json_escape_string(const char *str);
void json_append_string(char *json, int len, const char *key, const char *value);
void json_append_int(char *json, int len, const char *key, int value);
void json_append_bool(char *json, int len, const char *key, bool value);

#endif
