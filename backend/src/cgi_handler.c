#include "cpe.h"
#include <ctype.h>

#define MAX_POST_SIZE   (MAX_BUFFER_SIZE * 4)
#define MAX_RESPONSE_SIZE (MAX_BUFFER_SIZE * 16)

static char http_response[MAX_RESPONSE_SIZE];
static char post_data[MAX_POST_SIZE];

static int parse_json_string(const char *json, const char *key, char *value, int len)
{
    char search_key[128];
    snprintf(search_key, sizeof(search_key), "\"%s\"", key);
    
    const char *p = strstr(json, search_key);
    if (!p) return -1;
    
    p = strchr(p + strlen(search_key), ':');
    if (!p) return -1;
    p++;
    
    while (*p && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) p++;
    
    if (*p == '"') {
        p++;
        int i = 0;
        while (*p && *p != '"' && i < len - 1) {
            if (*p == '\\' && *(p+1)) {
                p++;
                switch (*p) {
                    case 'n': value[i++] = '\n'; break;
                    case 't': value[i++] = '\t'; break;
                    case 'r': value[i++] = '\r'; break;
                    case '"': value[i++] = '"'; break;
                    case '\\': value[i++] = '\\'; break;
                    default: value[i++] = *p; break;
                }
                p++;
            } else {
                value[i++] = *p++;
            }
        }
        value[i] = '\0';
        return 0;
    }
    
    return -1;
}

static int parse_json_int(const char *json, const char *key, int *value)
{
    char search_key[128];
    snprintf(search_key, sizeof(search_key), "\"%s\"", key);
    
    const char *p = strstr(json, search_key);
    if (!p) return -1;
    
    p = strchr(p + strlen(search_key), ':');
    if (!p) return -1;
    p++;
    
    while (*p && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) p++;
    
    if (isdigit(*p) || *p == '-') {
        *value = atoi(p);
        return 0;
    }
    
    return -1;
}

static int parse_json_bool(const char *json, const char *key, bool *value)
{
    char search_key[128];
    snprintf(search_key, sizeof(search_key), "\"%s\"", key);
    
    const char *p = strstr(json, search_key);
    if (!p) return -1;
    
    p = strchr(p + strlen(search_key), ':');
    if (!p) return -1;
    p++;
    
    while (*p && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) p++;
    
    if (strncmp(p, "true", 4) == 0) {
        *value = true;
        return 0;
    } else if (strncmp(p, "false", 5) == 0) {
        *value = false;
        return 0;
    }
    
    return -1;
}

void cgi_response_json(int code, const char *message, const char *data)
{
    printf("Content-Type: application/json\r\n");
    printf("Access-Control-Allow-Origin: *\r\n");
    printf("Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n");
    printf("Access-Control-Allow-Headers: Content-Type\r\n");
    printf("\r\n");
    
    printf("{\"code\":%d,\"message\":\"%s\"", code, message);
    if (data && strlen(data) > 0) {
        printf(",\"data\":%s", data);
    }
    printf("}\n");
}

void cgi_response_error(int code, const char *message)
{
    cgi_response_json(code, message, "null");
}

static void handle_status_request(void)
{
    DeviceStatus status;
    TrafficStats traffic;
    
    memset(&status, 0, sizeof(status));
    memset(&traffic, 0, sizeof(traffic));
    
    cgi_get_device_status(&status);
    cgi_get_traffic_stats(&traffic);
    
    char data[MAX_RESPONSE_SIZE];
    snprintf(data, sizeof(data),
        "{"
        "\"device_model\":\"%s\","
        "\"firmware_version\":\"%s\","
        "\"imei\":\"%s\","
        "\"iccid\":\"%s\","
        "\"imsi\":\"%s\","
        "\"lan_mac\":\"%s\","
        "\"wan_ip\":\"%s\","
        "\"wan_ipv6\":\"%s\","
        "\"uptime\":\"%s\","
        "\"uptime_seconds\":%d,"
        "\"signal_strength\":%d,"
        "\"network_type\":\"%s\","
        "\"network_mode\":\"%s\","
        "\"operator_name\":\"%s\","
        "\"mcc\":\"%s\","
        "\"mnc\":\"%s\","
        "\"rsrp\":%d,"
        "\"rsrq\":%d,"
        "\"sinr\":%d,"
        "\"band\":\"%s\","
        "\"earfcn\":%d,"
        "\"pci\":%d,"
        "\"cell_id\":\"%s\","
        "\"temperature\":%d,"
        "\"sim_status\":\"%s\","
        "\"tx_bytes\":\"%s\","
        "\"rx_bytes\":\"%s\","
        "\"tx_rate\":%d,"
        "\"rx_rate\":%d,"
        "\"connected_devices\":%d"
        "}",
        status.device_model, status.firmware_version, status.imei,
        status.iccid, status.imsi, status.lan_mac, status.wan_ip,
        status.wan_ipv6, status.uptime, status.uptime_seconds,
        status.signal_strength, status.network_type, status.network_mode_str,
        status.operator_name, status.mcc, status.mnc,
        status.rsrp, status.rsrq, status.sinr,
        status.band, status.earfcn, status.pci, status.cell_id,
        status.temperature, status.sim_status,
        traffic.tx_bytes, traffic.rx_bytes,
        traffic.tx_rate, traffic.rx_rate, traffic.connected_devices);
    
    cgi_response_json(CGI_OK, "success", data);
}

