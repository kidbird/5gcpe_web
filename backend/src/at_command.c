#include "cpe.h"

static int at_fd = -1;
static pthread_mutex_t at_mutex = PTHREAD_MUTEX_INITIALIZER;
static const char *at_device = "/dev/ttyUSB2";

int at_command_init(const char *device)
{
    if (at_fd >= 0) {
        return 0;
    }

    if (device) {
        at_device = device;
    }

    at_fd = open(at_device, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (at_fd < 0) {
        return -1;
    }

    struct termios tty;
    memset(&tty, 0, sizeof(tty));

    if (tcgetattr(at_fd, &tty) != 0) {
        close(at_fd);
        at_fd = -1;
        return -1;
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

    if (tcsetattr(at_fd, TCSANOW, &tty) != 0) {
        close(at_fd);
        at_fd = -1;
        return -1;
    }

    tcflush(at_fd, TCIOFLUSH);

    return 0;
}

void at_command_close(void)
{
    if (at_fd >= 0) {
        close(at_fd);
        at_fd = -1;
    }
}

int at_command_send(const char *command, char *response, int timeout_ms)
{
    int ret = -1;

    pthread_mutex_lock(&at_mutex);

    if (at_fd < 0) {
        pthread_mutex_unlock(&at_mutex);
        return -1;
    }

    tcflush(at_fd, TCIOFLUSH);

    char cmd[256];
    snprintf(cmd, sizeof(cmd), "%s\r\n", command);

    int len = strlen(cmd);
    int written = 0;
    while (written < len) {
        int n = write(at_fd, cmd + written, len - written);
        if (n < 0) {
            pthread_mutex_unlock(&at_mutex);
            return -1;
        }
        written += n;
    }

    usleep(100000);

    if (response) {
        memset(response, 0, MAX_BUFFER_SIZE);

        struct timeval tv;
        fd_set rfds;

        FD_ZERO(&rfds);
        FD_SET(at_fd, &rfds);

        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;

        int total = 0;
        int max_loops = timeout_ms / 100;

        for (int i = 0; i < max_loops; i++) {
            int sel = select(at_fd + 1, &rfds, NULL, NULL, &tv);
            if (sel > 0) {
                char buf[256];
                int n = read(at_fd, buf, sizeof(buf) - 1);
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

        char *ok_pos = strstr(response, "OK");
        char *err_pos = strstr(response, "ERROR");

        if (ok_pos || err_pos) {
            ret = 0;
        }
    } else {
        ret = 0;
    }

    pthread_mutex_unlock(&at_mutex);

    return ret;
}

int at_command_exec(const char *fmt, ...)
{
    char command[256];
    char response[MAX_BUFFER_SIZE];

    va_list args;
    va_start(args, fmt);
    vsnprintf(command, sizeof(command), fmt, args);
    va_end(args);

    return at_command_send(command, response, AT_COMMAND_TIMEOUT);
}

int at_get_signal_strength(void)
{
    char response[MAX_BUFFER_SIZE];

    if (at_command_send("AT+CSQ", response, AT_COMMAND_TIMEOUT) != 0) {
        return -1;
    }

    int rssi = -1;
    char *p = strstr(response, "+CSQ:");
    if (p) {
        sscanf(p, "+CSQ: %d,", &rssi);
        if (rssi == 99) {
            return -1;
        }
        return -113 + (rssi * 2);
    }

    return -1;
}

int at_get_network_registration(void)
{
    char response[MAX_BUFFER_SIZE];

    if (at_command_send("AT+CREG?", response, AT_COMMAND_TIMEOUT) != 0) {
        return -1;
    }

    int nreg = -1;
    char *p = strstr(response, "+CREG:");
    if (p) {
        sscanf(p, "+CREG: %*d,%d", &nreg);
    }

    return nreg;
}

int at_get_operator(char *operator_name, int len)
{
    char response[MAX_BUFFER_SIZE];

    if (at_command_send("AT+COPS?", response, AT_COMMAND_TIMEOUT) != 0) {
        return -1;
    }

    char *p = strstr(response, "+COPS:");
    if (p) {
        char *start = strchr(p, '"');
        char *end = strrchr(p, '"');
        if (start && end && start != end) {
            int slen = end - start - 1;
            if (slen < len) {
                strncpy(operator_name, start + 1, slen);
                operator_name[slen] = '\0';
                return 0;
            }
        }
    }

    return -1;
}

int at_get_oper_list(void)
{
    char response[MAX_BUFFER_SIZE];

    at_command_exec("AT+COPS=?");

    return 0;
}

int at_set_apn(const char *apn, const char *username, const char *password, int auth_type)
{
    at_command_exec("AT+CGDCONT=1,\"IP\",\"%s\"", apn);

    if (auth_type > 0) {
        at_command_exec("AT+CGAUTH=1,1,%d,\"%s\",\"%s\"", auth_type, password, username);
    }

    return 0;
}

int at_activate_pdp_context(void)
{
    char response[MAX_BUFFER_SIZE];

    if (at_command_send("AT+CGACT=1,1", response, 5000) != 0) {
        return -1;
    }

    if (strstr(response, "OK")) {
        return 0;
    }

    return -1;
}

int at_deactivate_pdp_context(void)
{
    char response[MAX_BUFFER_SIZE];

    if (at_command_send("AT+CGACT=0,1", response, 5000) != 0) {
        return -1;
    }

    return 0;
}

int at_get_imsi(char *imsi, int len)
{
    char response[MAX_BUFFER_SIZE];

    if (at_command_send("AT+CIMI", response, AT_COMMAND_TIMEOUT) != 0) {
        return -1;
    }

    char *p = response;
    while (*p && !isdigit(*p)) p++;

    if (*p) {
        int i = 0;
        while (*p && isdigit(*p) && i < len - 1) {
            imsi[i++] = *p++;
        }
        imsi[i] = '\0';
        return 0;
    }

    return -1;
}

int at_get_iccid(char *iccid, int len)
{
    char response[MAX_BUFFER_SIZE];

    if (at_command_send("AT+QCCID", response, AT_COMMAND_TIMEOUT) != 0) {
        return -1;
    }

    char *p = strstr(response, "+QCCID:");
    if (p) {
        sscanf(p, "+QCCID: %s", iccid);
        return 0;
    }

    return -1;
}

int at_get_imei(char *imei, int len)
{
    char response[MAX_BUFFER_SIZE];

    if (at_command_send("AT+GSN", response, AT_COMMAND_TIMEOUT) != 0) {
        return -1;
    }

    char *p = response;
    while (*p && !isdigit(*p)) p++;

    if (*p) {
        int i = 0;
        while (*p && isdigit(*p) && i < len - 1) {
            imei[i++] = *p++;
        }
        imei[i] = '\0';
        return 0;
    }

    return -1;
}

int at_set_network_mode(int mode)
{
    switch (mode) {
        case NETWORK_5G_SA:
            at_command_exec("AT+CNMP=71");
            break;
        case NETWORK_5G_SA_NSA:
            at_command_exec("AT+CNMP=70");
            break;
        case NETWORK_LTE:
            at_command_exec("AT+CNMP=61");
            break;
        default:
            at_command_exec("AT+CNMP=2");
    }

    return 0;
}

int at_get_network_mode(void)
{
    char response[MAX_BUFFER_SIZE];

    if (at_command_send("AT+CNMP?", response, AT_COMMAND_TIMEOUT) != 0) {
        return -1;
    }

    int mode = -1;
    char *p = strstr(response, "+CNMP:");
    if (p) {
        sscanf(p, "+CNMP: %d", &mode);
    }

    return mode;
}

int at_get_cell_info(char *response, int len)
{
    return at_command_send("AT+QENG=\"servingcell\"", response, AT_COMMAND_TIMEOUT);
}

int at_get_signal_quality(void)
{
    char response[MAX_BUFFER_SIZE];

    if (at_command_send("AT+QCSQ", response, AT_COMMAND_TIMEOUT) != 0) {
        return -1;
    }

    return 0;
}

int at_get_network_time(char *datetime, int len)
{
    char response[MAX_BUFFER_SIZE];

    if (at_command_send("AT+QLTS=2", response, AT_COMMAND_TIMEOUT) != 0) {
        return -1;
    }

    char *p = strstr(response, "+QLTS:");
    if (p) {
        char *start = strchr(p, '"');
        char *end = strchr(start + 1, '"');
        if (start && end) {
            int slen = end - start - 1;
            if (slen < len) {
                strncpy(datetime, start + 1, slen);
                datetime[slen] = '\0';
                return 0;
            }
        }
    }

    return -1;
}

int at_set_network_time_sync(int enable)
{
    if (enable) {
        at_command_exec("AT+QLTS=2");
    } else {
        at_command_exec("AT+QLTS=0");
    }

    return 0;
}

int at_get_pdp_context_state(void)
{
    char response[MAX_BUFFER_SIZE];

    if (at_command_send("AT+CGDCONT?", response, AT_COMMAND_TIMEOUT) != 0) {
        return -1;
    }

    return 0;
}

int at_get_apn_info(char *apn, int len)
{
    char response[MAX_BUFFER_SIZE];

    if (at_command_send("AT+CGDCONT?", response, AT_COMMAND_TIMEOUT) != 0) {
        return -1;
    }

    char *p = strstr(response, "+CGDCONT:");
    if (p) {
        char *start = strchr(p, '"');
        char *end = strchr(start + 1, '"');
        if (start && end) {
            int slen = end - start - 1;
            if (slen < len) {
                strncpy(apn, start + 1, slen);
                apn[slen] = '\0';
                return 0;
            }
        }
    }

    return -1;
}

int at_set_band(int mode, const char *bands)
{
    if (mode == 0) {
        at_command_exec("AT+QCFG=\"band\",%s", bands);
    } else {
        at_command_exec("AT+QCFG=\"band\",%s,%d", bands, mode);
    }

    return 0;
}

int at_get_band(char *band_info, int len)
{
    char response[MAX_BUFFER_SIZE];

    if (at_command_send("AT+QCFG=\"band\"", response, AT_COMMAND_TIMEOUT) != 0) {
        return -1;
    }

    strncpy(band_info, response, len - 1);
    return 0;
}

int at_set_auto_network_search(int enable)
{
    if (enable) {
        at_command_exec("AT+COPS=0");
    } else {
        at_command_exec("AT+COPS=1");
    }

    return 0;
}

int at_set_roaming(int enable)
{
    if (enable) {
        at_command_exec("AT+QCFG=\"roamservice\",1");
    } else {
        at_command_exec("AT+QCFG=\"roamservice\",0");
    }

    return 0;
}

int at_get_roaming_state(void)
{
    char response[MAX_BUFFER_SIZE];

    if (at_command_send("AT+QCFG=\"roamservice\"", response, AT_COMMAND_TIMEOUT) != 0) {
        return -1;
    }

    return 0;
}

int at_reboot(void)
{
    at_command_send("AT+CFUN=1,1", NULL, 0);

    return 0;
}

int at_factory_reset(void)
{
    at_command_exec("AT&F");

    return 0;
}

int at_set_airplane_mode(int enable)
{
    if (enable) {
        at_command_exec("AT+CFUN=0");
    } else {
        at_command_exec("AT+CFUN=1");
    }

    return 0;
}

int at_get_airplane_mode(void)
{
    char response[MAX_BUFFER_SIZE];

    if (at_command_send("AT+CFUN?", response, AT_COMMAND_TIMEOUT) != 0) {
        return -1;
    }

    int fun = -1;
    char *p = strstr(response, "+CFUN:");
    if (p) {
        sscanf(p, "+CFUN: %d", &fun);
    }

    return fun;
}

int at_sim_card_get_status(void)
{
    char response[MAX_BUFFER_SIZE];

    if (at_command_send("AT+QSIMSTAT?", response, AT_COMMAND_TIMEOUT) != 0) {
        return -1;
    }

    return 0;
}

int at_sim_card_detect(int *status)
{
    char response[MAX_BUFFER_SIZE];

    if (at_command_send("AT+QSIMSTAT=1", response, AT_COMMAND_TIMEOUT) != 0) {
        return -1;
    }

    char *p = strstr(response, "+QSIMSTAT:");
    if (p) {
        sscanf(p, "+QSIMSTAT: %d", status);
        return 0;
    }

    return -1;
}

int at_wlan_init(void)
{
    return at_command_exec("AT+QWIFI=1");
}

int at_wlan_deinit(void)
{
    return at_command_exec("AT+QWIFI=0");
}

int at_wlan_set_mode(int mode)
{
    return at_command_exec("AT+QWIFIMODE=%d", mode);
}

int at_wlan_set_ap(const char *ssid, const char *password, int channel, int encryption)
{
    if (encryption == 0) {
        return at_command_exec("AT+QAPCONFIG=1,\"%s\",%d,0", ssid, channel);
    } else {
        return at_command_exec("AT+QAPCONFIG=1,\"%s\",%d,4,\"%s\"", ssid, channel, password);
    }
}

int at_wlan_set_sta(const char *ssid, const char *password)
{
    return at_command_exec("AT+QWSTACONF=1,\"%s\",\"%s\"", ssid, password);
}

int at_wlan_sta_connect(void)
{
    return at_command_exec("AT+QWSTACONN=1");
}

int at_wlan_sta_disconnect(void)
{
    return at_command_exec("AT+QWSTACONN=0");
}

int at_wlan_sta_get_status(void)
{
    char response[MAX_BUFFER_SIZE];

    return at_command_send("AT+QWSTASTAT", response, AT_COMMAND_TIMEOUT);
}

int at_wlan_scan(const char *ssid)
{
    if (ssid) {
        return at_command_exec("AT+QSCAN=1,\"%s\"", ssid);
    } else {
        return at_command_exec("AT+QSCAN");
    }
}

int at_usb_config(int enable)
{
    if (enable) {
        return at_command_exec("AT+QUSBTCFG=1");
    } else {
        return at_command_exec("AT+QUSBTCFG=0");
    }

    return 0;
}

int at_get_temperature(int *temp)
{
    char response[MAX_BUFFER_SIZE];

    if (at_command_send("AT+QTEMP", response, AT_COMMAND_TIMEOUT) != 0) {
        return -1;
    }

    char *p = strstr(response, "+QTEMP:");
    if (p) {
        sscanf(p, "+QTEMP: %d", temp);
        return 0;
    }

    return -1;
}

int at_get_voltage(int *volt)
{
    char response[MAX_BUFFER_SIZE];

    if (at_command_send("AT+CBC", response, AT_COMMAND_TIMEOUT) != 0) {
        return -1;
    }

    char *p = strstr(response, "+CBC:");
    if (p) {
        sscanf(p, "+CBC: %*d,%*d,%d", volt);
        return 0;
    }

    return -1;
}

int at_get_version(char *version, int len)
{
    char response[MAX_BUFFER_SIZE];

    if (at_command_send("AT+CGMR", response, AT_COMMAND_TIMEOUT) != 0) {
        return -1;
    }

    char *p = response;
    while (*p && (*p == '\r' || *p == '\n' || *p == ' ')) p++;

    strncpy(version, p, len - 1);
    return 0;
}
