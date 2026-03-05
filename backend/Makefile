CC = gcc
CFLAGS = -Wall -Wextra -O2 -I./include
LDFLAGS = -lpthread

SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = www/cgi-bin

SOURCES = $(SRC_DIR)/cgi_handler.c $(SRC_DIR)/at_command.c $(SRC_DIR)/config_json.c \
          $(SRC_DIR)/cpe_control.c $(SRC_DIR)/at_backend.c $(SRC_DIR)/api_backend.c \
          $(SRC_DIR)/websocket.c $(SRC_DIR)/ws_server.c $(SRC_DIR)/module_at.c

OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
TARGET = $(BIN_DIR)/cpe_cgi
WS_TARGET = $(BIN_DIR)/ws_server

.PHONY: all clean install ws

all: $(TARGET) $(WS_TARGET)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(SRC_DIR)/cgi_handler.c $(SRC_DIR)/at_command.c $(SRC_DIR)/config_json.c $(SRC_DIR)/cpe_control.c $(SRC_DIR)/at_backend.c $(SRC_DIR)/api_backend.c $(SRC_DIR)/module_at.c
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(WS_TARGET): $(SRC_DIR)/websocket.c $(SRC_DIR)/ws_server.c $(SRC_DIR)/cpe_control.c $(SRC_DIR)/at_backend.c $(SRC_DIR)/api_backend.c $(SRC_DIR)/module_at.c
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $(SRC_DIR)/websocket.c $(SRC_DIR)/ws_server.c $(SRC_DIR)/cpe_control.c $(SRC_DIR)/at_backend.c $(SRC_DIR)/api_backend.c $(SRC_DIR)/module_at.c -o $@ $(LDFLAGS)

ws: $(WS_TARGET)

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

install: $(TARGET) $(WS_TARGET)
	cp $(TARGET) /usr/lib/cgi-bin/
	cp $(WS_TARGET) /usr/bin/
	chmod 755 /usr/lib/cgi-bin/cpe_cgi
	chmod 755 /usr/bin/ws_server
	@mkdir -p /etc/cpe
	@echo "# CPE Control Configuration" > /etc/cpe/cpe.conf
	@echo "mode=at" >> /etc/cpe/cpe.conf
	@echo "at_device=/dev/ttyUSB2" >> /etc/cpe/cpe.conf
	@echo "api_host=127.0.0.1" >> /etc/cpe/cpe.conf
	@echo "api_port=9000" >> /etc/cpe/cpe.conf
	@echo "debug=0" >> /etc/cpe/cpe.conf
	@echo "" >> /etc/cpe/cpe.conf
	@echo "# WebSocket Configuration" >> /etc/cpe/cpe.conf
	@echo "ws_port=8888" >> /etc/cpe/cpe.conf

arm64:
	@echo "ARM64 cross-compilation requires aarch64-linux-gnu-gcc toolchain"
	@echo "Please install it or cross-compile on your build machine"

help:
	@echo "5G CPE Backend Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all       - Build CGI and WebSocket server (default)"
	@echo "  ws        - Build only WebSocket server"
	@echo "  arm64     - Build for ARM64 (Yocto Linux)"
	@echo "  clean     - Remove build artifacts"
	@echo "  install   - Install CGI and WebSocket server"
	@echo ""
	@echo "Usage:"
	@echo "  make              # Build for local machine"
	@echo "  make ws           # Build WebSocket server only"
	@echo "  make arm64        # Cross-compile for ARM64"
	@echo "  make clean        # Clean build files"
	@echo "  make install      # Install to web server"
	@echo ""
	@echo "WebSocket Server Options:"
	@echo "  -p <port>    WebSocket port (default: 8888)"
	@echo "  -c <config>  CPE config file"
	@echo "  -d           Daemon mode"
