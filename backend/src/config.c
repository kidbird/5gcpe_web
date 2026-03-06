#include "cpe.h"

#define CONFIG_FILE     "/etc/cpe/config.json"

static LanConfig lan_config;
static CellularConfig cellular_config;
static ApnConfig apn_list[MAX_APN_COUNT];
static int apn_count = 0;
static WlanApConfig wlan_ap_config;
static WlanStaConfig wlan_sta_config;
static FirewallConfig firewall_config;
static VpnConfig vpn_configs[MAX_VPN_COUNT];
static IotConfig iot_config;
static SystemConfig system_config;

static char* trim(char *str)
{
    char *end;
    
    while(isspace((unsigned char)*str)) str++;
    
    if(*str == 0) return str;
    
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;
    
    end[1] = '\0';
    
    return str;
}

static int parse_json_object(const char *json, const char *key, char *value, int len)
{
    char key_str[128];
    snprintf(key_str, sizeof(key_str), "\"%s\"", key);
    
    char *p = strstr(json, key_str);
    if (!p) return -1;
    
    p = strchr(p, ':');
    if (!p) return -1;
    p++;
    
    while (*p && (*p == ' ' || *p == '\t' || *p == '"')) p++;
    
    int i = 0;
    while (*p && *p != '"' && *p != ',' && *p != '}' && i < len - 1) {
        value[i++] = *p++;
    }
    value[i] = '\0';
    
    return 0;
}

static int parse_json_bool(const char *json, const char *key, bool *value)
{
    char key_str[128];
    snprintf(key_str, sizeof(key_str), "\"%s\"", key);
    
    char *p = strstr(json, key_str);
    if (!p) return -1;
    
    p = strchr(p, ':');
    if (!p) return -1;
    p++;
    
    while (*p && (*p == ' ' || *p == '\t')) p++;
    
    if (strncmp(p, "true", 4) == 0) {
        *value = true;
        return 0;
    } else if (strncmp(p, "false", 5) == 0) {
        *value = false;
        return 0;
    }
    
    return -1;
}

static int parse_json_int(const char *json, const char *key, int *value)
{
    char key_str[128];
    snprintf(key_str, sizeof(key_str), "\"%s\"", key);

    char *p = strstr(json, key_str);
    if (!p) return -1;

    p = strchr(p, ':');
    if (!p) return -1;
    p++;

    while (*p && (*p == ' ' || *p == '\t')) p++;

    *value = atoi(p);

    return 0;
}