static void handle_lan_get_request(void)
{
    LanConfig config;
    memset(&config, 0, sizeof(config));
    cgi_get_lan_config(&config);
    
    char data[MAX_BUFFER_SIZE];
    snprintf(data, sizeof(data),
        "{\"work_mode\":\"%s\",\"ip\":\"%s\",\"netmask\":\"%s\","
        "\"dhcp_enabled\":%s,\"dhcp_start\":\"%s\",\"dhcp_end\":\"%s\","
        "\"dhcp_lease\":%d,\"dns1\":\"%s\",\"dns2\":\"%s\"}",
        config.work_mode, config.ip, config.netmask,
        config.dhcp_enabled ? "true" : "false",
        config.dhcp_start, config.dhcp_end,
        config.dhcp_lease, config.dns1, config.dns2);
    
    cgi_response_json(CGI_OK, "success", data);
}

static void handle_lan_set_request(const char *json)
{
    LanConfig config;
    memset(&config, 0, sizeof(config));
    
    parse_json_string(json, "work_mode", config.work_mode, sizeof(config.work_mode));
    parse_json_string(json, "ip", config.ip, sizeof(config.ip));
    parse_json_string(json, "netmask", config.netmask, sizeof(config.netmask));
    parse_json_bool(json, "dhcp_enabled", &config.dhcp_enabled);
    parse_json_string(json, "dhcp_start", config.dhcp_start, sizeof(config.dhcp_start));
    parse_json_string(json, "dhcp_end", config.dhcp_end, sizeof(config.dhcp_end));
    parse_json_int(json, "dhcp_lease", &config.dhcp_lease);
    parse_json_string(json, "dns1", config.dns1, sizeof(config.dns1));
    parse_json_string(json, "dns2", config.dns2, sizeof(config.dns2));
    
    if (cgi_set_lan_config(&config) == 0) {
        cgi_response_json(CGI_OK, "LAN配置已保存", "{\"restart_required\":true}");
    } else {
        cgi_response_error(CGI_ERROR, "LAN配置保存失败");
    }
}

static void handle_cellular_get_request(void)
{
    CellularConfig config;
    memset(&config, 0, sizeof(config));
    cgi_get_cellular_config(&config);
    
    const char *mode_str = "自动";
    switch (config.network_mode) {
        case NETWORK_5G_SA: mode_str = "5G SA"; break;
        case NETWORK_5G_SA_NSA: mode_str = "5G SA+NSA"; break;
        case NETWORK_LTE: mode_str = "LTE"; break;
        default: mode_str = "自动"; break;
    }
    
    char data[MAX_BUFFER_SIZE];
    snprintf(data, sizeof(data),
        "{\"network_mode\":%d,\"network_mode_str\":\"%s\","
        "\"bands_5g\":\"%s\",\"bands_lte\":\"%s\","
        "\"airplane_mode\":%s,\"data_roaming\":%s,"
        "\"hw_accel\":%s,\"ims_enabled\":%s}",
        config.network_mode, mode_str,
        config.bands_5g, config.bands_lte,
        config.airplane_mode ? "true" : "false",
        config.data_roaming ? "true" : "false",
        config.hw_accel ? "true" : "false",
        config.ims_enabled ? "true" : "false");
    
    cgi_response_json(CGI_OK, "success", data);
}

static void handle_cellular_set_request(const char *json)
{
    CellularConfig config;
    memset(&config, 0, sizeof(config));
    
    parse_json_int(json, "network_mode", &config.network_mode);
    parse_json_string(json, "bands_5g", config.bands_5g, sizeof(config.bands_5g));
    parse_json_string(json, "bands_lte", config.bands_lte, sizeof(config.bands_lte));
    parse_json_bool(json, "airplane_mode", &config.airplane_mode);
    parse_json_bool(json, "data_roaming", &config.data_roaming);
    parse_json_bool(json, "hw_accel", &config.hw_accel);
    parse_json_bool(json, "ims_enabled", &config.ims_enabled);
    parse_json_string(json, "preferred_plmn", config.preferred_plmn, sizeof(config.preferred_plmn));
    
    if (cgi_set_cellular_config(&config) == 0) {
        cgi_response_json(CGI_OK, "蜂窝网络配置已保存", "null");
    } else {
        cgi_response_error(CGI_ERROR, "蜂窝网络配置保存失败");
    }
}

