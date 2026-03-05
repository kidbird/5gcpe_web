#include "websocket.h"
#include "cpe_control.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <errno.h>

static volatile int g_running = 1;

static void signal_handler(int sig)
{
    g_running = 0;
}

typedef struct {
    int id;
    int fd;
    WSState state;
    char ip[32];
} WSClientInfo;

static WSClientInfo *g_clients[WS_MAX_CLIENTS] = {0};
static int g_client_count = 0;
static CpeControlOps *g_cpe_ops = NULL;

static int find_free_client_slot(void)
{
    for (int i = 0; i < WS_MAX_CLIENTS; i++) {
        if (!g_clients[i]) return i;
    }
    return -1;
}

static void remove_client(int client_id)
{
    if (client_id >= 0 && client_id < WS_MAX_CLIENTS && g_clients[client_id]) {
        if (g_clients[client_id]->fd >= 0) {
            close(g_clients[client_id]->fd);
        }
        free(g_clients[client_id]);
        g_clients[client_id] = NULL;
        g_client_count--;
        printf("Client %d disconnected. Total: %d\n", client_id, g_client_count);
    }
}

static int send_to_client(int client_id, const char *message)
{
    if (client_id < 0 || client_id >= WS_MAX_CLIENTS) return -1;
    WSClientInfo *client = g_clients[client_id];
    if (!client || client->state != WS_STATE_OPEN) return -1;
    
    int frame_len;
    char *frame = ws_encode_frame(message, &frame_len);
    if (!frame) return -1;
    
    int sent = send(client->fd, frame, frame_len, 0);
    free(frame);
    
    return sent;
}

static void broadcast_message(const char *message)
{
    for (int i = 0; i < WS_MAX_CLIENTS; i++) {
        if (g_clients[i] && g_clients[i]->state == WS_STATE_OPEN) {
            send_to_client(i, message);
        }
    }
}

static void handle_client_message(int client_id, const char *message)
{
    printf("Client %d message: %s\n", client_id, message);
    
    char response[WS_BUFFER_SIZE] = {0};
    
    if (strncmp(message, "get_status", 10) == 0) {
        DeviceStatus status;
        if (g_cpe_ops && g_cpe_ops->get_device_status) {
            g_cpe_ops->get_device_status(&status);
            snprintf(response, sizeof(response),
                "{\"type\":\"status\",\"device_model\":\"%s\",\"firmware_version\":\"%s\","
                "\"imei\":\"%s\",\"signal_strength\":%d,\"network_type\":\"%s\",\"operator\":\"%s\"}",
                status.device_model, status.firmware_version, status.imei,
                status.signal_strength, status.network_type, status.operator_name);
        }
    }
    else if (strncmp(message, "get_signal", 10) == 0) {
        int rssi = 0;
        if (g_cpe_ops && g_cpe_ops->get_signal_strength) {
            g_cpe_ops->get_signal_strength(&rssi);
            snprintf(response, sizeof(response), "{\"type\":\"signal\",\"rssi\":%d}", rssi);
        }
    }
    else if (strncmp(message, "set_apn:", 8) == 0) {
        const char *apn = message + 8;
        if (g_cpe_ops && g_cpe_ops->set_apn) {
            g_cpe_ops->set_apn(apn, "", "", 0);
            snprintf(response, sizeof(response), "{\"type\":\"result\",\"action\":\"set_apn\",\"apn\":\"%s\",\"success\":true}", apn);
        }
    }
    else if (strncmp(message, "activate_pdp", 12) == 0) {
        if (g_cpe_ops && g_cpe_ops->activate_pdp) {
            g_cpe_ops->activate_pdp(1);
            snprintf(response, sizeof(response), "{\"type\":\"result\",\"action\":\"activate_pdp\",\"success\":true}");
        }
    }
    else if (strncmp(message, "reboot", 6) == 0) {
        if (g_cpe_ops && g_cpe_ops->reboot) {
            g_cpe_ops->reboot();
            snprintf(response, sizeof(response), "{\"type\":\"result\",\"action\":\"reboot\",\"success\":true}");
        }
    }
    else {
        snprintf(response, sizeof(response), "{\"type\":\"error\",\"message\":\"Unknown command\"}");
    }
    
    if (response[0]) {
        send_to_client(client_id, response);
    }
}

static int process_client_data(int client_id, const char *data, int len)
{
    WSFrame frame;
    int processed = ws_decode_frame(data, len, &frame);
    
    if (processed < 0) return -1;
    
    switch (frame.opcode) {
        case WS_OPCODE_TEXT: {
            char *message = malloc(frame.payload_len + 1);
            memcpy(message, frame.data, frame.payload_len);
            message[frame.payload_len] = '\0';
            
            handle_client_message(client_id, message);
            
            free(message);
            break;
        }
        
        case WS_OPCODE_CLOSE:
            printf("Client %d sent close frame\n", client_id);
            remove_client(client_id);
            return -1;
            
        case WS_OPCODE_PING: {
            char pong_frame[16];
            pong_frame[0] = 0x8A;
            pong_frame[1] = frame.payload_len;
            if (frame.payload_len > 0 && frame.masked) {
                memcpy(pong_frame + 2, frame.data, frame.payload_len);
            }
            WSClientInfo *client = g_clients[client_id];
            if (client && client->fd >= 0) {
                send(client->fd, pong_frame, 2 + frame.payload_len, 0);
            }
            break;
        }
        
        default:
            break;
    }
    
    if (frame.data != (uint8_t*)data + processed && frame.data != (uint8_t*)data) {
        free(frame.data);
    }
    
    return processed;
}