int config_load(const char *config_file)
{
    FILE *fp = fopen(config_file ? config_file : CONFIG_FILE, "r");
    if (!fp) {
        strcpy(lan_config.ip, "192.168.1.1");
        strcpy(lan_config.netmask, "255.255.255.0");
        lan_config.dhcp_enabled = true;
        strcpy(lan_config.dhcp_start, "192.168.1.100");
        strcpy(lan_config.dhcp_end, "192.168.1.200");
        lan_config.dhcp_lease = 86400;
        
        strcpy(wlan_ap_config.ssid_2g4, "CPE_5G_24G");
        strcpy(wlan_ap_config.ssid_5g, "CPE_5G_5G");
        wlan_ap_config.enabled_2g4 = true;
        wlan_ap_config.enabled_5g = true;
        wlan_ap_config.encryption = 4;
        
        firewall_config.enabled = true;
        firewall_config.spi_enabled = true;
        firewall_config.dos_enabled = true;
        
        strcpy(system_config.device_name, "5G CPE");
        strcpy(system_config.timezone, "Asia/Shanghai");
        strcpy(system_config.admin_user, "admin");
        strcpy(system_config.admin_pass, "admin");
        system_config.web_port = 80;
        
        return 0;
    }
    
    char buffer[MAX_BUFFER_SIZE * 4];
    int offset = 0;
    int n;
    
    while ((n = fread(buffer + offset, 1, sizeof(buffer) - offset - 1, fp)) > 0) {
        offset += n;
    }
    buffer[offset] = '\0';
    fclose(fp);
    
    parse_json_object(buffer, "lan_ip", lan_config.ip, sizeof(lan_config.ip));
    parse_json_object(buffer, "lan_netmask", lan_config.netmask, sizeof(lan_config.netmask));
    parse_json_bool(buffer, "dhcp_enabled", &lan_config.dhcp_enabled);
    parse_json_object(buffer, "dhcp_start", lan_config.dhcp_start, sizeof(lan_config.dhcp_start));
    parse_json_object(buffer, "dhcp_end", lan_config.dhcp_end, sizeof(lan_config.dhcp_end));
    if (parse_json_int(buffer, "dhcp_lease", &lan_config.dhcp_lease) != 0) {
        lan_config.dhcp_lease = 86400;
    }
    parse_json_object(buffer, "dns1", lan_config.dns1, sizeof(lan_config.dns1));
    parse_json_object(buffer, "dns2", lan_config.dns2, sizeof(lan_config.dns2));
    
    parse_json_int(buffer, "network_mode", &cellular_config.network_mode);
    parse_json_object(buffer, "bands_5g", cellular_config.bands_5g, sizeof(cellular_config.bands_5g));
    parse_json_object(buffer, "bands_lte", cellular_config.bands_lte, sizeof(cellular_config.bands_lte));
    parse_json_bool(buffer, "airplane_mode", &cellular_config.airplane_mode);
    parse_json_bool(buffer, "data_roaming", &cellular_config.data_roaming);
    parse_json_bool(buffer, "hw_accel", &cellular_config.hw_accel);
    parse_json_bool(buffer, "ims_enabled", &cellular_config.ims_enabled);
    
    parse_json_bool(buffer, "wlan_2g4_enabled", &wlan_ap_config.enabled_2g4);
    parse_json_bool(buffer, "wlan_5g_enabled", &wlan_ap_config.enabled_5g);
    parse_json_object(buffer, "ssid_2g4", wlan_ap_config.ssid_2g4, sizeof(wlan_ap_config.ssid_2g4));
    parse_json_object(buffer, "ssid_5g", wlan_ap_config.ssid_5g, sizeof(wlan_ap_config.ssid_5g));
    parse_json_object(buffer, "password_2g4", wlan_ap_config.password_2g4, sizeof(wlan_ap_config.password_2g4));
    parse_json_object(buffer, "password_5g", wlan_ap_config.password_5g, sizeof(wlan_ap_config.password_5g));
    parse_json_int(buffer, "channel_2g4", &wlan_ap_config.channel_2g4);
    parse_json_int(buffer, "channel_5g", &wlan_ap_config.channel_5g);
    parse_json_int(buffer, "encryption", &wlan_ap_config.encryption);
    
    parse_json_bool(buffer, "sta_enabled", &wlan_sta_config.sta_enabled);
    parse_json_bool(buffer, "nat_enabled", &wlan_sta_config.nat_enabled);
    parse_json_object(buffer, "wan_mac", wlan_sta_config.wan_mac, sizeof(wlan_sta_config.wan_mac));
    
    parse_json_bool(buffer, "firewall_enabled", &firewall_config.enabled);
    parse_json_bool(buffer, "spi_enabled", &firewall_config.spi_enabled);
    parse_json_bool(buffer, "dos_enabled", &firewall_config.dos_enabled);
    parse_json_bool(buffer, "dmz_enabled", &firewall_config.dmz_enabled);
    parse_json_object(buffer, "dmz_ip", firewall_config.dmz_ip, sizeof(firewall_config.dmz_ip));
    
    parse_json_object(buffer, "device_name", system_config.device_name, sizeof(system_config.device_name));
    parse_json_object(buffer, "timezone", system_config.timezone, sizeof(system_config.timezone));
    parse_json_object(buffer, "ntp_server", system_config.ntp_server, sizeof(system_config.ntp_server));
    parse_json_int(buffer, "web_port", &system_config.web_port);
    parse_json_object(buffer, "admin_user", system_config.admin_user, sizeof(system_config.admin_user));
    parse_json_object(buffer, "admin_pass", system_config.admin_pass, sizeof(system_config.admin_pass));
    
    parse_json_bool(buffer, "iot_enabled", &iot_config.enabled);
    parse_json_int(buffer, "iot_protocol", &iot_config.protocol);
    parse_json_object(buffer, "iot_server", iot_config.server, sizeof(iot_config.server));
    parse_json_int(buffer, "iot_port", &iot_config.port);
    
    return 0;
}