static void handle_apn_list_request(void)
{
    ApnConfig apns[MAX_APN_COUNT];
    int count = 0;
    
    memset(apns, 0, sizeof(apns));
    cgi_get_apn_list(apns, &count);
    
    char data[MAX_RESPONSE_SIZE];
    strcpy(data, "[");
    
    for (int i = 0; i < count; i++) {
        char apn_json[512];
        snprintf(apn_json, sizeof(apn_json),
            "%s{\"id\":%d,\"name\":\"%s\",\"username\":\"%s\",\"password\":\"******\","
            "\"auth_type\":%d,\"bearer_type\":%d,\"is_default\":%s,\"is_active\":%s}",
            i > 0 ? "," : "",
            apns[i].id, apns[i].name, apns[i].username,
            apns[i].auth_type, apns[i].bearer_type,
            apns[i].is_default ? "true" : "false",
            apns[i].is_active ? "true" : "false");
        strcat(data, apn_json);
    }
    strcat(data, "]");
    
    cgi_response_json(CGI_OK, "success", data);
}

static void handle_apn_add_request(const char *json)
{
    ApnConfig apn;
    memset(&apn, 0, sizeof(apn));
    
    apn.id = (int)time(NULL);
    parse_json_string(json, "name", apn.name, sizeof(apn.name));
    parse_json_string(json, "username", apn.username, sizeof(apn.username));
    parse_json_string(json, "password", apn.password, sizeof(apn.password));
    parse_json_int(json, "auth_type", (int*)&apn.auth_type);
    parse_json_int(json, "bearer_type", (int*)&apn.bearer_type);
    parse_json_bool(json, "is_default", &apn.is_default);
    parse_json_bool(json, "is_active", &apn.is_active);
    
    if (cgi_add_apn(&apn) == 0) {
        char data[64];
        snprintf(data, sizeof(data), "{\"id\":%d}", apn.id);
        cgi_response_json(CGI_OK, "APN添加成功", data);
    } else {
        cgi_response_error(CGI_ERROR, "APN添加失败");
    }
}

static void handle_apn_edit_request(const char *json)
{
    ApnConfig apn;
    memset(&apn, 0, sizeof(apn));
    
    parse_json_int(json, "id", &apn.id);
    parse_json_string(json, "name", apn.name, sizeof(apn.name));
    parse_json_string(json, "username", apn.username, sizeof(apn.username));
    parse_json_string(json, "password", apn.password, sizeof(apn.password));
    parse_json_int(json, "auth_type", (int*)&apn.auth_type);
    parse_json_int(json, "bearer_type", (int*)&apn.bearer_type);
    parse_json_bool(json, "is_default", &apn.is_default);
    parse_json_bool(json, "is_active", &apn.is_active);
    
    if (cgi_edit_apn(&apn) == 0) {
        cgi_response_json(CGI_OK, "APN更新成功", "null");
    } else {
        cgi_response_error(CGI_ERROR, "APN更新失败");
    }
}

static void handle_apn_delete_request(const char *json)
{
    int apn_id = 0;
    parse_json_int(json, "id", &apn_id);
    
    if (cgi_delete_apn(apn_id) == 0) {
        cgi_response_json(CGI_OK, "APN删除成功", "null");
    } else {
        cgi_response_error(CGI_ERROR, "APN删除失败");
    }
}

static void handle_apn_activate_request(const char *json)
{
    int apn_id = 0;
    parse_json_int(json, "id", &apn_id);
    
    if (cgi_activate_apn(apn_id) == 0) {
        cgi_response_json(CGI_OK, "APN激活成功", "null");
    } else {
        cgi_response_error(CGI_ERROR, "APN激活失败");
    }
}

static void handle_wlan_ap_get_request(void)
{
    WlanApConfig config;
    memset(&config, 0, sizeof(config));
    cgi_get_wlan_ap_config(&config);
    
    char data[MAX_BUFFER_SIZE];
    snprintf(data, sizeof(data),
        "{\"enabled_2g4\":%s,\"enabled_5g\":%s,"
        "\"ssid_2g4\":\"%s\",\"ssid_5g\":\"%s\","
        "\"password_2g4\":\"******\",\"password_5g\":\"******\","
        "\"channel_2g4\":%d,\"channel_5g\":%d,"
        "\"encryption\":%d,\"hidden_2g4\":%s,\"hidden_5g\":%s,"
        "\"max_clients\":%d,\"bandwidth_2g4\":%d,\"bandwidth_5g\":%d}",
        config.enabled_2g4 ? "true" : "false",
        config.enabled_5g ? "true" : "false",
        config.ssid_2g4, config.ssid_5g,
        config.channel_2g4, config.channel_5g,
        config.encryption,
        config.hidden_2g4 ? "true" : "false",
        config.hidden_5g ? "true" : "false",
        config.max_clients, config.bandwidth_2g4, config.bandwidth_5g);
    
    cgi_response_json(CGI_OK, "success", data);
}

