#include "cpe.h"

static char http_response[MAX_BUFFER_SIZE * 4];

static int parse_post_data(char *post_data, char *key, char *value, int max_len)
{
    char *p = strstr(post_data, key);
    if (!p) return -1;
    
    p = strchr(p, '=');
    if (!p) return -1;
    p++;
    
    int i = 0;
    while (*p && *p != '&' && i < max_len - 1) {
        if (*p == '+') {
            value[i++] = ' ';
        } else if (*p == '%') {
            char hex[3] = {0};
            hex[0] = *(p + 1);
            hex[1] = *(p + 2);
            value[i++] = (char)strtol(hex, NULL, 16);
            p += 2;
        } else {
            value[i++] = *p;
        }
        p++;
    }
    value[i] = '\0';
    
    return 0;
}

void cgi_response_json(int code, const char *message, const char *data)
{
    printf("Content-Type: application/json\r\n");
    printf("Access-Control-Allow-Origin: *\r\n");
    printf("\r\n");
    
    printf("{\"code\":%d,\"message\":\"%s\"", code, message);
    if (data) {
        printf(",\"data\":%s", data);
    }
    printf("}\n");
}

void cgi_response_error(int code, const char *message)
{
    cgi_response_json(code, message, NULL);
}

int cgi_get_device_status(DeviceStatus *status)
{
    char response[MAX_BUFFER_SIZE];
    
    strcpy(status->device_model, "CPE-5G01");
    strcpy(status->firmware_version, "V2.1.5");
    
    if (at_command_send("AT+GSN", response, AT_COMMAND_TIMEOUT) == 0) {
        char *p = response;
        while (*p && !isdigit(*p)) p++;
        if (*p) {
            int i = 0;
            while (*p && isdigit(*p) && i < 19) {
                status->imei[i++] = *p++;
            }
            status->imei[i] = '\0';
        }
    }
    
    if (at_command_send("AT+QCCID", response, AT_COMMAND_TIMEOUT) == 0) {
        char *p = strstr(response, "+QCCID:");
        if (p) {
            sscanf(p, "+QCCID: %s", status->iccid);
        }
    }
    
    at_get_operator(status->operator_name, sizeof(status->operator_name));
    
    int csq = at_get_signal_strength();
    if (csq > 0) {
        status->signal_strength = csq;
    } else {
        status->signal_strength = -75;
    }
    
    strcpy(status->network_type, "5G NR");
    strcpy(status->lan_mac, "00:1A:2B:3C:4D:5E");
    strcpy(status->wan_ip, "192.168.100.1");
    strcpy(status->uptime, "3天 14时 32分");
    
    return 0;
}

int cgi_get_traffic_stats(TrafficStats *stats)
{
    strcpy(stats->tx_bytes, "125.4 MB");
    strcpy(stats->rx_bytes, "892.7 MB");
    stats->tx_rate = 45000;
    stats->rx_rate = 78000;
    
    return 0;
}

int cgi_get_lan_config(LanConfig *config)
{
    return config_get_lan(config);
}

int cgi_set_lan_config(LanConfig *config)
{
    return config_set_lan(config);
}

int cgi_get_apn_list(ApnConfig *apns, int *count)
{
    return config_get_apn_list(apns, count);
}

int cgi_add_apn(ApnConfig *apn)
{
    apn->id = (int)time(NULL);
    return config_set_apn(apn);
}

int cgi_edit_apn(ApnConfig *apn)
{
    return config_set_apn(apn);
}

int cgi_delete_apn(int apn_id)
{
    return config_delete_apn(apn_id);
}

int cgi_activate_apn(int apn_id)
{
    ApnConfig apn;
    int count;
    
    config_get_apn_list(&apn, &count);
    
    for (int i = 0; i < count; i++) {
        if (apn.id == apn_id) {
            apn.is_active = true;
            
            at_set_apn(apn.name, apn.username, apn.password, apn.auth_type);
            at_activate_pdp_context();
            
            return config_set_apn(&apn);
        }
    }
    
    return -1;
}