int config_save(const char *config_file)
{
    FILE *fp = fopen(config_file ? config_file : CONFIG_FILE, "w");
    if (!fp) {
        return -1;
    }
    
    fprintf(fp, "{\n");
    
    fprintf(fp, "  \"lan_ip\":\"%s\",\n", lan_config.ip);
    fprintf(fp, "  \"lan_netmask\":\"%s\",\n", lan_config.netmask);
    fprintf(fp, "  \"dhcp_enabled\":%s,\n", lan_config.dhcp_enabled ? "true" : "false");
    fprintf(fp, "  \"dhcp_start\":\"%s\",\n", lan_config.dhcp_start);
    fprintf(fp, "  \"dhcp_end\":\"%s\",\n", lan_config.dhcp_end);
    fprintf(fp, "  \"dhcp_lease\":%d,\n", lan_config.dhcp_lease);
    fprintf(fp, "  \"dns1\":\"%s\",\n", lan_config.dns1);
    fprintf(fp, "  \"dns2\":\"%s\",\n", lan_config.dns2);
    
    fprintf(fp, "  \"network_mode\":%d,\n", cellular_config.network_mode);
    fprintf(fp, "  \"bands_5g\":\"%s\",\n", cellular_config.bands_5g);
    fprintf(fp, "  \"bands_lte\":\"%s\",\n", cellular_config.bands_lte);
    fprintf(fp, "  \"airplane_mode\":%s,\n", cellular_config.airplane_mode ? "true" : "false");
    fprintf(fp, "  \"data_roaming\":%s,\n", cellular_config.data_roaming ? "true" : "false");
    fprintf(fp, "  \"hw_accel\":%s,\n", cellular_config.hw_accel ? "true" : "false");
    fprintf(fp, "  \"ims_enabled\":%s,\n", cellular_config.ims_enabled ? "true" : "false");
    
    fprintf(fp, "  \"wlan_2g4_enabled\":%s,\n", wlan_ap_config.enabled_2g4 ? "true" : "false");
    fprintf(fp, "  \"wlan_5g_enabled\":%s,\n", wlan_ap_config.enabled_5g ? "true" : "false");
    fprintf(fp, "  \"ssid_2g4\":\"%s\",\n", wlan_ap_config.ssid_2g4);
    fprintf(fp, "  \"ssid_5g\":\"%s\",\n", wlan_ap_config.ssid_5g);
    fprintf(fp, "  \"password_2g4\":\"%s\",\n", wlan_ap_config.password_2g4);
    fprintf(fp, "  \"password_5g\":\"%s\",\n", wlan_ap_config.password_5g);
    fprintf(fp, "  \"channel_2g4\":%d,\n", wlan_ap_config.channel_2g4);
    fprintf(fp, "  \"channel_5g\":%d,\n", wlan_ap_config.channel_5g);
    fprintf(fp, "  \"encryption\":%d,\n", wlan_ap_config.encryption);
    
    fprintf(fp, "  \"sta_enabled\":%s,\n", wlan_sta_config.sta_enabled ? "true" : "false");
    fprintf(fp, "  \"target_ssid\":\"%s\",\n", wlan_sta_config.target_ssid);
    fprintf(fp, "  \"target_password\":\"%s\",\n", wlan_sta_config.target_password);
    fprintf(fp, "  \"wan_type\":%d,\n", wlan_sta_config.wan_type);
    fprintf(fp, "  \"wan_mac\":\"%s\",\n", wlan_sta_config.wan_mac);
    fprintf(fp, "  \"nat_enabled\":%s,\n", wlan_sta_config.nat_enabled ? "true" : "false");
    
    fprintf(fp, "  \"firewall_enabled\":%s,\n", firewall_config.enabled ? "true" : "false");
    fprintf(fp, "  \"dmz_enabled\":%s,\n", firewall_config.dmz_enabled ? "true" : "false");
    fprintf(fp, "  \"dmz_ip\":\"%s\",\n", firewall_config.dmz_ip);
    fprintf(fp, "  \"spi_enabled\":%s,\n", firewall_config.spi_enabled ? "true" : "false");
    fprintf(fp, "  \"dos_enabled\":%s,\n", firewall_config.dos_enabled ? "true" : "false");
    
    fprintf(fp, "  \"iot_enabled\":%s,\n", iot_config.enabled ? "true" : "false");
    fprintf(fp, "  \"iot_protocol\":%d,\n", iot_config.protocol);
    fprintf(fp, "  \"iot_server\":\"%s\",\n", iot_config.server);
    fprintf(fp, "  \"iot_port\":%d,\n", iot_config.port);
    
    fprintf(fp, "  \"device_name\":\"%s\",\n", system_config.device_name);
    fprintf(fp, "  \"timezone\":\"%s\",\n", system_config.timezone);
    fprintf(fp, "  \"ntp_server\":\"%s\",\n", system_config.ntp_server);
    fprintf(fp, "  \"web_port\":%d,\n", system_config.web_port);
    fprintf(fp, "  \"admin_user\":\"%s\",\n", system_config.admin_user);
    fprintf(fp, "  \"admin_pass\":\"%s\"\n", system_config.admin_pass);
    
    fprintf(fp, "}\n");
    
    fclose(fp);
    return 0;
}

