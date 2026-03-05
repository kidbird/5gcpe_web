#include "cpe_control.h"
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <pthread.h>
#include <sys/select.h>
#include <ctype.h>
#include <stdarg.h>

typedef struct {
    int fd;
    pthread_mutex_t mutex;
    const char *device;
} AtBackendContext;

static int at_send_command(int fd, const char *command, char *response, int timeout_ms)
{
    if (fd < 0) return -1;

    tcflush(fd, TCIOFLUSH);

    char cmd[256];
    snprintf(cmd, sizeof(cmd), "%s\r\n", command);

    int len = strlen(cmd);
    int written = 0;
    while (written < len) {
        int n = write(fd, cmd + written, len - written);
        if (n < 0) return -1;
        written += n;
    }

    usleep(100000);

    if (response) {
        memset(response, 0, MAX_BUFFER_SIZE);

        fd_set rfds;
        struct timeval tv;

        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);

        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;

        int total = 0;
        int max_loops = timeout_ms / 100;

        for (int i = 0; i < max_loops; i++) {
            int sel = select(fd + 1, &rfds, NULL, NULL, &tv);
            if (sel > 0) {
                char buf[256];
                int n = read(fd, buf, sizeof(buf) - 1);
                if (n > 0) {
                    buf[n] = '\0';
                    if (total + n < MAX_BUFFER_SIZE) {
                        strcat(response, buf);
                        total += n;
                    }

                    if (strstr(response, "OK") || strstr(response, "ERROR")) {
                        break;
                    }
                }
            } else if (sel == 0) {
                break;
            }
        }
    }

    return 0;
}

static CpeCtrlResult at_init(const char *device)
{
    AtBackendContext *ctx = (AtBackendContext *)((CpeControlOps*)NULL)->context;

    int fd = open(device ? device : "/dev/ttyUSB2", O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) {
        return CTRL_NO_DEVICE;
    }

    struct termios tty;
    memset(&tty, 0, sizeof(tty));

    if (tcgetattr(fd, &tty) != 0) {
        close(fd);
        return CTRL_ERROR;
    }

    cfsetospeed(&tty, B115200);
    cfsetispeed(&tty, B115200);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_iflag &= ~IGNBRK;
    tty.c_lflag = 0;
    tty.c_oflag = 0;
    tty.c_cc[VMIN]  = 0;
    tty.c_cc[VTIME] = 5;

    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~(PARENB | PARODD);
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        close(fd);
        return CTRL_ERROR;
    }

    tcflush(fd, TCIOFLUSH);

    return CTRL_OK;
}

static void at_close(void)
{
}

static CpeCtrlResult at_get_signal_strength(int *rssi)
{
    char response[MAX_BUFFER_SIZE];
    AtBackendContext *ctx = NULL;

    at_send_command(ctx ? ctx->fd : -1, "AT+CSQ", response, AT_COMMAND_TIMEOUT);

    int value = -1;
    char *p = strstr(response, "+CSQ:");
    if (p) {
        sscanf(p, "+CSQ: %d,", &value);
        if (value == 99) {
            return CTRL_ERROR;
        }
        *rssi = -113 + (value * 2);
        return CTRL_OK;
    }

    return CTRL_ERROR;
}

static CpeCtrlResult at_get_operator(char *operator_name, int len)
{
    char response[MAX_BUFFER_SIZE];
    AtBackendContext *ctx = NULL;

    at_send_command(ctx ? ctx->fd : -1, "AT+COPS?", response, AT_COMMAND_TIMEOUT);

    char *p = strstr(response, "+COPS:");
    if (p) {
        char *start = strchr(p, '"');
        char *end = strrchr(p, '"');
        if (start && end && start != end) {
            int slen = end - start - 1;
            if (slen < len) {
                strncpy(operator_name, start + 1, slen);
                operator_name[slen] = '\0';
                return CTRL_OK;
            }
        }
    }

    return CTRL_ERROR;
}

static CpeCtrlResult at_get_imei(char *imei, int len)
{
    char response[MAX_BUFFER_SIZE];
    AtBackendContext *ctx = NULL;

    at_send_command(ctx ? ctx->fd : -1, "AT+GSN", response, AT_COMMAND_TIMEOUT);

    char *p = response;
    while (*p && !isdigit(*p)) p++;

    if (*p) {
        int i = 0;
        while (*p && isdigit(*p) && i < len - 1) {
            imei[i++] = *p++;
        }
        imei[i] = '\0';
        return CTRL_OK;
    }

    return CTRL_ERROR;
}

