#include "cpe.h"

static int at_fd = -1;
static pthread_mutex_t at_mutex = PTHREAD_MUTEX_INITIALIZER;

int at_command_init(const char *device)
{
    if (at_fd >= 0) {
        close(at_fd);
    }
    
    const char *dev = device ? device : "/dev/ttyUSB2";
    
    at_fd = open(dev, O_RDWR | O_NOCTTY | O_NONBLOCK);
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
    
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_cflag &= ~CRTSCTS;
    tty.c_cflag |= CREAD | CLOCAL;
    
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);
    
    tty.c_oflag &= ~OPOST;
    tty.c_oflag &= ~ONLCR;
    
    tty.c_lflag &= ~ICANON;
    tty.c_lflag &= ~ECHO;
    tty.c_lflag &= ~ECHOE;
    tty.c_lflag &= ~ECHONL;
    tty.c_lflag &= ~ISIG;
    
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 1;
    
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
    if (at_fd < 0) {
        if (at_command_init(NULL) < 0) {
            return -1;
        }
    }
    
    pthread_mutex_lock(&at_mutex);
    
    char cmd_buf[256];
    snprintf(cmd_buf, sizeof(cmd_buf), "%s\r\n", command);
    
    tcflush(at_fd, TCIOFLUSH);
    
    int written = write(at_fd, cmd_buf, strlen(cmd_buf));
    if (written != (int)strlen(cmd_buf)) {
        pthread_mutex_unlock(&at_mutex);
        return -1;
    }
    
    if (response) {
        memset(response, 0, MAX_BUFFER_SIZE);
        
        fd_set read_fds;
        struct timeval tv;
        int total_read = 0;
        int timeout_remaining = timeout_ms;
        
        while (timeout_remaining > 0) {
            FD_ZERO(&read_fds);
            FD_SET(at_fd, &read_fds);
            
            tv.tv_sec = timeout_remaining / 1000;
            tv.tv_usec = (timeout_remaining % 1000) * 1000;
            
            int ret = select(at_fd + 1, &read_fds, NULL, NULL, &tv);
            if (ret <= 0) {
                break;
            }
            
            int bytes_read = read(at_fd, response + total_read, 
                                  MAX_BUFFER_SIZE - total_read - 1);
            if (bytes_read > 0) {
                total_read += bytes_read;
                response[total_read] = '\0';
                
                if (strstr(response, "OK") || strstr(response, "ERROR") ||
                    strstr(response, "+CME ERROR") || strstr(response, "+CMS ERROR")) {
                    break;
                }
            }
            
            timeout_remaining -= (timeout_ms - tv.tv_sec * 1000 - tv.tv_usec / 1000);
        }
    }
    
    pthread_mutex_unlock(&at_mutex);
    return 0;
}

int at_command_exec(const char *fmt, ...)
{
    char command[256];
    va_list args;
    
    va_start(args, fmt);
    vsnprintf(command, sizeof(command), fmt, args);
    va_end(args);
    
    char response[MAX_BUFFER_SIZE];
    int ret = at_command_send(command, response, AT_COMMAND_TIMEOUT);
    
    if (ret == 0 && strstr(response, "OK")) {
        return 0;
    }
    
    return -1;
}

int at_get_signal_strength(void)
{
    char response[MAX_BUFFER_SIZE];
    
    if (at_command_send("AT+CSQ", response, AT_COMMAND_TIMEOUT) == 0) {
        int rssi, ber;
        if (sscanf(response, "+CSQ: %d,%d", &rssi, &ber) == 2) {
            return (rssi == 99) ? 0 : -113 + rssi * 2;
        }
    }
    
    return -1;
}

int at_get_network_registration(void)
{
    char response[MAX_BUFFER_SIZE];
    
    if (at_command_send("AT+CREG?", response, AT_COMMAND_TIMEOUT) == 0) {
        int n, stat;
        if (sscanf(response, "+CREG: %d,%d", &n, &stat) == 2) {
            return stat;
        }
    }
    
    return -1;
}