static void handle_wlan_ap_set_request(const char *json)
{
    WlanApConfig config;
    memset(&config, 0, sizeof(config));
    
    parse_json_bool(json, "enabled_2g4", &config.enabled_2g4);
    parse_json_bool(json, "enabled_5g", &config.enabled_5g);
    parse_json_string(json, "ssid_2g4", config.ssid_2g4, sizeof(config.ssid_2g4));
    parse_json_string(json, "ssid_5g", config.ssid_5g, sizeof(config.ssid_5g));
    parse_json_string(json, "password_2g4", config.password_2g4, sizeof(config.password_2g4));
    parse_json_string(json, "password_5g", config.password_5g, sizeof(config.password_5g));
    parse_json_int(json, "channel_2g4", &config.channel_2g4);
    parse_json_int(json, "channel_5g", &config.channel_5g);
    parse_json_int(json, "encryption", &config.encryption);
    parse_json_bool(json, "hidden_2g4", &config.hidden_2g4);
    parse_json_bool(json, "hidden_5g", &config.hidden_5g);
    parse_json_int(json, "max_clients", &config.max_clients);
    parse_json_int(json, "bandwidth_2g4", &config.bandwidth_2g4);
    parse_json_int(json, "bandwidth_5g", &config.bandwidth_5g);
    
    if (cgi_set_wlan_ap_config(&config) == 0) {
        cgi_response_json(CGI_OK, "WLAN AP配置已保存", "null");
    } else {
        cgi_response_error(CGI_ERROR, "WLAN AP配置保存失败");
    }
}

static void handle_wlan_sta_get_request(void)
{
    WlanStaConfig config;
    memset(&config, 0, sizeof(config));
    cgi_get_wlan_sta_config(&config);
    
    char data[MAX_BUFFER_SIZE];
    snprintf(data, sizeof(data),
        "{\"sta_enabled\":%s,\"target_ssid\":\"%s\","
        "\"target_password\":\"******\",\"security_type\":%d,"
        "\"band\":%d,\"wan_type\":%d,\"wan_mac\":\"%s\",\"nat_enabled\":%s}",
        config.sta_enabled ? "true" : "false",
        config.target_ssid, config.security_type,
        config.band, config.wan_type, config.wan_mac,
        config.nat_enabled ? "true" : "false");
    
    cgi_response_json(CGI_OK, "success", data);
}

static void handle_wlan_sta_set_request(const char *json)
{
    WlanStaConfig config;
    memset(&config, 0, sizeof(config));
    
    parse_json_bool(json, "sta_enabled", &config.sta_enabled);
    parse_json_string(json, "target_ssid", config.target_ssid, sizeof(config.target_ssid));
    parse_json_string(json, "target_password", config.target_password, sizeof(config.target_password));
    parse_json_int(json, "security_type", &config.security_type);
    parse_json_int(json, "band", &config.band);
    parse_json_int(json, "wan_type", &config.wan_type);
    parse_json_string(json, "wan_mac", config.wan_mac, sizeof(config.wan_mac));
    parse_json_bool(json, "nat_enabled", &config.nat_enabled);
    
    if (cgi_set_wlan_sta_config(&config) == 0) {
        cgi_response_json(CGI_OK, "WLAN STA配置已保存", "null");
    } else {
        cgi_response_error(CGI_ERROR, "WLAN STA配置保存失败");
    }
}

static void handle_wifi_scan_request(void)
{
    cgi_response_json(CGI_OK, "success", 
        "{\"scan_time\":\"2024-01-15 10:30:00\",\"count\":2,"
        "\"networks\":["
        "{\"ssid\":\"MyWiFi_5G\",\"bssid\":\"AA:BB:CC:DD:EE:FF\",\"signal\":85,\"channel\":36,\"security\":\"WPA2\",\"is_5g\":true},"
        "{\"ssid\":\"MyWiFi_24G\",\"bssid\":\"AA:BB:CC:DD:EE:FE\",\"signal\":75,\"channel\":6,\"security\":\"WPA2\",\"is_5g\":false}"
        "]}");
}

static void handle_firewall_get_request(void)
{
    FirewallConfig config;
    memset(&config, 0, sizeof(config));
    cgi_get_firewall_config(&config);
    
    char data[MAX_RESPONSE_SIZE];
    snprintf(data, sizeof(data),
        "{\"enabled\":%s,\"dmz_enabled\":%s,\"dmz_ip\":\"%s\","
        "\"spi_enabled\":%s,\"dos_enabled\":%s,\"arp_proxy\":%s,\"ping_wan\":%s,"
        "\"port_forwards\":[",
        config.enabled ? "true" : "false",
        config.dmz_enabled ? "true" : "false",
        config.dmz_ip,
        config.spi_enabled ? "true" : "false",
        config.dos_enabled ? "true" : "false",
        config.arp_proxy ? "true" : "false",
        config.ping_wan ? "true" : "false");
    
    for (int i = 0; i < config.port_forward_count; i++) {
        char rule_json[256];
        snprintf(rule_json, sizeof(rule_json),
            "%s{\"id\":%d,\"protocol\":\"%s\",\"external_port\":%d,"
            "\"internal_ip\":\"%s\",\"internal_port\":%d,"
            "\"description\":\"%s\",\"enabled\":%s}",
            i > 0 ? "," : "",
            config.port_forwards[i].id,
            config.port_forwards[i].protocol,
            config.port_forwards[i].external_port,
            config.port_forwards[i].internal_ip,
            config.port_forwards[i].internal_port,
            config.port_forwards[i].description,
            config.port_forwards[i].enabled ? "true" : "false");
        strcat(data, rule_json);
    }
    strcat(data, "]}");
    
    cgi_response_json(CGI_OK, "success", data);
}