static CpeCtrlResult at_get_iccid(char *iccid, int len)
{
    char response[MAX_BUFFER_SIZE];
    AtBackendContext *ctx = NULL;

    at_send_command(ctx ? ctx->fd : -1, "AT+QCCID", response, AT_COMMAND_TIMEOUT);

    char *p = strstr(response, "+QCCID:");
    if (p) {
        sscanf(p, "+QCCID: %s", iccid);
        return CTRL_OK;
    }

    return CTRL_ERROR;
}

static CpeCtrlResult at_get_imsi(char *imsi, int len)
{
    char response[MAX_BUFFER_SIZE];
    AtBackendContext *ctx = NULL;

    at_send_command(ctx ? ctx->fd : -1, "AT+CIMI", response, AT_COMMAND_TIMEOUT);

    char *p = response;
    while (*p && !isdigit(*p)) p++;

    if (*p) {
        int i = 0;
        while (*p && isdigit(*p) && i < len - 1) {
            imsi[i++] = *p++;
        }
        imsi[i] = '\0';
        return CTRL_OK;
    }

    return CTRL_ERROR;
}

static CpeCtrlResult at_get_network_mode(int *mode)
{
    char response[MAX_BUFFER_SIZE];
    AtBackendContext *ctx = NULL;

    at_send_command(ctx ? ctx->fd : -1, "AT+CNMP?", response, AT_COMMAND_TIMEOUT);

    char *p = strstr(response, "+CNMP:");
    if (p) {
        sscanf(p, "+CNMP: %d", mode);
        return CTRL_OK;
    }

    return CTRL_ERROR;
}

static CpeCtrlResult at_set_network_mode(int mode)
{
    AtBackendContext *ctx = NULL;
    char cmd[64];

    switch (mode) {
        case NETWORK_5G_SA:
            snprintf(cmd, sizeof(cmd), "AT+CNMP=71");
            break;
        case NETWORK_5G_SA_NSA:
            snprintf(cmd, sizeof(cmd), "AT+CNMP=70");
            break;
        case NETWORK_LTE:
            snprintf(cmd, sizeof(cmd), "AT+CNMP=61");
            break;
        default:
            snprintf(cmd, sizeof(cmd), "AT+CNMP=2");
    }

    char response[MAX_BUFFER_SIZE];
    at_send_command(ctx ? ctx->fd : -1, cmd, response, AT_COMMAND_TIMEOUT);

    return strstr(response, "OK") ? CTRL_OK : CTRL_ERROR;
}

static CpeCtrlResult at_get_network_registration(int *reg_status)
{
    char response[MAX_BUFFER_SIZE];
    AtBackendContext *ctx = NULL;

    at_send_command(ctx ? ctx->fd : -1, "AT+CREG?", response, AT_COMMAND_TIMEOUT);

    char *p = strstr(response, "+CREG:");
    if (p) {
        sscanf(p, "+CREG: %*d,%d", reg_status);
        return CTRL_OK;
    }

    return CTRL_ERROR;
}

static CpeCtrlResult at_set_apn(const char *apn, const char *username, const char *password, int auth_type)
{
    AtBackendContext *ctx = NULL;
    char cmd[256];

    snprintf(cmd, sizeof(cmd), "AT+CGDCONT=1,\"IP\",\"%s\"", apn);
    char response[MAX_BUFFER_SIZE];
    at_send_command(ctx ? ctx->fd : -1, cmd, response, AT_COMMAND_TIMEOUT);

    if (auth_type > 0) {
        snprintf(cmd, sizeof(cmd), "AT+CGAUTH=1,1,%d,\"%s\",\"%s\"", auth_type, password, username);
        at_send_command(ctx ? ctx->fd : -1, cmd, response, AT_COMMAND_TIMEOUT);
    }

    return CTRL_OK;
}

static CpeCtrlResult at_activate_pdp(int cid)
{
    AtBackendContext *ctx = NULL;
    char cmd[64];
    char response[MAX_BUFFER_SIZE];

    snprintf(cmd, sizeof(cmd), "AT+CGACT=1,%d", cid);
    at_send_command(ctx ? ctx->fd : -1, cmd, response, 5000);

    return strstr(response, "OK") ? CTRL_OK : CTRL_ERROR;
}

static CpeCtrlResult at_deactivate_pdp(int cid)
{
    AtBackendContext *ctx = NULL;
    char cmd[64];
    char response[MAX_BUFFER_SIZE];

    snprintf(cmd, sizeof(cmd), "AT+CGACT=0,%d", cid);
    at_send_command(ctx ? ctx->fd : -1, cmd, response, 5000);

    return CTRL_OK;
}