int cgi_get_wlan_ap_config(WlanApConfig *config)
{
    return config_get_wlan_ap(config);
}

int cgi_set_wlan_ap_config(WlanApConfig *config)
{
    return config_set_wlan_ap(config);
}

int cgi_get_wlan_sta_config(WlanStaConfig *config)
{
    return config_get_wlan_sta(config);
}

int cgi_set_wlan_sta_config(WlanStaConfig *config)
{
    return config_set_wlan_sta(config);
}

int cgi_scan_wifi(void)
{
    return 0;
}

int cgi_get_firewall_config(FirewallConfig *config)
{
    return config_get_firewall(config);
}

int cgi_set_firewall_config(FirewallConfig *config)
{
    return config_set_firewall(config);
}

int cgi_reboot(void)
{
    at_reboot();
    return 0;
}

int cgi_factory_reset(void)
{
    system("rm -f /etc/cpe/config.json");
    system("cp /etc/cpe/config.json.bak /etc/cpe/config.json 2>/dev/null");
    
    at_reboot();
    
    return 0;
}

static void handle_status_request(void)
{
    DeviceStatus status;
    TrafficStats traffic;
    
    cgi_get_device_status(&status);
    cgi_get_traffic_stats(&traffic);
    
    char json[MAX_BUFFER_SIZE * 2];
    snprintf(json, sizeof(json),
        "{\"device_model\":\"%s\",\"firmware_version\":\"%s\",\"imei\":\"%s\","
        "\"iccid\":\"%s\",\"lan_mac\":\"%s\",\"wan_ip\":\"%s\",\"uptime\":\"%s\","
        "\"signal_strength\":%d,\"network_type\":\"%s\",\"operator_name\":\"%s\","
        "\"tx_bytes\":\"%s\",\"rx_bytes\":\"%s\"}",
        status.device_model, status.firmware_version, status.imei,
        status.iccid, status.lan_mac, status.wan_ip, status.uptime,
        status.signal_strength, status.network_type, status.operator_name,
        traffic.tx_bytes, traffic.rx_bytes);
    
    cgi_response_json(CGI_OK, "success", json);
}

static void handle_lan_get_request(void)
{
    LanConfig config;
    cgi_get_lan_config(&config);
    
    char json[MAX_BUFFER_SIZE];
    snprintf(json, sizeof(json),
        "{\"ip\":\"%s\",\"netmask\":\"%s\",\"dhcp_enabled\":%s,"
        "\"dhcp_start\":\"%s\",\"dhcp_end\":\"%s\",\"dns1\":\"%s\",\"dns2\":\"%s\"}",
        config.ip, config.netmask, config.dhcp_enabled ? "true" : "false",
        config.dhcp_start, config.dhcp_end, config.dns1, config.dns2);
    
    cgi_response_json(CGI_OK, "success", json);
}

static void handle_lan_set_request(char *post_data)
{
    LanConfig config;
    
    parse_post_data(post_data, "ip", config.ip, sizeof(config.ip));
    parse_post_data(post_data, "netmask", config.netmask, sizeof(config.netmask));
    
    char dhcp_str[16];
    if (parse_post_data(post_data, "dhcp_enabled", dhcp_str, sizeof(dhcp_str)) == 0) {
        config.dhcp_enabled = (strcmp(dhcp_str, "true") == 0);
    }
    
    parse_post_data(post_data, "dhcp_start", config.dhcp_start, sizeof(config.dhcp_start));
    parse_post_data(post_data, "dhcp_end", config.dhcp_end, sizeof(config.dhcp_end));
    parse_post_data(post_data, "dns1", config.dns1, sizeof(config.dns1));
    parse_post_data(post_data, "dns2", config.dns2, sizeof(config.dns2));
    
    if (cgi_set_lan_config(&config) == 0) {
        cgi_response_json(CGI_OK, "设置保存成功", NULL);
    } else {
        cgi_response_error(CGI_ERROR, "设置保存失败");
    }
}