static void handle_firewall_set_request(const char *json)
{
    FirewallConfig config;
    memset(&config, 0, sizeof(config));
    
    parse_json_bool(json, "enabled", &config.enabled);
    parse_json_bool(json, "dmz_enabled", &config.dmz_enabled);
    parse_json_string(json, "dmz_ip", config.dmz_ip, sizeof(config.dmz_ip));
    parse_json_bool(json, "spi_enabled", &config.spi_enabled);
    parse_json_bool(json, "dos_enabled", &config.dos_enabled);
    parse_json_bool(json, "arp_proxy", &config.arp_proxy);
    parse_json_bool(json, "ping_wan", &config.ping_wan);
    
    if (cgi_set_firewall_config(&config) == 0) {
        cgi_response_json(CGI_OK, "防火墙配置已保存", "null");
    } else {
        cgi_response_error(CGI_ERROR, "防火墙配置保存失败");
    }
}

static void handle_port_forward_add_request(const char *json)
{
    PortForwardRule rule;
    memset(&rule, 0, sizeof(rule));
    
    rule.id = (int)time(NULL);
    parse_json_string(json, "protocol", rule.protocol, sizeof(rule.protocol));
    parse_json_int(json, "external_port", &rule.external_port);
    parse_json_string(json, "internal_ip", rule.internal_ip, sizeof(rule.internal_ip));
    parse_json_int(json, "internal_port", &rule.internal_port);
    parse_json_string(json, "description", rule.description, sizeof(rule.description));
    parse_json_bool(json, "enabled", &rule.enabled);
    
    if (config_add_port_forward(&rule) == 0) {
        char data[64];
        snprintf(data, sizeof(data), "{\"id\":%d}", rule.id);
        cgi_response_json(CGI_OK, "端口转发规则添加成功", data);
    } else {
        cgi_response_error(CGI_ERROR, "端口转发规则添加失败");
    }
}

static void handle_port_forward_delete_request(const char *json)
{
    int rule_id = 0;
    parse_json_int(json, "id", &rule_id);
    
    if (config_delete_port_forward(rule_id) == 0) {
        cgi_response_json(CGI_OK, "端口转发规则删除成功", "null");
    } else {
        cgi_response_error(CGI_ERROR, "端口转发规则删除失败");
    }
}

static void handle_vpn_get_request(const char *query)
{
    int vpn_type = VPN_TYPE_PPTP;
    
    const char *type_param = strstr(query, "type=");
    if (type_param) {
        type_param += 5;
        if (strncmp(type_param, "pptp", 4) == 0) vpn_type = VPN_TYPE_PPTP;
        else if (strncmp(type_param, "l2tp", 4) == 0) vpn_type = VPN_TYPE_L2TP;
        else if (strncmp(type_param, "gre", 3) == 0) vpn_type = VPN_TYPE_GRE;
        else if (strncmp(type_param, "eoip", 4) == 0) vpn_type = VPN_TYPE_EOIP;
        else if (strncmp(type_param, "ipsec", 5) == 0) vpn_type = VPN_TYPE_IPSEC;
    }
    
    VpnConfig config;
    memset(&config, 0, sizeof(config));
    cgi_get_vpn_config(vpn_type, &config);
    
    char data[MAX_BUFFER_SIZE];
    snprintf(data, sizeof(data),
        "{\"type\":%d,\"enabled\":%s,\"server\":\"%s\","
        "\"username\":\"%s\",\"password\":\"******\","
        "\"mppe\":%s,\"local_ip\":\"%s\",\"remote_ip\":\"%s\"}",
        config.type, config.enabled ? "true" : "false",
        config.server, config.username,
        config.mppe ? "true" : "false",
        config.local_ip, config.remote_ip);
    
    cgi_response_json(CGI_OK, "success", data);
}