static CpeCtrlResult at_get_apn_info(char *apn, int len)
{
    char response[MAX_BUFFER_SIZE];
    AtBackendContext *ctx = NULL;

    at_send_command(ctx ? ctx->fd : -1, "AT+CGDCONT?", response, AT_COMMAND_TIMEOUT);

    char *p = strstr(response, "+CGDCONT:");
    if (p) {
        char *start = strchr(p, '"');
        char *end = strchr(start + 1, '"');
        if (start && end) {
            int slen = end - start - 1;
            if (slen < len) {
                strncpy(apn, start + 1, slen);
                apn[slen] = '\0';
                return CTRL_OK;
            }
        }
    }

    return CTRL_ERROR;
}

static CpeCtrlResult at_get_device_status(DeviceStatus *status)
{
    char response[MAX_BUFFER_SIZE];
    AtBackendContext *ctx = NULL;

    strcpy(status->device_model, "RG500U-CN");
    strcpy(status->firmware_version, "RG500UCNAAR02A04M2G");

    at_send_command(ctx ? ctx->fd : -1, "AT+GSN", response, AT_COMMAND_TIMEOUT);
    char *p = response;
    while (*p && !isdigit(*p)) p++;
    if (*p) {
        int i = 0;
        while (*p && isdigit(*p) && i < 19) {
            status->imei[i++] = *p++;
        }
        status->imei[i] = '\0';
    }

    at_send_command(ctx ? ctx->fd : -1, "AT+QCCID", response, AT_COMMAND_TIMEOUT);
    p = strstr(response, "+QCCID:");
    if (p) {
        sscanf(p, "+QCCID: %s", status->iccid);
    }

    at_send_command(ctx ? ctx->fd : -1, "AT+COPS?", response, AT_COMMAND_TIMEOUT);
    p = strstr(response, "+COPS:");
    if (p) {
        char *start = strchr(p, '"');
        char *end = strrchr(p, '"');
        if (start && end && start != end) {
            int slen = end - start - 1;
            if (slen < 32) {
                strncpy(status->operator_name, start + 1, slen);
                status->operator_name[slen] = '\0';
            }
        }
    }

    at_send_command(ctx ? ctx->fd : -1, "AT+CSQ", response, AT_COMMAND_TIMEOUT);
    p = strstr(response, "+CSQ:");
    if (p) {
        int csq;
        sscanf(p, "+CSQ: %d,", &csq);
        if (csq != 99) {
            status->signal_strength = -113 + (csq * 2);
        }
    }

    strcpy(status->network_type, "5G NR");
    strcpy(status->lan_mac, "00:1A:2B:3C:4D:5E");
    strcpy(status->wan_ip, "10.0.0.1");
    strcpy(status->uptime, "0天 0时 0分");

    status->rsrp = -85;
    status->sinr = 15;

    return CTRL_OK;
}

static CpeCtrlResult at_get_traffic_stats(TrafficStats *stats)
{
    strcpy(stats->tx_bytes, "0 MB");
    strcpy(stats->rx_bytes, "0 MB");
    stats->tx_rate = 0;
    stats->rx_rate = 0;
    return CTRL_OK;
}

static CpeCtrlResult at_wlan_init(void)
{
    AtBackendContext *ctx = NULL;
    char response[MAX_BUFFER_SIZE];
    at_send_command(ctx ? ctx->fd : -1, "AT+QWIFI=1", response, AT_COMMAND_TIMEOUT);
    return strstr(response, "OK") ? CTRL_OK : CTRL_ERROR;
}

static CpeCtrlResult at_wlan_deinit(void)
{
    AtBackendContext *ctx = NULL;
    char response[MAX_BUFFER_SIZE];
    at_send_command(ctx ? ctx->fd : -1, "AT+QWIFI=0", response, AT_COMMAND_TIMEOUT);
    return CTRL_OK;
}

static CpeCtrlResult at_wlan_set_ap(const char *ssid, const char *password, int channel, int encryption)
{
    AtBackendContext *ctx = NULL;
    char cmd[256];
    char response[MAX_BUFFER_SIZE];

    if (encryption == 0) {
        snprintf(cmd, sizeof(cmd), "AT+QAPCONFIG=1,\"%s\",%d,0", ssid, channel);
    } else {
        snprintf(cmd, sizeof(cmd), "AT+QAPCONFIG=1,\"%s\",%d,4,\"%s\"", ssid, channel, password);
    }

    at_send_command(ctx ? ctx->fd : -1, cmd, response, AT_COMMAND_TIMEOUT);
    return CTRL_OK;
}

