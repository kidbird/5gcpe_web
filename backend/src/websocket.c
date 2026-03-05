#include "websocket.h"
#include "cpe_control.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/select.h>
#include <arpa/inet.h>

static const char *WS_MAGIC_STRING = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

static const char base64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static void sha1_hash(const char *input, int len, unsigned char *output)
{
    unsigned int hash[5] = {0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0};
    
    unsigned int w[80];
    int i, j;
    
    unsigned char *data = (unsigned char *)input;
    unsigned int padded_len = ((len + 8) / 64 + 1) * 64;
    unsigned char *padded = calloc(padded_len, 1);
    memcpy(padded, data, len);
    padded[len] = 0x80;
    
    unsigned long long bit_len = (unsigned long long)len * 8;
    for (i = 0; i < 8; i++) {
        padded[padded_len - 1 - i] = (bit_len >> (i * 8)) & 0xFF;
    }
    
    for (i = 0; i < padded_len / 64; i++) {
        for (j = 0; j < 16; j++) {
            w[j] = (padded[i*64 + j*4] << 24) | (padded[i*64 + j*4 + 1] << 16) |
                   (padded[i*64 + j*4 + 2] << 8) | padded[i*64 + j*4 + 3];
        }
        for (j = 16; j < 80; j++) {
            unsigned int val = w[j-3] ^ w[j-8] ^ w[j-14] ^ w[j-16];
            w[j] = (val << 1) | (val >> 31);
        }
        
        unsigned int a = hash[0], b = hash[1], c = hash[2], d = hash[3], e = hash[4];
        
        for (j = 0; j < 80; j++) {
            unsigned int f, k;
            if (j < 20) {
                f = (b & c) | ((~b) & d);
                k = 0x5A827999;
            } else if (j < 40) {
                f = b ^ c ^ d;
                k = 0x6ED9EBA1;
            } else if (j < 60) {
                f = (b & c) | (b & d) | (c & d);
                k = 0x8F1BBCDC;
            } else {
                f = b ^ c ^ d;
                k = 0xCA62C1D6;
            }
            
            unsigned int temp = ((a << 5) | (a >> 27)) + f + e + k + w[j];
            e = d;
            d = c;
            c = ((b << 30) | (b >> 2));
            b = a;
            a = temp;
        }
        
        hash[0] += a; hash[1] += b; hash[2] += c; hash[3] += d; hash[4] += e;
    }
    
    free(padded);
    
    for (i = 0; i < 5; i++) {
        output[i*4] = (hash[i] >> 24) & 0xFF;
        output[i*4+1] = (hash[i] >> 16) & 0xFF;
        output[i*4+2] = (hash[i] >> 8) & 0xFF;
        output[i*4+3] = hash[i] & 0xFF;
    }
}

static char* base64_encode_simple(const unsigned char *input, int length)
{
    int i, j;
    char *result = malloc((length + 2) / 3 * 4 + 1);
    
    for (i = 0, j = 0; i < length; i += 3) {
        int a = input[i];
        int b = (i + 1 < length) ? input[i + 1] : 0;
        int c = (i + 2 < length) ? input[i + 2] : 0;
        
        result[j++] = base64_table[(a >> 2) & 0x3F];
        result[j++] = base64_table[((a << 4) | (b >> 4)) & 0x3F];
        result[j++] = (i + 1 < length) ? base64_table[((b << 2) | (c >> 6)) & 0x3F] : '=';
        result[j++] = (i + 2 < length) ? base64_table[c & 0x3F] : '=';
    }
    
    result[j] = '\0';
    return result;
}

int ws_handshake(int client_fd, const char *host)
{
    char buffer[WS_BUFFER_SIZE];
    char key[WS_KEY_LEN];
    char response[512];
    
    int n = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if (n <= 0) return -1;
    buffer[n] = '\0';
    
    char *ptr = strstr(buffer, "Sec-WebSocket-Key:");
    if (!ptr) return -1;
    
    ptr += 18;
    while (*ptr == ' ') ptr++;
    
    char *end = strchr(ptr, '\r');
    if (!end) return -1;
    
    int key_len = end - ptr;
    if (key_len >= WS_KEY_LEN) key_len = WS_KEY_LEN - 1;
    strncpy(key, ptr, key_len);
    key[key_len] = '\0';
    
    char accept[128];
    snprintf(accept, sizeof(accept), "%s%s", key, WS_MAGIC_STRING);
    
    unsigned char hash[20];
    sha1_hash(accept, strlen(accept), hash);
    
    char *encoded = base64_encode_simple(hash, 20);
    
    snprintf(response, sizeof(response),
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Accept: %s\r\n"
        "\r\n",
        encoded);
    
    send(client_fd, response, strlen(response), 0);
    free(encoded);
    
    return 0;
}

