#include "cpe.h"

#define CONFIG_FILE     "/etc/cpe/config.json"

static LanConfig lan_config;
static ApnConfig apn_list[MAX_APN_COUNT];
static int apn_count = 0;
static WlanApConfig wlan_ap_config;
static WlanStaConfig wlan_sta_config;
static FirewallConfig firewall_config;

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

static void generate_json_string(char *json, int len, const char *key, const char *value)
{
    char buf[MAX_BUFFER_SIZE];
    snprintf(buf, sizeof(buf), "\"%s\":\"%s\"", key, value);
    strcat(json, buf);
}

static void generate_json_bool(char *json, int len, const char *key, bool value)
{
    char buf[MAX_BUFFER_SIZE];
    snprintf(buf, sizeof(buf), "\"%s\":%s", key, value ? "true" : "false");
    strcat(json, buf);
}

static void generate_json_int(char *json, int len, const char *key, int value)
{
    char buf[MAX_BUFFER_SIZE];
    snprintf(buf, sizeof(buf), "\"%s\":%d", key, value);
    strcat(json, buf);
}

int config_load(const char *config_file)
{
    FILE *fp = fopen(config_file ? config_file : CONFIG_FILE, "r");
    if (!fp) {
        return -1;
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
    
    parse_json_bool(buffer, "wlan_2g4_enabled", &wlan_ap_config.enabled_2g4);
    parse_json_bool(buffer, "wlan_5g_enabled", &wlan_ap_config.enabled_5g);
    parse_json_object(buffer, "ssid_2g4", wlan_ap_config.ssid_2g4, sizeof(wlan_ap_config.ssid_2g4));
    parse_json_object(buffer, "ssid_5g", wlan_ap_config.ssid_5g, sizeof(wlan_ap_config.ssid_5g));
    parse_json_object(buffer, "password_2g4", wlan_ap_config.password_2g4, sizeof(wlan_ap_config.password_2g4));
    parse_json_object(buffer, "password_5g", wlan_ap_config.password_5g, sizeof(wlan_ap_config.password_5g));
    
    parse_json_bool(buffer, "sta_enabled", &wlan_sta_config.sta_enabled);
    parse_json_bool(buffer, "nat_enabled", &wlan_sta_config.nat_enabled);
    parse_json_object(buffer, "wan_mac", wlan_sta_config.wan_mac, sizeof(wlan_sta_config.wan_mac));
    
    parse_json_bool(buffer, "firewall_enabled", &firewall_config.enabled);
    parse_json_bool(buffer, "spi_enabled", &firewall_config.spi_enabled);
    parse_json_bool(buffer, "dos_enabled", &firewall_config.dos_enabled);
    
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
    fprintf(fp, "  \"dns1\":\"%s\",\n", lan_config.dns1);
    fprintf(fp, "  \"dns2\":\"%s\",\n", lan_config.dns2);
    
    fprintf(fp, "  \"wlan_2g4_enabled\":%s,\n", wlan_ap_config.enabled_2g4 ? "true" : "false");
    fprintf(fp, "  \"wlan_5g_enabled\":%s,\n", wlan_ap_config.enabled_5g ? "true" : "false");
    fprintf(fp, "  \"ssid_2g4\":\"%s\",\n", wlan_ap_config.ssid_2g4);
    fprintf(fp, "  \"ssid_5g\":\"%s\",\n", wlan_ap_config.ssid_5g);
    fprintf(fp, "  \"password_2g4\":\"%s\",\n", wlan_ap_config.password_2g4);
    fprintf(fp, "  \"password_5g\":\"%s\",\n", wlan_ap_config.password_5g);
    fprintf(fp, "  \"channel_2g4\":%d,\n", wlan_ap_config.channel_2g4);
    fprintf(fp, "  \"channel_5g\":%d,\n", wlan_ap_config.channel_5g);
    
    fprintf(fp, "  \"sta_enabled\":%s,\n", wlan_sta_config.sta_enabled ? "true" : "false");
    fprintf(fp, "  \"target_ssid\":\"%s\",\n", wlan_sta_config.target_ssid);
    fprintf(fp, "  \"target_password\":\"%s\",\n", wlan_sta_config.target_password);
    fprintf(fp, "  \"wan_type\":%d,\n", wlan_sta_config.wan_type);
    fprintf(fp, "  \"wan_mac\":\"%s\",\n", wlan_sta_config.wan_mac);
    fprintf(fp, "  \"nat_enabled\":%s,\n", wlan_sta_config.nat_enabled ? "true" : "false");
    
    fprintf(fp, "  \"firewall_enabled\":%s,\n", firewall_config.enabled ? "true" : "false");
    fprintf(fp, "  \"spi_enabled\":%s,\n", firewall_config.spi_enabled ? "true" : "false");
    fprintf(fp, "  \"dos_enabled\":%s\n", firewall_config.dos_enabled ? "true" : "false");
    
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

int config_get_apn_list(ApnConfig *apns, int *count)
{
    *count = apn_count;
    memcpy(apns, apn_list, sizeof(ApnConfig) * apn_count);
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