static void handle_vpn_set_request(const char *json)
{
    VpnConfig config;
    memset(&config, 0, sizeof(config));
    
    parse_json_int(json, "type", &config.type);
    parse_json_bool(json, "enabled", &config.enabled);
    parse_json_string(json, "server", config.server, sizeof(config.server));
    parse_json_string(json, "username", config.username, sizeof(config.username));
    parse_json_string(json, "password", config.password, sizeof(config.password));
    parse_json_bool(json, "mppe", &config.mppe);
    
    if (cgi_set_vpn_config(&config) == 0) {
        cgi_response_json(CGI_OK, "VPN配置已保存", "null");
    } else {
        cgi_response_error(CGI_ERROR, "VPN配置保存失败");
    }
}

static void handle_vpn_connect_request(const char *json)
{
    int vpn_type = VPN_TYPE_PPTP;
    parse_json_int(json, "type", &vpn_type);
    
    if (cgi_vpn_connect(vpn_type) == 0) {
        cgi_response_json(CGI_OK, "正在连接VPN...", "{\"status\":\"connecting\"}");
    } else {
        cgi_response_error(CGI_ERROR, "VPN连接失败");
    }
}

static void handle_vpn_disconnect_request(const char *json)
{
    int vpn_type = VPN_TYPE_PPTP;
    parse_json_int(json, "type", &vpn_type);
    
    if (cgi_vpn_disconnect(vpn_type) == 0) {
        cgi_response_json(CGI_OK, "VPN已断开", "null");
    } else {
        cgi_response_error(CGI_ERROR, "VPN断开失败");
    }
}

static void handle_iot_get_request(void)
{
    IotConfig config;
    memset(&config, 0, sizeof(config));
    cgi_get_iot_config(&config);
    
    char data[MAX_RESPONSE_SIZE];
    snprintf(data, sizeof(data),
        "{\"enabled\":%s,\"protocol\":%d,\"server\":\"%s\","
        "\"port\":%d,\"client_id\":\"%s\",\"username\":\"%s\","
        "\"password\":\"******\",\"keepalive\":%d,\"qos\":%d,"
        "\"clean_session\":%s,\"publish_topic\":\"%s\",\"subscribe_topic\":\"%s\"}",
        config.enabled ? "true" : "false",
        config.protocol, config.server, config.port,
        config.client_id, config.username,
        config.keepalive, config.qos,
        config.clean_session ? "true" : "false",
        config.publish_topic, config.subscribe_topic);
    
    cgi_response_json(CGI_OK, "success", data);
}

static void handle_iot_set_request(const char *json)
{
    IotConfig config;
    memset(&config, 0, sizeof(config));
    
    parse_json_bool(json, "enabled", &config.enabled);
    parse_json_int(json, "protocol", &config.protocol);
    parse_json_string(json, "server", config.server, sizeof(config.server));
    parse_json_int(json, "port", &config.port);
    parse_json_string(json, "client_id", config.client_id, sizeof(config.client_id));
    parse_json_string(json, "username", config.username, sizeof(config.username));
    parse_json_string(json, "password", config.password, sizeof(config.password));
    parse_json_int(json, "keepalive", &config.keepalive);
    parse_json_int(json, "qos", &config.qos);
    parse_json_bool(json, "clean_session", &config.clean_session);
    parse_json_string(json, "publish_topic", config.publish_topic, sizeof(config.publish_topic));
    parse_json_string(json, "subscribe_topic", config.subscribe_topic, sizeof(config.subscribe_topic));
    
    if (cgi_set_iot_config(&config) == 0) {
        cgi_response_json(CGI_OK, "IOT配置已保存", "null");
    } else {
        cgi_response_error(CGI_ERROR, "IOT配置保存失败");
    }
}

static void handle_system_get_request(void)
{
    SystemConfig config;
    memset(&config, 0, sizeof(config));
    cgi_get_system_config(&config);
    
    char data[MAX_BUFFER_SIZE];
    snprintf(data, sizeof(data),
        "{\"device_name\":\"%s\",\"timezone\":\"%s\","
        "\"ntp_server\":\"%s\",\"web_port\":%d,"
        "\"admin_user\":\"%s\",\"https_enabled\":%s,"
        "\"remote_manage\":%s,\"language\":\"%s\"}",
        config.device_name, config.timezone,
        config.ntp_server, config.web_port,
        config.admin_user,
        config.https_enabled ? "true" : "false",
        config.remote_manage ? "true" : "false",
        config.language);
    
    cgi_response_json(CGI_OK, "success", data);
}

static void handle_system_set_request(const char *json)
{
    SystemConfig config;
    memset(&config, 0, sizeof(config));
    
    parse_json_string(json, "device_name", config.device_name, sizeof(config.device_name));
    parse_json_string(json, "timezone", config.timezone, sizeof(config.timezone));
    parse_json_string(json, "ntp_server", config.ntp_server, sizeof(config.ntp_server));
    parse_json_int(json, "web_port", &config.web_port);
    parse_json_bool(json, "https_enabled", &config.https_enabled);
    parse_json_bool(json, "remote_manage", &config.remote_manage);
    parse_json_string(json, "language", config.language, sizeof(config.language));
    
    if (cgi_set_system_config(&config) == 0) {
        cgi_response_json(CGI_OK, "系统配置已保存", "null");
    } else {
        cgi_response_error(CGI_ERROR, "系统配置保存失败");
    }
}