char* ws_encode_frame(const char *message, int *out_len)
{
    int msg_len = strlen(message);
    int frame_len = 2 + msg_len;
    if (msg_len > 125) frame_len += 2;
    if (msg_len > 65535) frame_len += 6;
    
    char *frame = malloc(frame_len + 1);
    if (!frame) return NULL;
    
    frame[0] = 0x81;
    
    if (msg_len <= 125) {
        frame[1] = msg_len;
        memcpy(frame + 2, message, msg_len);
    } else if (msg_len <= 65535) {
        frame[1] = 126;
        frame[2] = (msg_len >> 8) & 0xFF;
        frame[3] = msg_len & 0xFF;
        memcpy(frame + 4, message, msg_len);
    } else {
        frame[1] = 127;
        frame[2] = 0;
        frame[3] = 0;
        frame[4] = 0;
        frame[5] = 0;
        frame[6] = (msg_len >> 24) & 0xFF;
        frame[7] = (msg_len >> 16) & 0xFF;
        frame[8] = (msg_len >> 8) & 0xFF;
        frame[9] = msg_len & 0xFF;
        memcpy(frame + 10, message, msg_len);
    }
    
    *out_len = frame_len;
    return frame;
}

char* ws_encode_binary(const void *data, int len, int *out_len)
{
    int frame_len = 2 + len;
    if (len > 125) frame_len += 2;
    if (len > 65535) frame_len += 6;
    
    char *frame = malloc(frame_len + 1);
    if (!frame) return NULL;
    
    frame[0] = 0x82;
    
    if (len <= 125) {
        frame[1] = len;
        memcpy(frame + 2, data, len);
    } else if (len <= 65535) {
        frame[1] = 126;
        frame[2] = (len >> 8) & 0xFF;
        frame[3] = len & 0xFF;
        memcpy(frame + 4, data, len);
    } else {
        frame[1] = 127;
        frame[2] = 0;
        frame[3] = 0;
        frame[4] = 0;
        frame[5] = 0;
        frame[6] = (len >> 24) & 0xFF;
        frame[7] = (len >> 16) & 0xFF;
        frame[8] = (len >> 8) & 0xFF;
        frame[9] = len & 0xFF;
        memcpy(frame + 10, data, len);
    }
    
    *out_len = frame_len;
    return frame;
}

static uint64_t parse_uint64(const char *p)
{
    uint64_t val = 0;
    for (int i = 0; i < 8; i++) {
        val = (val << 8) | ((uint8_t)p[i] & 0xFF);
    }
    return val;
}

int ws_decode_frame(const char *input, int in_len, WSFrame *frame)
{
    if (in_len < 2) return -1;
    
    frame->fin = (input[0] & 0x80) != 0;
    frame->opcode = input[0] & 0x0F;
    frame->masked = (input[1] & 0x80) != 0;
    
    uint64_t payload_len = input[1] & 0x7F;
    int offset = 2;
    
    if (payload_len == 126) {
        if (in_len < 4) return -1;
        payload_len = (input[2] << 8) | input[3];
        offset = 4;
    } else if (payload_len == 127) {
        if (in_len < 10) return -1;
        payload_len = parse_uint64(input + 2);
        offset = 10;
    }
    
    frame->payload_len = payload_len;
    
    if (frame->masked) {
        if (in_len < offset + 4) return -1;
        memcpy(frame->mask, input + offset, 4);
        offset += 4;
    }
    
    if (in_len < offset + payload_len) return -1;
    
    if (frame->masked && payload_len > 0) {
        frame->data = malloc(payload_len);
        if (!frame->data) return -1;
        
        for (uint64_t i = 0; i < payload_len; i++) {
            frame->data[i] = input[offset + i] ^ frame->mask[i % 4];
        }
    } else {
        frame->data = (uint8_t *)(input + offset);
    }
    
    return offset + payload_len;
}

WSServer* ws_server_create(int port)
{
    WSServer *server = malloc(sizeof(WSServer));
    if (!server) return NULL;
    
    memset(server, 0, sizeof(WSServer));
    server->port = port > 0 ? port : WS_PORT_DEFAULT;
    server->max_clients = WS_MAX_CLIENTS;
    
    return server;
}

void ws_server_destroy(WSServer *server)
{
    if (!server) return;
    
    for (int i = 0; i < server->max_clients; i++) {
        if (server->clients[i]) {
            if (server->clients[i]->fd >= 0) {
                close(server->clients[i]->fd);
            }
            free(server->clients[i]);
        }
    }
    
    if (server->listen_fd >= 0) {
        close(server->listen_fd);
    }
    
    free(server);
}

static int set_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int ws_server_start(WSServer *server)
{
    server->listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server->listen_fd < 0) return -1;
    
    int opt = 1;
    setsockopt(server->listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(server->port);
    
    if (bind(server->listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(server->listen_fd);
        return -1;
    }
    
    if (listen(server->listen_fd, 10) < 0) {
        close(server->listen_fd);
        return -1;
    }
    
    set_nonblocking(server->listen_fd);
    
    printf("WebSocket server started on port %d\n", server->port);
    
    return 0;
}

void ws_server_stop(WSServer *server)
{
    if (server->listen_fd >= 0) {
        close(server->listen_fd);
        server->listen_fd = -1;
    }
}

int ws_server_send_text(int client_id, const char *message)
{
    return 0;
}

int ws_server_send_binary(int client_id, const void *data, int len)
{
    return 0;
}

int ws_server_broadcast_text(const char *message)
{
    return 0;
}

void ws_server_set_message_callback(WSServer *server, void (*cb)(int, const char*, char*))
{
    server->message_callback = cb;
}

void ws_server_set_connect_callback(WSServer *server, void (*cb)(int))
{
    server->client_connect_callback = cb;
}

void ws_server_set_disconnect_callback(WSServer *server, void (*cb)(int))
{
    server->client_disconnect_callback = cb;
}
