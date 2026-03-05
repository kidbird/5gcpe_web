#include "cpe_control.h"
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>

typedef struct {
    int sock_fd;
    char host[128];
    int port;
    pthread_mutex_t mutex;
} ApiBackendContext;

static CpeCtrlResult api_socket_connect(ApiBackendContext *ctx)
{
    if (ctx->sock_fd >= 0) {
        return CTRL_OK;
    }

    ctx->sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (ctx->sock_fd < 0) {
        return CTRL_ERROR;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(ctx->port);

    if (inet_pton(AF_INET, ctx->host, &server_addr.sin_addr) <= 0) {
        struct hostent *he = gethostbyname(ctx->host);
        if (!he) {
            close(ctx->sock_fd);
            ctx->sock_fd = -1;
            return CTRL_ERROR;
        }
        memcpy(&server_addr.sin_addr, he->h_addr_list[0], he->h_length);
    }

    if (connect(ctx->sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        close(ctx->sock_fd);
        ctx->sock_fd = -1;
        return CTRL_NO_DEVICE;
    }

    return CTRL_OK;
}

static void api_socket_close(ApiBackendContext *ctx)
{
    if (ctx->sock_fd >= 0) {
        close(ctx->sock_fd);
        ctx->sock_fd = -1;
    }
}

static CpeCtrlResult api_socket_send_recv(ApiBackendContext *ctx, const char *request, char *response, int resp_len)
{
    pthread_mutex_lock(&ctx->mutex);

    CpeCtrlResult ret = api_socket_connect(ctx);
    if (ret != CTRL_OK) {
        pthread_mutex_unlock(&ctx->mutex);
        return ret;
    }

    send(ctx->sock_fd, request, strlen(request), 0);

    memset(response, 0, resp_len);
    int n = recv(ctx->sock_fd, response, resp_len - 1, 0);
    if (n <= 0) {
        pthread_mutex_unlock(&ctx->mutex);
        return CTRL_ERROR;
    }

    pthread_mutex_unlock(&ctx->mutex);

    return CTRL_OK;
}

static CpeCtrlResult api_init(const char *config)
{
    return CTRL_OK;
}

static void api_close(void)
{
}

static CpeCtrlResult api_get_signal_strength(int *rssi)
{
    char response[MAX_BUFFER_SIZE];
    ApiBackendContext *ctx = NULL;

    char request[256];
    snprintf(request, sizeof(request), "AT+CSQ\r\n");

    api_socket_send_recv(ctx, request, response, sizeof(response));

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

static CpeCtrlResult api_get_operator(char *operator_name, int len)
{
    char response[MAX_BUFFER_SIZE];
    ApiBackendContext *ctx = NULL;

    char request[256];
    snprintf(request, sizeof(request), "AT+COPS?\r\n");
    api_socket_send_recv(ctx, request, response, sizeof(response));

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

static CpeCtrlResult api_get_imei(char *imei, int len)
{
    char response[MAX_BUFFER_SIZE];
    ApiBackendContext *ctx = NULL;

    char request[256];
    snprintf(request, sizeof(request), "AT+GSN\r\n");
    api_socket_send_recv(ctx, request, response, sizeof(response));

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

static CpeCtrlResult api_get_iccid(char *iccid, int len)
{
    char response[MAX_BUFFER_SIZE];
    ApiBackendContext *ctx = NULL;

    char request[256];
    snprintf(request, sizeof(request), "AT+QCCID\r\n");
    api_socket_send_recv(ctx, request, response, sizeof(response));

    char *p = strstr(response, "+QCCID:");
    if (p) {
        sscanf(p, "+QCCID: %s", iccid);
        return CTRL_OK;
    }

    return CTRL_ERROR;
}

static CpeCtrlResult api_get_imsi(char *imsi, int len)
{
    char response[MAX_BUFFER_SIZE];
    ApiBackendContext *ctx = NULL;

    char request[256];
    snprintf(request, sizeof(request), "AT+CIMI\r\n");
    api_socket_send_recv(ctx, request, response, sizeof(response));

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

static CpeCtrlResult api_get_network_mode(int *mode)
{
    char response[MAX_BUFFER_SIZE];
    ApiBackendContext *ctx = NULL;

    char request[256];
    snprintf(request, sizeof(request), "AT+CNMP?\r\n");
    api_socket_send_recv(ctx, request, response, sizeof(response));

    char *p = strstr(response, "+CNMP:");
    if (p) {
        sscanf(p, "+CNMP: %d", mode);
        return CTRL_OK;
    }

    return CTRL_ERROR;
}

static CpeCtrlResult api_set_network_mode(int mode)
{
    ApiBackendContext *ctx = NULL;

    char cmd[64];
    switch (mode) {
        case NETWORK_5G_SA:
            snprintf(cmd, sizeof(cmd), "AT+CNMP=71\r\n");
            break;
        case NETWORK_5G_SA_NSA:
            snprintf(cmd, sizeof(cmd), "AT+CNMP=70\r\n");
            break;
        case NETWORK_LTE:
            snprintf(cmd, sizeof(cmd), "AT+CNMP=61\r\n");
            break;
        default:
            snprintf(cmd, sizeof(cmd), "AT+CNMP=2\r\n");
    }

    char response[MAX_BUFFER_SIZE];
    api_socket_send_recv(ctx, cmd, response, sizeof(response));

    return strstr(response, "OK") ? CTRL_OK : CTRL_ERROR;
}

static CpeCtrlResult api_get_network_registration(int *reg_status)
{
    char response[MAX_BUFFER_SIZE];
    ApiBackendContext *ctx = NULL;

    char request[256];
    snprintf(request, sizeof(request), "AT+CREG?\r\n");
    api_socket_send_recv(ctx, request, response, sizeof(response));

    char *p = strstr(response, "+CREG:");
    if (p) {
        sscanf(p, "+CREG: %*d,%d", reg_status);
        return CTRL_OK;
    }

    return CTRL_ERROR;
}

static CpeCtrlResult api_set_apn(const char *apn, const char *username, const char *password, int auth_type)
{
    ApiBackendContext *ctx = NULL;

    char cmd[256];
    char response[MAX_BUFFER_SIZE];

    snprintf(cmd, sizeof(cmd), "AT+CGDCONT=1,\"IP\",\"%s\"\r\n", apn);
    api_socket_send_recv(ctx, cmd, response, sizeof(response));

    if (auth_type > 0) {
        snprintf(cmd, sizeof(cmd), "AT+CGAUTH=1,1,%d,\"%s\",\"%s\"\r\n", auth_type, password, username);
        api_socket_send_recv(ctx, cmd, response, sizeof(response));
    }

    return CTRL_OK;
}

static CpeCtrlResult api_activate_pdp(int cid)
{
    ApiBackendContext *ctx = NULL;
    char cmd[64];
    char response[MAX_BUFFER_SIZE];

    snprintf(cmd, sizeof(cmd), "AT+CGACT=1,%d\r\n", cid);
    api_socket_send_recv(ctx, cmd, response, sizeof(response));

    return strstr(response, "OK") ? CTRL_OK : CTRL_ERROR;
}

static CpeCtrlResult api_deactivate_pdp(int cid)
{
    ApiBackendContext *ctx = NULL;
    char cmd[64];
    char response[MAX_BUFFER_SIZE];

    snprintf(cmd, sizeof(cmd), "AT+CGACT=0,%d\r\n", cid);
    api_socket_send_recv(ctx, cmd, response, sizeof(response));

    return CTRL_OK;
}

static CpeCtrlResult api_get_apn_info(char *apn, int len)
{
    char response[MAX_BUFFER_SIZE];
    ApiBackendContext *ctx = NULL;

    char request[256];
    snprintf(request, sizeof(request), "AT+CGDCONT?\r\n");
    api_socket_send_recv(ctx, request, response, sizeof(response));

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

static CpeCtrlResult api_get_device_status(DeviceStatus *status)
{
    ApiBackendContext *ctx = NULL;

    strcpy(status->device_model, "RG500U-CN");
    strcpy(status->firmware_version, "RG500UCNAAR02A04M2G");

    char request[256];
    char response[MAX_BUFFER_SIZE];

    snprintf(request, sizeof(request), "AT+GSN\r\n");
    api_socket_send_recv(ctx, request, response, sizeof(response));
    char *p = response;
    while (*p && !isdigit(*p)) p++;
    if (*p) {
        int i = 0;
        while (*p && isdigit(*p) && i < 19) {
            status->imei[i++] = *p++;
        }
        status->imei[i] = '\0';
    }

    snprintf(request, sizeof(request), "AT+QCCID\r\n");
    api_socket_send_recv(ctx, request, response, sizeof(response));
    p = strstr(response, "+QCCID:");
    if (p) {
        sscanf(p, "+QCCID: %s", status->iccid);
    }

    snprintf(request, sizeof(request), "AT+COPS?\r\n");
    api_socket_send_recv(ctx, request, response, sizeof(response));
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

    snprintf(request, sizeof(request), "AT+CSQ\r\n");
    api_socket_send_recv(ctx, request, response, sizeof(response));
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

static CpeCtrlResult api_get_traffic_stats(TrafficStats *stats)
{
    strcpy(stats->tx_bytes, "0 MB");
    strcpy(stats->rx_bytes, "0 MB");
    stats->tx_rate = 0;
    stats->rx_rate = 0;
    return CTRL_OK;
}

static CpeCtrlResult api_wlan_init(void)
{
    ApiBackendContext *ctx = NULL;
    char response[MAX_BUFFER_SIZE];
    api_socket_send_recv(ctx, "AT+QWIFI=1\r\n", response, sizeof(response));
    return strstr(response, "OK") ? CTRL_OK : CTRL_ERROR;
}

static CpeCtrlResult api_wlan_deinit(void)
{
    ApiBackendContext *ctx = NULL;
    char response[MAX_BUFFER_SIZE];
    api_socket_send_recv(ctx, "AT+QWIFI=0\r\n", response, sizeof(response));
    return CTRL_OK;
}

static CpeCtrlResult api_wlan_set_ap(const char *ssid, const char *password, int channel, int encryption)
{
    ApiBackendContext *ctx = NULL;
    char cmd[256];
    char response[MAX_BUFFER_SIZE];

    if (encryption == 0) {
        snprintf(cmd, sizeof(cmd), "AT+QAPCONFIG=1,\"%s\",%d,0\r\n", ssid, channel);
    } else {
        snprintf(cmd, sizeof(cmd), "AT+QAPCONFIG=1,\"%s\",%d,4,\"%s\"\r\n", ssid, channel, password);
    }

    api_socket_send_recv(ctx, cmd, response, sizeof(response));
    return CTRL_OK;
}

static CpeCtrlResult api_wlan_set_sta(const char *ssid, const char *password)
{
    ApiBackendContext *ctx = NULL;
    char cmd[256];
    char response[MAX_BUFFER_SIZE];

    snprintf(cmd, sizeof(cmd), "AT+QWSTACONF=1,\"%s\",\"%s\"\r\n", ssid, password);
    api_socket_send_recv(ctx, cmd, response, sizeof(response));

    return CTRL_OK;
}

static CpeCtrlResult api_wlan_sta_connect(void)
{
    ApiBackendContext *ctx = NULL;
    char response[MAX_BUFFER_SIZE];
    api_socket_send_recv(ctx, "AT+QWSTACONN=1\r\n", response, sizeof(response));
    return CTRL_OK;
}

static CpeCtrlResult api_wlan_sta_disconnect(void)
{
    ApiBackendContext *ctx = NULL;
    char response[MAX_BUFFER_SIZE];
    api_socket_send_recv(ctx, "AT+QWSTACONN=0\r\n", response, sizeof(response));
    return CTRL_OK;
}

static CpeCtrlResult api_wlan_scan(WifiScanResult *results, int *count)
{
    return CTRL_ERROR;
}

static CpeCtrlResult api_reboot(void)
{
    ApiBackendContext *ctx = NULL;
    char response[MAX_BUFFER_SIZE];
    api_socket_send_recv(ctx, "AT+CFUN=1,1\r\n", response, sizeof(response));
    return CTRL_OK;
}

static CpeCtrlResult api_factory_reset(void)
{
    ApiBackendContext *ctx = NULL;
    char response[MAX_BUFFER_SIZE];
    api_socket_send_recv(ctx, "AT&F\r\n", response, sizeof(response));
    return CTRL_OK;
}

static CpeCtrlResult api_set_airplane_mode(int enable)
{
    ApiBackendContext *ctx = NULL;
    char cmd[64];
    char response[MAX_BUFFER_SIZE];

    snprintf(cmd, sizeof(cmd), "AT+CFUN=%d\r\n", enable ? 0 : 1);
    api_socket_send_recv(ctx, cmd, response, sizeof(response));

    return CTRL_OK;
}

static CpeCtrlResult api_get_temperature(int *temp)
{
    char response[MAX_BUFFER_SIZE];
    ApiBackendContext *ctx = NULL;

    api_socket_send_recv(ctx, "AT+QTEMP\r\n", response, sizeof(response));

    char *p = strstr(response, "+QTEMP:");
    if (p) {
        sscanf(p, "+QTEMP: %d", temp);
        return CTRL_OK;
    }

    return CTRL_ERROR;
}

CpeControlOps* api_backend_create(void)
{
    CpeControlOps *ops = (CpeControlOps *)malloc(sizeof(CpeControlOps));
    if (!ops) return NULL;

    ApiBackendContext *ctx = (ApiBackendContext *)malloc(sizeof(ApiBackendContext));
    if (!ctx) {
        free(ops);
        return NULL;
    }

    ctx->sock_fd = -1;
    strcpy(ctx->host, "127.0.0.1");
    ctx->port = 9000;
    pthread_mutex_init(&ctx->mutex, NULL);

    ops->context = ctx;
    ops->init = api_init;
    ops->close = api_close;
    ops->get_signal_strength = api_get_signal_strength;
    ops->get_operator = api_get_operator;
    ops->get_imei = api_get_imei;
    ops->get_iccid = api_get_iccid;
    ops->get_imsi = api_get_imsi;
    ops->get_network_mode = api_get_network_mode;
    ops->set_network_mode = api_set_network_mode;
    ops->get_network_registration = api_get_network_registration;
    ops->set_apn = api_set_apn;
    ops->activate_pdp = api_activate_pdp;
    ops->deactivate_pdp = api_deactivate_pdp;
    ops->get_apn_info = api_get_apn_info;
    ops->get_device_status = api_get_device_status;
    ops->get_traffic_stats = api_get_traffic_stats;
    ops->wlan_init = api_wlan_init;
    ops->wlan_deinit = api_wlan_deinit;
    ops->wlan_set_ap = api_wlan_set_ap;
    ops->wlan_set_sta = api_wlan_set_sta;
    ops->wlan_sta_connect = api_wlan_sta_connect;
    ops->wlan_sta_disconnect = api_wlan_sta_disconnect;
    ops->wlan_scan = api_wlan_scan;
    ops->reboot = api_reboot;
    ops->factory_reset = api_factory_reset;
    ops->set_airplane_mode = api_set_airplane_mode;
    ops->get_temperature = api_get_temperature;

    return ops;
}