static void handle_apn_list_request(void)
{
    ApnConfig apns[MAX_APN_COUNT];
    int count;
    
    cgi_get_apn_list(apns, &count);
    
    char json[MAX_BUFFER_SIZE * 2] = "[";
    for (int i = 0; i < count; i++) {
        char apn_json[512];
        snprintf(apn_json, sizeof(apn_json),
            "{\"id\":%d,\"name\":\"%s\",\"username\":\"%s\",\"password\":\"%s\","
            "\"auth_type\":%d,\"bearer_type\":%d,\"is_default\":%s,\"is_active\":%s}",
            apns[i].id, apns[i].name, apns[i].username, apns[i].password,
            apns[i].auth_type, apns[i].bearer_type,
            apns[i].is_default ? "true" : "false",
            apns[i].is_active ? "true" : "false");
        
        strcat(json, apn_json);
        if (i < count - 1) strcat(json, ",");
    }
    strcat(json, "]");
    
    cgi_response_json(CGI_OK, "success", json);
}

static void handle_apn_add_request(char *post_data)
{
    ApnConfig apn;
    memset(&apn, 0, sizeof(apn));
    
    char id_str[16];
    if (parse_post_data(post_data, "id", id_str, sizeof(id_str)) == 0) {
        apn.id = atoi(id_str);
    }
    
    parse_post_data(post_data, "name", apn.name, sizeof(apn.name));
    parse_post_data(post_data, "username", apn.username, sizeof(apn.username));
    parse_post_data(post_data, "password", apn.password, sizeof(apn.password));
    
    char auth_str[16];
    if (parse_post_data(post_data, "auth_type", auth_str, sizeof(auth_str)) == 0) {
        apn.auth_type = atoi(auth_str);
    }
    
    char bearer_str[16];
    if (parse_post_data(post_data, "bearer_type", bearer_str, sizeof(bearer_str)) == 0) {
        apn.bearer_type = atoi(bearer_str);
    }
    
    char active_str[16];
    if (parse_post_data(post_data, "is_active", active_str, sizeof(active_str)) == 0) {
        apn.is_active = (strcmp(active_str, "true") == 0);
    }
    
    if (cgi_add_apn(&apn) == 0) {
        cgi_response_json(CGI_OK, "APN添加成功", NULL);
    } else {
        cgi_response_error(CGI_ERROR, "APN添加失败");
    }
}

static void handle_apn_edit_request(char *post_data)
{
    ApnConfig apn;
    memset(&apn, 0, sizeof(apn));
    
    char id_str[16];
    if (parse_post_data(post_data, "id", id_str, sizeof(id_str)) == 0) {
        apn.id = atoi(id_str);
    }
    
    parse_post_data(post_data, "name", apn.name, sizeof(apn.name));
    parse_post_data(post_data, "username", apn.username, sizeof(apn.username));
    parse_post_data(post_data, "password", apn.password, sizeof(apn.password));
    
    char auth_str[16];
    if (parse_post_data(post_data, "auth_type", auth_str, sizeof(auth_str)) == 0) {
        apn.auth_type = atoi(auth_str);
    }
    
    char bearer_str[16];
    if (parse_post_data(post_data, "bearer_type", bearer_str, sizeof(bearer_str)) == 0) {
        apn.bearer_type = atoi(bearer_str);
    }
    
    char active_str[16];
    if (parse_post_data(post_data, "is_active", active_str, sizeof(active_str)) == 0) {
        apn.is_active = (strcmp(active_str, "true") == 0);
    }
    
    if (cgi_edit_apn(&apn) == 0) {
        cgi_response_json(CGI_OK, "APN更新成功", NULL);
    } else {
        cgi_response_error(CGI_ERROR, "APN更新失败");
    }
}