int config_get_lan(LanConfig *config)
{
    memcpy(config, &lan_config, sizeof(LanConfig));
    return 0;
}

int config_set_lan(LanConfig *config)
{
    memcpy(&lan_config, config, sizeof(LanConfig));
    return config_save(NULL);
}

int config_get_cellular(CellularConfig *config)
{
    memcpy(config, &cellular_config, sizeof(CellularConfig));
    return 0;
}

int config_set_cellular(CellularConfig *config)
{
    memcpy(&cellular_config, config, sizeof(CellularConfig));
    return config_save(NULL);
}

int config_get_apn_list(ApnConfig *apns, int *count)
{
    *count = apn_count;
    if (apn_count > 0) {
        memcpy(apns, apn_list, sizeof(ApnConfig) * apn_count);
    }
    return 0;
}

int config_set_apn(ApnConfig *apn)
{
    for (int i = 0; i < apn_count; i++) {
        if (apn_list[i].id == apn->id) {
            memcpy(&apn_list[i], apn, sizeof(ApnConfig));
            return config_save(NULL);
        }
    }
    
    if (apn_count >= MAX_APN_COUNT) {
        return -1;
    }
    
    memcpy(&apn_list[apn_count++], apn, sizeof(ApnConfig));
    return config_save(NULL);
}

int config_delete_apn(int apn_id)
{
    for (int i = 0; i < apn_count; i++) {
        if (apn_list[i].id == apn_id) {
            for (int j = i; j < apn_count - 1; j++) {
                apn_list[j] = apn_list[j + 1];
            }
            apn_count--;
            return config_save(NULL);
        }
    }
    return -1;
}

int config_get_wlan_ap(WlanApConfig *config)
{
    memcpy(config, &wlan_ap_config, sizeof(WlanApConfig));
    return 0;
}

int config_set_wlan_ap(WlanApConfig *config)
{
    memcpy(&wlan_ap_config, config, sizeof(WlanApConfig));
    return config_save(NULL);
}

int config_get_wlan_sta(WlanStaConfig *config)
{
    memcpy(config, &wlan_sta_config, sizeof(WlanStaConfig));
    return 0;
}

int config_set_wlan_sta(WlanStaConfig *config)
{
    memcpy(&wlan_sta_config, config, sizeof(WlanStaConfig));
    return config_save(NULL);
}

int config_get_firewall(FirewallConfig *config)
{
    memcpy(config, &firewall_config, sizeof(FirewallConfig));
    return 0;
}

int config_set_firewall(FirewallConfig *config)
{
    memcpy(&firewall_config, config, sizeof(FirewallConfig));
    return config_save(NULL);
}

int config_get_vpn(int type, VpnConfig *config)
{
    if (type >= 0 && type < MAX_VPN_COUNT) {
        memcpy(config, &vpn_configs[type], sizeof(VpnConfig));
        return 0;
    }
    return -1;
}

int config_set_vpn(VpnConfig *config)
{
    if (config->type >= 0 && config->type < MAX_VPN_COUNT) {
        memcpy(&vpn_configs[config->type], config, sizeof(VpnConfig));
        return config_save(NULL);
    }
    return -1;
}

int config_get_iot(IotConfig *config)
{
    memcpy(config, &iot_config, sizeof(IotConfig));
    return 0;
}

