#ifndef WEBSOCKET_H
#define WEBSOCKET_H

#include <stdint.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define WS_MAX_CLIENTS       32
#define WS_BUFFER_SIZE      8192
#define WS_KEY_LEN          64
#define WS_PORT_DEFAULT     8888

typedef enum {
    WS_OPCODE_CONTINUE = 0x0,
    WS_OPCODE_TEXT     = 0x1,
    WS_OPCODE_BINARY   = 0x2,
    WS_OPCODE_CLOSE    = 0x8,
    WS_OPCODE_PING     = 0x9,
    WS_OPCODE_PONG     = 0xA
} WSOpcode;

typedef enum {
    WS_STATE_CONNECTING = 0,
    WS_STATE_OPEN       = 1,
    WS_STATE_CLOSING    = 2,
    WS_STATE_CLOSED     = 3
} WSState;

typedef struct {
    int fd;
    WSState state;
    char client_ip[32];
    uint8_t mask_key[4];
    bool has_mask;
} WSClient;

typedef struct {
    int listen_fd;
    int port;
    int max_clients;
    WSClient *clients[WS_MAX_CLIENTS];
    void (*message_callback)(int client_id, const char *message, char *response);
    void (*client_connect_callback)(int client_id);
    void (*client_disconnect_callback)(int client_id);
} WSServer;

typedef struct {
    uint8_t opcode;
    bool fin;
    bool masked;
    uint64_t payload_len;
    uint8_t mask[4];
    uint8_t *data;
} WSFrame;

WSServer* ws_server_create(int port);
void ws_server_destroy(WSServer *server);
int ws_server_start(WSServer *server);
void ws_server_stop(WSServer *server);
int ws_server_send_text(int client_id, const char *message);
int ws_server_send_binary(int client_id, const void *data, int len);
int ws_server_broadcast_text(const char *message);
void ws_server_set_message_callback(WSServer *server, void (*cb)(int, const char*, char*));
void ws_server_set_connect_callback(WSServer *server, void (*cb)(int));
void ws_server_set_disconnect_callback(WSServer *server, void (*cb)(int));

char* ws_encode_frame(const char *message, int *out_len);
char* ws_encode_binary(const void *data, int len, int *out_len);

int ws_handshake(int client_fd, const char *host);
int ws_decode_frame(const char *input, int in_len, WSFrame *frame);

#endif