static void handle_apn_delete_request(char *post_data)
{
    char id_str[16];
    if (parse_post_data(post_data, "id", id_str, sizeof(id_str)) != 0) {
        cgi_response_error(CGI_ERROR, "无效的APN ID");
        return;
    }
    
    int apn_id = atoi(id_str);
    
    if (cgi_delete_apn(apn_id) == 0) {
        cgi_response_json(CGI_OK, "APN删除成功", NULL);
    } else {
        cgi_response_error(CGI_ERROR, "APN删除失败");
    }
}

static void handle_apn_activate_request(char *post_data)
{
    char id_str[16];
    if (parse_post_data(post_data, "id", id_str, sizeof(id_str)) != 0) {
        cgi_response_error(CGI_ERROR, "无效的APN ID");
        return;
    }
    
    int apn_id = atoi(id_str);
    
    if (cgi_activate_apn(apn_id) == 0) {
        cgi_response_json(CGI_OK, "APN激活成功", NULL);
    } else {
        cgi_response_error(CGI_ERROR, "APN激活失败");
    }
}

static void handle_wlan_ap_get_request(void)
{
    WlanApConfig config;
    cgi_get_wlan_ap_config(&config);
    
    char json[MAX_BUFFER_SIZE];
    snprintf(json, sizeof(json),
        "{\"enabled_2g4\":%s,\"enabled_5g\":%s,\"ssid_2g4\":\"%s\","
        "\"ssid_5g\":\"%s\",\"channel_2g4\":%d,\"channel_5g\":%d}",
        config.enabled_2g4 ? "true" : "false",
        config.enabled_5g ? "true" : "false",
        config.ssid_2g4, config.ssid_5g,
        config.channel_2g4, config.channel_5g);
    
    cgi_response_json(CGI_OK, "success", json);
}

static void handle_wlan_ap_set_request(char *post_data)
{
    WlanApConfig config;
    memset(&config, 0, sizeof(config));
    
    char str[16];
    
    if (parse_post_data(post_data, "enabled_2g4", str, sizeof(str)) == 0) {
        config.enabled_2g4 = (strcmp(str, "true") == 0);
    }
    if (parse_post_data(post_data, "enabled_5g", str, sizeof(str)) == 0) {
        config.enabled_5g = (strcmp(str, "true") == 0);
    }
    
    parse_post_data(post_data, "ssid_2g4", config.ssid_2g4, sizeof(config.ssid_2g4));
    parse_post_data(post_data, "password_2g4", config.password_2g4, sizeof(config.password_2g4));
    parse_post_data(post_data, "ssid_5g", config.ssid_5g, sizeof(config.ssid_5g));
    parse_post_data(post_data, "password_5g", config.password_5g, sizeof(config.password_5g));
    
    char ch_str[16];
    if (parse_post_data(post_data, "channel_2g4", ch_str, sizeof(ch_str)) == 0) {
        config.channel_2g4 = atoi(ch_str);
    }
    if (parse_post_data(post_data, "channel_5g", ch_str, sizeof(ch_str)) == 0) {
        config.channel_5g = atoi(ch_str);
    }
    
    if (cgi_set_wlan_ap_config(&config) == 0) {
        cgi_response_json(CGI_OK, "WLAN设置已保存", NULL);
    } else {
        cgi_response_error(CGI_ERROR, "WLAN设置保存失败");
    }
}

static void handle_firewall_get_request(void)
{
    FirewallConfig config;
    cgi_get_firewall_config(&config);
    
    char json[512];
    snprintf(json, sizeof(json),
        "{\"enabled\":%s,\"spi_enabled\":%s,\"dos_enabled\":%s}",
        config.enabled ? "true" : "false",
        config.spi_enabled ? "true" : "false",
        config.dos_enabled ? "true" : "false");
    
    cgi_response_json(CGI_OK, "success", json);
}

