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
#define MAX_APN_COUNT      10
#define AT_COMMAND_TIMEOUT  3000

#define CGI_OK             200
#define CGI_ERROR          500
#define CGI_NOT_FOUND      404

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
} DeviceStatus;

typedef struct {
    char    tx_bytes[32];
    char    rx_bytes[32];
    int     tx_rate;
    int     rx_rate;
} TrafficStats;

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

void cgi_response_json(int code, const char *message, const char *data);
void cgi_response_error(int code, const char *message);

#endif