static void handle_reboot_request(void)
{
    if (cgi_reboot() == 0) {
        cgi_response_json(CGI_OK, "设备正在重启...", "null");
    } else {
        cgi_response_error(CGI_ERROR, "重启命令执行失败");
    }
}

static void handle_factory_reset_request(void)
{
    if (cgi_factory_reset() == 0) {
        cgi_response_json(CGI_OK, "正在恢复出厂设置...", "null");
    } else {
        cgi_response_error(CGI_ERROR, "恢复出厂设置失败");
    }
}

static void handle_at_send_request(const char *json)
{
    char command[256] = {0};
    int timeout = 3000;
    
    parse_json_string(json, "command", command, sizeof(command));
    parse_json_int(json, "timeout", &timeout);
    
    if (strlen(command) == 0) {
        cgi_response_error(CGI_BAD_REQUEST, "请输入AT指令");
        return;
    }
    
    char response[MAX_BUFFER_SIZE];
    memset(response, 0, sizeof(response));
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    int ret = cgi_send_at_command(command, response, timeout);
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    int duration_ms = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_nsec - start.tv_nsec) / 1000000;
    
    char *escaped = json_escape_string(response);
    char data[MAX_RESPONSE_SIZE];
    snprintf(data, sizeof(data),
        "{\"command\":\"%s\",\"response\":\"%s\",\"duration_ms\":%d,\"success\":%s}",
        command, escaped ? escaped : response, duration_ms, ret == 0 ? "true" : "false");
    
    if (escaped) free(escaped);
    
    cgi_response_json(CGI_OK, "success", data);
}

static void handle_device_list_request(void)
{
    ConnectedDevice devices[64];
    int count = 0;
    
    memset(devices, 0, sizeof(devices));
    cgi_get_connected_devices(devices, &count);
    
    char data[MAX_RESPONSE_SIZE];
    snprintf(data, sizeof(data), "{\"count\":%d,\"devices\":[", count);
    
    for (int i = 0; i < count; i++) {
        char device_json[256];
        snprintf(device_json, sizeof(device_json),
            "%s{\"ip\":\"%s\",\"mac\":\"%s\",\"hostname\":\"%s\",\"interface\":\"%s\"}",
            i > 0 ? "," : "",
            devices[i].ip, devices[i].mac,
            devices[i].hostname, devices[i].interface);
        strcat(data, device_json);
    }
    strcat(data, "]}");
    
    cgi_response_json(CGI_OK, "success", data);
}

static void handle_login_request(const char *json)
{
    char username[64] = {0};
    char password[128] = {0};
    
    parse_json_string(json, "username", username, sizeof(username));
    parse_json_string(json, "password", password, sizeof(password));
    
    SystemConfig config;
    cgi_get_system_config(&config);
    
    if (strcmp(username, config.admin_user) == 0 &&
        strcmp(password, config.admin_pass) == 0) {
        char session_id[64];
        snprintf(session_id, sizeof(session_id), "%ld%06d", time(NULL), rand() % 1000000);
        
        char data[256];
        snprintf(data, sizeof(data),
            "{\"session_id\":\"%s\",\"expires\":%ld,\"username\":\"%s\"}",
            session_id, time(NULL) + 1800, username);
        cgi_response_json(CGI_OK, "登录成功", data);
    } else {
        cgi_response_error(CGI_UNAUTHORIZED, "用户名或密码错误");
    }
}

static void handle_password_change_request(const char *json)
{
    char old_password[128] = {0};
    char new_password[128] = {0};
    char confirm_password[128] = {0};
    
    parse_json_string(json, "old_password", old_password, sizeof(old_password));
    parse_json_string(json, "new_password", new_password, sizeof(new_password));
    parse_json_string(json, "confirm_password", confirm_password, sizeof(confirm_password));
    
    SystemConfig config;
    cgi_get_system_config(&config);
    
    if (strcmp(old_password, config.admin_pass) != 0) {
        cgi_response_error(CGI_BAD_REQUEST, "原密码错误");
        return;
    }
    
    if (strcmp(new_password, confirm_password) != 0) {
        cgi_response_error(CGI_BAD_REQUEST, "两次输入的密码不一致");
        return;
    }
    
    if (strlen(new_password) < 6) {
        cgi_response_error(CGI_BAD_REQUEST, "密码长度不能少于6位");
        return;
    }
    
    strncpy(config.admin_pass, new_password, sizeof(config.admin_pass) - 1);
    cgi_set_system_config(&config);
    
    cgi_response_json(CGI_OK, "密码修改成功，请重新登录", "null");
}

typedef struct {
    const char *action;
    void (*get_handler)(void);
    void (*get_handler_with_query)(const char *query);
    void (*post_handler)(const char *json);
} RouteEntry;