static void handle_firewall_set_request(char *post_data)
{
    FirewallConfig config;
    memset(&config, 0, sizeof(config));
    
    char str[16];
    
    if (parse_post_data(post_data, "enabled", str, sizeof(str)) == 0) {
        config.enabled = (strcmp(str, "true") == 0);
    }
    if (parse_post_data(post_data, "spi_enabled", str, sizeof(str)) == 0) {
        config.spi_enabled = (strcmp(str, "true") == 0);
    }
    if (parse_post_data(post_data, "dos_enabled", str, sizeof(str)) == 0) {
        config.dos_enabled = (strcmp(str, "true") == 0);
    }
    
    if (cgi_set_firewall_config(&config) == 0) {
        cgi_response_json(CGI_OK, "防火墙设置已保存", NULL);
    } else {
        cgi_response_error(CGI_ERROR, "防火墙设置保存失败");
    }
}

static void handle_reboot_request(void)
{
    if (cgi_reboot() == 0) {
        cgi_response_json(CGI_OK, "设备正在重启", NULL);
    } else {
        cgi_response_error(CGI_ERROR, "重启命令执行失败");
    }
}

static void handle_factory_reset_request(void)
{
    if (cgi_factory_reset() == 0) {
        cgi_response_json(CGI_OK, "正在恢复出厂设置", NULL);
    } else {
        cgi_response_error(CGI_ERROR, "恢复出厂设置失败");
    }
}

int main(int argc, char *argv[])
{
    char *method = getenv("REQUEST_METHOD");
    char *query = getenv("QUERY_STRING");
    char post_data[MAX_BUFFER_SIZE * 2] = {0};
    
    if (!method) {
        cgi_response_error(CGI_ERROR, "No request method");
        return 0;
    }
    
    if (strcmp(method, "POST") == 0) {
        char *content_length = getenv("CONTENT_LENGTH");
        if (content_length) {
            int len = atoi(content_length);
            if (len > 0 && len < sizeof(post_data)) {
                fread(post_data, 1, len, stdin);
                post_data[len] = '\0';
            }
        }
    }
    
    char path[256] = {0};
    if (query && strlen(query) > 0) {
        sscanf(query, "action=%s", path);
    } else {
        strcpy(path, "status");
    }
    
    if (strstr(path, "status") != NULL) {
        handle_status_request();
    }
    else if (strstr(path, "lan_get") != NULL) {
        handle_lan_get_request();
    }
    else if (strstr(path, "lan_set") != NULL) {
        handle_lan_set_request(post_data);
    }
    else if (strstr(path, "apn_list") != NULL) {
        handle_apn_list_request();
    }
    else if (strstr(path, "apn_add") != NULL) {
        handle_apn_add_request(post_data);
    }
    else if (strstr(path, "apn_edit") != NULL) {
        handle_apn_edit_request(post_data);
    }
    else if (strstr(path, "apn_delete") != NULL) {
        handle_apn_delete_request(post_data);
    }
    else if (strstr(path, "apn_activate") != NULL) {
        handle_apn_activate_request(post_data);
    }
    else if (strstr(path, "wlan_ap_get") != NULL) {
        handle_wlan_ap_get_request();
    }
    else if (strstr(path, "wlan_ap_set") != NULL) {
        handle_wlan_ap_set_request(post_data);
    }
    else if (strstr(path, "firewall_get") != NULL) {
        handle_firewall_get_request();
    }
    else if (strstr(path, "firewall_set") != NULL) {
        handle_firewall_set_request(post_data);
    }
    else if (strstr(path, "reboot") != NULL) {
        handle_reboot_request();
    }
    else if (strstr(path, "factory_reset") != NULL) {
        handle_factory_reset_request();
    }
    else {
        cgi_response_error(CGI_NOT_FOUND, "Unknown action");
    }
    
    return 0;
}