int at_get_operator(char *operator_name, int len)
{
    if (!operator_name || len <= 0) return -1;
    
    char response[MAX_BUFFER_SIZE];
    
    if (at_command_send("AT+COPS?", response, AT_COMMAND_TIMEOUT) == 0) {
        char *p = strstr(response, "+COPS:");
        if (p) {
            int mode, format;
            char oper[64] = {0};
            
            if (sscanf(p, "+COPS: %d,%d,\"%63[^\"]\"", &mode, &format, oper) >= 3) {
                strncpy(operator_name, oper, len - 1);
                return 0;
            }
        }
    }
    
    strcpy(operator_name, "Unknown");
    return -1;
}

int at_set_apn(const char *apn, const char *username, const char *password, int auth_type)
{
    char command[256];
    char response[MAX_BUFFER_SIZE];
    
    snprintf(command, sizeof(command), "AT+CGDCONT=1,\"IP\",\"%s\"", apn);
    if (at_command_send(command, response, AT_COMMAND_TIMEOUT) != 0) {
        return -1;
    }
    
    if (username && strlen(username) > 0) {
        snprintf(command, sizeof(command), "AT+CGAUTH=1,%d,\"%s\",\"%s\"", 
                 auth_type, password ? password : "", username);
        at_command_send(command, response, AT_COMMAND_TIMEOUT);
    }
    
    return 0;
}

int at_activate_pdp_context(void)
{
    char response[MAX_BUFFER_SIZE];
    return at_command_send("AT+CGACT=1,1", response, 10000);
}

int at_deactivate_pdp_context(void)
{
    char response[MAX_BUFFER_SIZE];
    return at_command_send("AT+CGACT=1,0", response, AT_COMMAND_TIMEOUT);
}

int at_get_imsi(char *imsi, int len)
{
    if (!imsi || len <= 0) return -1;
    
    char response[MAX_BUFFER_SIZE];
    
    if (at_command_send("AT+CIMI", response, AT_COMMAND_TIMEOUT) == 0) {
        char *p = response;
        while (*p && (*p == '\r' || *p == '\n' || *p == ' ')) p++;
        
        if (isdigit(*p)) {
            int i = 0;
            while (isdigit(*p) && i < len - 1) {
                imsi[i++] = *p++;
            }
            imsi[i] = '\0';
            return 0;
        }
    }
    
    strcpy(imsi, "Unknown");
    return -1;
}

int at_get_iccid(char *iccid, int len)
{
    if (!iccid || len <= 0) return -1;
    
    char response[MAX_BUFFER_SIZE];
    
    if (at_command_send("AT+QCCID", response, AT_COMMAND_TIMEOUT) == 0) {
        char *p = strstr(response, "+QCCID:");
        if (p) {
            p += 7;
            while (*p && (*p == ' ' || *p == '\t')) p++;
            
            int i = 0;
            while (isdigit(*p) && i < len - 1) {
                iccid[i++] = *p++;
            }
            iccid[i] = '\0';
            return 0;
        }
    }
    
    strcpy(iccid, "Unknown");
    return -1;
}

int at_get_imei(char *imei, int len)
{
    if (!imei || len <= 0) return -1;
    
    char response[MAX_BUFFER_SIZE];
    
    if (at_command_send("AT+GSN", response, AT_COMMAND_TIMEOUT) == 0) {
        char *p = response;
        while (*p && (*p == '\r' || *p == '\n' || *p == ' ')) p++;
        
        if (isdigit(*p)) {
            int i = 0;
            while (isdigit(*p) && i < len - 1) {
                imei[i++] = *p++;
            }
            imei[i] = '\0';
            return 0;
        }
    }
    
    strcpy(imei, "Unknown");
    return -1;
}

int at_set_network_mode(int mode)
{
    char command[64];
    char response[MAX_BUFFER_SIZE];
    
    switch (mode) {
        case NETWORK_5G_SA:
            strcpy(command, "AT+CNMP=71");
            break;
        case NETWORK_5G_SA_NSA:
            strcpy(command, "AT+CNMP=70");
            break;
        case NETWORK_LTE:
            strcpy(command, "AT+CNMP=61");
            break;
        default:
            strcpy(command, "AT+CNMP=2");
            break;
    }
    
    return at_command_send(command, response, AT_COMMAND_TIMEOUT);
}

int at_reboot(void)
{
    char response[MAX_BUFFER_SIZE];
    return at_command_send("AT+CFUN=1,1", response, AT_COMMAND_TIMEOUT);
}
