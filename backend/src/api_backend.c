#include "cpe.h"
#include <sys/stat.h>
#include <sys/sysinfo.h>

static time_t system_start_time = 0;
static uint64_t total_tx_bytes = 0;
static uint64_t total_rx_bytes = 0;

void cpe_control_init(void)
{
    system_start_time = time(NULL);
    config_load(NULL);
}

int cgi_get_device_status(DeviceStatus *status)
{
    if (!status) return -1;
    
    memset(status, 0, sizeof(DeviceStatus));
    
    strcpy(status->device_model, "CPE-5G01");
    strcpy(status->firmware_version, "V2.1.5");
    
    at_get_imei(status->imei, sizeof(status->imei));
    at_get_iccid(status->iccid, sizeof(status->iccid));
    at_get_imsi(status->imsi, sizeof(status->imsi));
    
    FILE *fp = fopen("/sys/class/net/eth0/address", "r");
    if (fp) {
        fgets(status->lan_mac, sizeof(status->lan_mac), fp);
        status->lan_mac[strcspn(status->lan_mac, "\n")] = 0;
        fclose(fp);
    }
    
    fp = fopen("/sys/class/net/wwan0/address", "r");
    if (fp) {
        fgets(status->wan_ip, sizeof(status->wan_ip), fp);
        status->wan_ip[strcspn(status->wan_ip, "\n")] = 0;
        fclose(fp);
    }
    
    if (system_start_time > 0) {
        time_t now = time(NULL);
        int uptime_sec = (int)(now - system_start_time);
        status->uptime_seconds = uptime_sec;
        
        int days = uptime_sec / 86400;
        int hours = (uptime_sec % 86400) / 3600;
        int mins = (uptime_sec % 3600) / 60;
        
        snprintf(status->uptime, sizeof(status->uptime), 
                 "%d天 %d时 %d分", days, hours, mins);
    }
    
    status->signal_strength = at_get_signal_strength();
    
    at_get_operator(status->operator_name, sizeof(status->operator_name));
    
    strcpy(status->network_type, "5G NR");
    strcpy(status->network_mode_str, "SA");
    
    status->rsrp = -72;
    status->rsrq = -10;
    status->sinr = 15;
    strcpy(status->band, "n78");
    status->earfcn = 627264;
    status->pci = 120;
    strcpy(status->cell_id, "0x12345678");
    status->temperature = 45;
    strcpy(status->sim_status, "ready");
    
    return 0;
}

int cgi_get_traffic_stats(TrafficStats *stats)
{
    if (!stats) return -1;
    
    memset(stats, 0, sizeof(TrafficStats));
    
    FILE *fp = fopen("/proc/net/dev", "r");
    if (fp) {
        char line[256];
        while (fgets(line, sizeof(line), fp)) {
            if (strstr(line, "wwan0:")) {
                unsigned long rx_bytes, tx_bytes;
                sscanf(line, "wwan0: %lu %*d %*d %*d %*d %*d %*d %*d %lu",
                       &rx_bytes, &tx_bytes);
                stats->total_rx = rx_bytes;
                stats->total_tx = tx_bytes;
                
                double rx_mb = rx_bytes / (1024.0 * 1024.0);
                double tx_mb = tx_bytes / (1024.0 * 1024.0);
                
                snprintf(stats->rx_bytes, sizeof(stats->rx_bytes), "%.1f MB", rx_mb);
                snprintf(stats->tx_bytes, sizeof(stats->tx_bytes), "%.1f MB", tx_mb);
                break;
            }
        }
        fclose(fp);
    }
    
    stats->tx_rate = 45000;
    stats->rx_rate = 78000;
    stats->connected_devices = 4;
    stats->session_start = system_start_time;
    
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
    ApnConfig apns[MAX_APN_COUNT];
    int count = 0;
    
    config_get_apn_list(apns, &count);
    
    for (int i = 0; i < count; i++) {
        apns[i].is_active = (apns[i].id == apn_id);
        if (apns[i].id == apn_id) {
            at_set_apn(apns[i].name, apns[i].username, apns[i].password, apns[i].auth_type);
            at_activate_pdp_context();
        }
        config_set_apn(&apns[i]);
    }
    
    return 0;
}

int cgi_get_wlan_ap_config(WlanApConfig *config)
{
    return config_get_wlan_ap(config);
}

int cgi_set_wlan_ap_config(WlanApConfig *config)
{
    int ret = config_set_wlan_ap(config);
    if (ret == 0) {
        system("wifi reload");
    }
    return ret;
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
    int ret = config_set_firewall(config);
    if (ret == 0) {
        system("firewall restart");
    }
    return ret;
}

int cgi_reboot(void)
{
    system("sync");
    system("reboot");
    return 0;
}

int cgi_factory_reset(void)
{
    system("rm -f /etc/cpe/config.json");
    system("sync");
    system("reboot");
    return 0;
}

int cgi_get_cellular_config(CellularConfig *config)
{
    return config_get_cellular(config);
}

int cgi_set_cellular_config(CellularConfig *config)
{
    int ret = config_set_cellular(config);
    if (ret == 0) {
        at_set_network_mode(config->network_mode);
    }
    return ret;
}

int cgi_get_vpn_config(int type, VpnConfig *config)
{
    return config_get_vpn(type, config);
}

int cgi_set_vpn_config(VpnConfig *config)
{
    return config_set_vpn(config);
}

int cgi_vpn_connect(int type)
{
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "vpn start %d", type);
    system(cmd);
    return 0;
}

int cgi_vpn_disconnect(int type)
{
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "vpn stop %d", type);
    system(cmd);
    return 0;
}

int cgi_get_iot_config(IotConfig *config)
{
    return config_get_iot(config);
}

int cgi_set_iot_config(IotConfig *config)
{
    return config_set_iot(config);
}

int cgi_get_system_config(SystemConfig *config)
{
    return config_get_system(config);
}

int cgi_set_system_config(SystemConfig *config)
{
    return config_set_system(config);
}

int cgi_get_connected_devices(ConnectedDevice *devices, int *count)
{
    if (!devices || !count) return -1;
    
    *count = 0;
    
    FILE *fp = fopen("/proc/net/arp", "r");
    if (!fp) return -1;
    
    char line[256];
    fgets(line, sizeof(line), fp);
    
    while (fgets(line, sizeof(line), fp) && *count < 64) {
        char ip[16], hw_type[8], flags[8], hw_addr[18], mask[8], device[16];
        
        if (sscanf(line, "%15s %7s %7s %17s %7s %15s",
                   ip, hw_type, flags, hw_addr, mask, device) == 6) {
            if (strcmp(hw_addr, "00:00:00:00:00:00") != 0 && 
                strncmp(hw_addr, "ff:ff:ff:ff:ff:ff", 17) != 0) {
                strncpy(devices[*count].ip, ip, sizeof(devices[*count].ip) - 1);
                strncpy(devices[*count].mac, hw_addr, sizeof(devices[*count].mac) - 1);
                strcpy(devices[*count].hostname, "Unknown");
                strncpy(devices[*count].interface, device, sizeof(devices[*count].interface) - 1);
                devices[*count].connected_time = time(NULL);
                (*count)++;
            }
        }
    }
    
    fclose(fp);
    return 0;
}

int cgi_send_at_command(const char *command, char *response, int timeout_ms)
{
    if (!command || !response) return -1;
    
    return at_command_send(command, response, timeout_ms);
}