static const RouteEntry route_table[] = {
    { "status",           handle_status_request,      NULL, NULL },
    { "traffic_stats",    handle_status_request,      NULL, NULL },
    { "device_list",      handle_device_list_request, NULL, NULL },
    
    { "lan_get",          handle_lan_get_request,     NULL, NULL },
    { "lan_set",          NULL,                       NULL, handle_lan_set_request },
    
    { "cellular_get",     handle_cellular_get_request, NULL, NULL },
    { "cellular_set",     NULL,                       NULL, handle_cellular_set_request },
    
    { "apn_list",         handle_apn_list_request,    NULL, NULL },
    { "apn_add",          NULL,                       NULL, handle_apn_add_request },
    { "apn_edit",         NULL,                       NULL, handle_apn_edit_request },
    { "apn_delete",       NULL,                       NULL, handle_apn_delete_request },
    { "apn_activate",     NULL,                       NULL, handle_apn_activate_request },
    
    { "wlan_ap_get",      handle_wlan_ap_get_request, NULL, NULL },
    { "wlan_ap_set",      NULL,                       NULL, handle_wlan_ap_set_request },
    { "wlan_sta_get",     handle_wlan_sta_get_request, NULL, NULL },
    { "wlan_sta_set",     NULL,                       NULL, handle_wlan_sta_set_request },
    { "wifi_scan",        handle_wifi_scan_request,   NULL, NULL },
    
    { "firewall_get",     handle_firewall_get_request, NULL, NULL },
    { "firewall_set",     NULL,                       NULL, handle_firewall_set_request },
    { "port_forward_add", NULL,                       NULL, handle_port_forward_add_request },
    { "port_forward_delete", NULL,                    NULL, handle_port_forward_delete_request },
    
    { "vpn_get",          NULL,                       handle_vpn_get_request, NULL },
    { "vpn_set",          NULL,                       NULL, handle_vpn_set_request },
    { "vpn_connect",      NULL,                       NULL, handle_vpn_connect_request },
    { "vpn_disconnect",   NULL,                       NULL, handle_vpn_disconnect_request },
    
    { "iot_get",          handle_iot_get_request,     NULL, NULL },
    { "iot_set",          NULL,                       NULL, handle_iot_set_request },
    
    { "system_get",       handle_system_get_request,  NULL, NULL },
    { "system_set",       NULL,                       NULL, handle_system_set_request },
    { "reboot",           NULL,                       NULL, handle_reboot_request },
    { "factory_reset",    NULL,                       NULL, handle_factory_reset_request },
    
    { "at_send",          NULL,                       NULL, handle_at_send_request },
    
    { "login",            NULL,                       NULL, handle_login_request },
    { "password_change",  NULL,                       NULL, handle_password_change_request },
    
    { NULL, NULL, NULL, NULL }
};

int main(int argc, char *argv[])
{
    char *method = getenv("REQUEST_METHOD");
    char *query = getenv("QUERY_STRING");
    char *content_type = getenv("CONTENT_TYPE");
    char *content_length = getenv("CONTENT_LENGTH");
    
    if (!method) {
        cgi_response_error(CGI_ERROR, "No request method");
        return 0;
    }
    
    if (strcmp(method, "OPTIONS") == 0) {
        printf("Content-Type: application/json\r\n");
        printf("Access-Control-Allow-Origin: *\r\n");
        printf("Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n");
        printf("Access-Control-Allow-Headers: Content-Type\r\n");
        printf("\r\n");
        return 0;
    }
    
    if (strcmp(method, "POST") == 0) {
        if (content_length) {
            int len = atoi(content_length);
            if (len > 0 && len < sizeof(post_data)) {
                fread(post_data, 1, len, stdin);
                post_data[len] = '\0';
            }
        }
    }
    
    char action[64] = "status";
    if (query && strlen(query) > 0) {
        if (sscanf(query, "action=%63s", action) != 1) {
            strncpy(action, query, sizeof(action) - 1);
        }
    }
    
    const RouteEntry *route = NULL;
    for (int i = 0; route_table[i].action != NULL; i++) {
        if (strstr(action, route_table[i].action) != NULL) {
            route = &route_table[i];
            break;
        }
    }
    
    if (!route) {
        cgi_response_error(CGI_NOT_FOUND, "Unknown action");
        return 0;
    }
    
    if (strcmp(method, "GET") == 0) {
        if (route->get_handler) {
            route->get_handler();
        } else if (route->get_handler_with_query) {
            route->get_handler_with_query(query);
        } else {
            cgi_response_error(CGI_BAD_REQUEST, "GET method not supported");
        }
    } else if (strcmp(method, "POST") == 0) {
        if (route->post_handler) {
            route->post_handler(post_data);
        } else {
            cgi_response_error(CGI_BAD_REQUEST, "POST method not supported");
        }
    } else {
        cgi_response_error(CGI_BAD_REQUEST, "Unsupported method");
    }
    
    return 0;
}