static CpeCtrlResult at_wlan_set_sta(const char *ssid, const char *password)
{
    AtBackendContext *ctx = NULL;
    char cmd[256];
    char response[MAX_BUFFER_SIZE];

    snprintf(cmd, sizeof(cmd), "AT+QWSTACONF=1,\"%s\",\"%s\"", ssid, password);
    at_send_command(ctx ? ctx->fd : -1, cmd, response, AT_COMMAND_TIMEOUT);

    return CTRL_OK;
}

static CpeCtrlResult at_wlan_sta_connect(void)
{
    AtBackendContext *ctx = NULL;
    char response[MAX_BUFFER_SIZE];
    at_send_command(ctx ? ctx->fd : -1, "AT+QWSTACONN=1", response, AT_COMMAND_TIMEOUT);
    return CTRL_OK;
}

static CpeCtrlResult at_wlan_sta_disconnect(void)
{
    AtBackendContext *ctx = NULL;
    char response[MAX_BUFFER_SIZE];
    at_send_command(ctx ? ctx->fd : -1, "AT+QWSTACONN=0", response, AT_COMMAND_TIMEOUT);
    return CTRL_OK;
}

static CpeCtrlResult at_wlan_scan(WifiScanResult *results, int *count)
{
    return CTRL_ERROR;
}

static CpeCtrlResult at_reboot(void)
{
    AtBackendContext *ctx = NULL;
    char response[MAX_BUFFER_SIZE];
    at_send_command(ctx ? ctx->fd : -1, "AT+CFUN=1,1", response, 0);
    return CTRL_OK;
}

static CpeCtrlResult at_factory_reset(void)
{
    AtBackendContext *ctx = NULL;
    char response[MAX_BUFFER_SIZE];
    at_send_command(ctx ? ctx->fd : -1, "AT&F", response, AT_COMMAND_TIMEOUT);
    return CTRL_OK;
}

static CpeCtrlResult at_set_airplane_mode(int enable)
{
    AtBackendContext *ctx = NULL;
    char cmd[64];
    char response[MAX_BUFFER_SIZE];

    snprintf(cmd, sizeof(cmd), "AT+CFUN=%d", enable ? 0 : 1);
    at_send_command(ctx ? ctx->fd : -1, cmd, response, AT_COMMAND_TIMEOUT);

    return CTRL_OK;
}

static CpeCtrlResult at_get_temperature(int *temp)
{
    char response[MAX_BUFFER_SIZE];
    AtBackendContext *ctx = NULL;

    at_send_command(ctx ? ctx->fd : -1, "AT+QTEMP", response, AT_COMMAND_TIMEOUT);

    char *p = strstr(response, "+QTEMP:");
    if (p) {
        sscanf(p, "+QTEMP: %d", temp);
        return CTRL_OK;
    }

    return CTRL_ERROR;
}

CpeControlOps* at_backend_create(void)
{
    CpeControlOps *ops = (CpeControlOps *)malloc(sizeof(CpeControlOps));
    if (!ops) return NULL;

    AtBackendContext *ctx = (AtBackendContext *)malloc(sizeof(AtBackendContext));
    if (!ctx) {
        free(ops);
        return NULL;
    }

    ctx->fd = -1;
    pthread_mutex_init(&ctx->mutex, NULL);
    ctx->device = "/dev/ttyUSB2";

    ops->context = ctx;
    ops->init = at_init;
    ops->close = at_close;
    ops->get_signal_strength = at_get_signal_strength;
    ops->get_operator = at_get_operator;
    ops->get_imei = at_get_imei;
    ops->get_iccid = at_get_iccid;
    ops->get_imsi = at_get_imsi;
    ops->get_network_mode = at_get_network_mode;
    ops->set_network_mode = at_set_network_mode;
    ops->get_network_registration = at_get_network_registration;
    ops->set_apn = at_set_apn;
    ops->activate_pdp = at_activate_pdp;
    ops->deactivate_pdp = at_deactivate_pdp;
    ops->get_apn_info = at_get_apn_info;
    ops->get_device_status = at_get_device_status;
    ops->get_traffic_stats = at_get_traffic_stats;
    ops->wlan_init = at_wlan_init;
    ops->wlan_deinit = at_wlan_deinit;
    ops->wlan_set_ap = at_wlan_set_ap;
    ops->wlan_set_sta = at_wlan_set_sta;
    ops->wlan_sta_connect = at_wlan_sta_connect;
    ops->wlan_sta_disconnect = at_wlan_sta_disconnect;
    ops->wlan_scan = at_wlan_scan;
    ops->reboot = at_reboot;
    ops->factory_reset = at_factory_reset;
    ops->set_airplane_mode = at_set_airplane_mode;
    ops->get_temperature = at_get_temperature;

    return ops;
}