int config_set_iot(IotConfig *config)
{
    memcpy(&iot_config, config, sizeof(IotConfig));
    return config_save(NULL);
}

int config_get_system(SystemConfig *config)
{
    memcpy(config, &system_config, sizeof(SystemConfig));
    return 0;
}

int config_set_system(SystemConfig *config)
{
    memcpy(&system_config, config, sizeof(SystemConfig));
    return config_save(NULL);
}

int config_add_port_forward(PortForwardRule *rule)
{
    if (firewall_config.port_forward_count >= MAX_PORT_FORWARD) {
        return -1;
    }
    memcpy(&firewall_config.port_forwards[firewall_config.port_forward_count++], 
           rule, sizeof(PortForwardRule));
    return config_save(NULL);
}

int config_edit_port_forward(PortForwardRule *rule)
{
    for (int i = 0; i < firewall_config.port_forward_count; i++) {
        if (firewall_config.port_forwards[i].id == rule->id) {
            memcpy(&firewall_config.port_forwards[i], rule, sizeof(PortForwardRule));
            return config_save(NULL);
        }
    }
    return -1;
}

int config_delete_port_forward(int rule_id)
{
    for (int i = 0; i < firewall_config.port_forward_count; i++) {
        if (firewall_config.port_forwards[i].id == rule_id) {
            for (int j = i; j < firewall_config.port_forward_count - 1; j++) {
                firewall_config.port_forwards[j] = firewall_config.port_forwards[j + 1];
            }
            firewall_config.port_forward_count--;
            return config_save(NULL);
        }
    }
    return -1;
}

char* json_escape_string(const char *str)
{
    if (!str) return NULL;
    
    int len = strlen(str);
    char *escaped = malloc(len * 2 + 1);
    if (!escaped) return NULL;
    
    int j = 0;
    for (int i = 0; i < len; i++) {
        switch (str[i]) {
            case '"':  escaped[j++] = '\\'; escaped[j++] = '"'; break;
            case '\\': escaped[j++] = '\\'; escaped[j++] = '\\'; break;
            case '\n': escaped[j++] = '\\'; escaped[j++] = 'n'; break;
            case '\r': escaped[j++] = '\\'; escaped[j++] = 'r'; break;
            case '\t': escaped[j++] = '\\'; escaped[j++] = 't'; break;
            default:   escaped[j++] = str[i]; break;
        }
    }
    escaped[j] = '\0';
    
    return escaped;
}

void json_append_string(char *json, int len, const char *key, const char *value)
{
    char buf[MAX_BUFFER_SIZE];
    snprintf(buf, sizeof(buf), "\"%s\":\"%s\"", key, value);
    strcat(json, buf);
}

void json_append_int(char *json, int len, const char *key, int value)
{
    char buf[MAX_BUFFER_SIZE];
    snprintf(buf, sizeof(buf), "\"%s\":%d", key, value);
    strcat(json, buf);
}

void json_append_bool(char *json, int len, const char *key, bool value)
{
    char buf[MAX_BUFFER_SIZE];
    snprintf(buf, sizeof(buf), "\"%s\":%s", key, value ? "true" : "false");
    strcat(json, buf);
}

int json_parse_string(const char *json, const char *key, char *value, int len)
{
    return parse_json_object(json, key, value, len);
}

int json_parse_int(const char *json, const char *key, int *value)
{
    return parse_json_int(json, key, value);
}

int json_parse_bool(const char *json, const char *key, bool *value)
{
    return parse_json_bool(json, key, value);
}

int json_parse_array(const char *json, const char *key, char *array, int len)
{
    char key_str[128];
    snprintf(key_str, sizeof(key_str), "\"%s\"", key);
    
    const char *p = strstr(json, key_str);
    if (!p) return -1;
    
    p = strchr(p, ':');
    if (!p) return -1;
    p++;
    
    while (*p && (*p == ' ' || *p == '\t' || *p == '\n')) p++;
    
    if (*p != '[') return -1;
    
    const char *end = strchr(p, ']');
    if (!end) return -1;
    
    int array_len = end - p + 1;
    if (array_len >= len) array_len = len - 1;
    
    strncpy(array, p, array_len);
    array[array_len] = '\0';
    
    return 0;
}