static int accept_new_client(int listen_fd)
{
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    int client_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_len);
    if (client_fd < 0) {
        return -1;
    }
    
    int slot = find_free_client_slot();
    if (slot < 0) {
        close(client_fd);
        printf("Max clients reached, rejecting connection\n");
        return -1;
    }
    
    WSClientInfo *client = malloc(sizeof(WSClientInfo));
    if (!client) {
        close(client_fd);
        return -1;
    }
    
    client->id = slot;
    client->fd = client_fd;
    client->state = WS_STATE_CONNECTING;
    strncpy(client->ip, inet_ntoa(client_addr.sin_addr), sizeof(client->ip) - 1);
    
    g_clients[slot] = client;
    g_client_count++;
    
    printf("New client %d from %s. Total: %d\n", slot, client->ip, g_client_count);
    
    if (ws_handshake(client_fd, NULL) < 0) {
        close(client_fd);
        free(client);
        g_clients[slot] = NULL;
        g_client_count--;
        return -1;
    }
    
    client->state = WS_STATE_OPEN;
    
    const char *welcome = "{\"type\":\"welcome\",\"message\":\"Connected to 5G CPE WebSocket Server\"}";
    send_to_client(slot, welcome);
    
    return slot;
}

static int run_event_loop(WSServer *server)
{
    fd_set read_fds;
    int max_fd = server->listen_fd;
    
    while (g_running) {
        FD_ZERO(&read_fds);
        FD_SET(server->listen_fd, &read_fds);
        
        for (int i = 0; i < WS_MAX_CLIENTS; i++) {
            if (g_clients[i] && g_clients[i]->fd >= 0) {
                FD_SET(g_clients[i]->fd, &read_fds);
                if (g_clients[i]->fd > max_fd) {
                    max_fd = g_clients[i]->fd;
                }
            }
        }
        
        struct timeval tv = {1, 0};
        int ready = select(max_fd + 1, &read_fds, NULL, NULL, &tv);
        
        if (ready < 0) {
            if (errno == EINTR) continue;
            break;
        }
        
        if (FD_ISSET(server->listen_fd, &read_fds)) {
            accept_new_client(server->listen_fd);
            ready--;
        }
        
        for (int i = 0; ready > 0 && i < WS_MAX_CLIENTS; i++) {
            if (g_clients[i] && g_clients[i]->fd >= 0 && 
                FD_ISSET(g_clients[i]->fd, &read_fds)) {
                
                char buffer[WS_BUFFER_SIZE];
                int n = recv(g_clients[i]->fd, buffer, sizeof(buffer) - 1, 0);
                
                if (n <= 0) {
                    remove_client(i);
                } else {
                    buffer[n] = '\0';
                    while (process_client_data(i, buffer, n) > 0) {}
                }
                
                ready--;
            }
        }
    }
    
    return 0;
}

static void print_usage(const char *prog)
{
    printf("Usage: %s [options]\n", prog);
    printf("Options:\n");
    printf("  -p <port>    WebSocket port (default: %d)\n", WS_PORT_DEFAULT);
    printf("  -c <config>  CPE config file\n");
    printf("  -d           Daemon mode\n");
    printf("  -h           Show this help\n");
}

int main(int argc, char *argv[])
{
    int ws_port = WS_PORT_DEFAULT;
    char *config_file = NULL;
    int daemon_mode = 0;
    
    int opt;
    while ((opt = getopt(argc, argv, "p:c:dh")) != -1) {
        switch (opt) {
            case 'p':
                ws_port = atoi(optarg);
                break;
            case 'c':
                config_file = optarg;
                break;
            case 'd':
                daemon_mode = 1;
                break;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    CpeControlMode mode = CPE_CTRL_MODE_AT;
    if (config_file) {
        CpeConfig config;
        if (cpe_config_load(config_file, &config) == 0) {
            mode = config.mode;
        }
    }
    
    g_cpe_ops = cpe_control_create(mode);
    if (g_cpe_ops) {
        cpe_control_init(g_cpe_ops, config_file);
        printf("CPE control initialized in %s mode\n", cpe_control_get_mode_name(mode));
    }
    
    WSServer *server = ws_server_create(ws_port);
    if (!server) {
        fprintf(stderr, "Failed to create WebSocket server\n");
        return 1;
    }
    
    if (ws_server_start(server) < 0) {
        fprintf(stderr, "Failed to start WebSocket server on port %d\n", ws_port);
        return 1;
    }
    
    if (daemon_mode) {
        daemon(0, 0);
    }
    
    printf("5G CPE WebSocket Server running...\n");
    printf("Connect to: ws://<server-ip>:%d\n", ws_port);
    
    run_event_loop(server);
    
    ws_server_stop(server);
    ws_server_destroy(server);
    
    if (g_cpe_ops) {
        cpe_control_destroy(g_cpe_ops);
    }
    
    printf("Server stopped.\n");
    return 0;
}
