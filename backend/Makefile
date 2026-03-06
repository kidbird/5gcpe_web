CC ?= gcc
CROSS_COMPILE ?=

CC = $(CROSS_COMPILE)gcc
AR = $(CROSS_COMPILE)ar
STRIP = $(CROSS_COMPILE)strip

CFLAGS = -Wall -O2 -I./include
LDFLAGS = -lpthread

DEBUG ?= 0
ifeq ($(DEBUG), 1)
    CFLAGS += -g -DDEBUG
    STRIP_CMD = @echo "Debug mode, skipping strip"
else
    STRIP_CMD = $(STRIP) $(CGI_TARGET)
endif

SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build

CGI_SOURCES = $(SRC_DIR)/cgi_handler.c \
              $(SRC_DIR)/config.c \
              $(SRC_DIR)/api_backend.c \
              $(SRC_DIR)/at_command.c

CGI_TARGET = $(BUILD_DIR)/cpe_cgi

all: $(BUILD_DIR) $(CGI_TARGET)
	$(STRIP_CMD)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(CGI_TARGET): $(CGI_SOURCES)
	$(CC) $(CFLAGS) -o $@ $(CGI_SOURCES) $(LDFLAGS)

clean:
	rm -rf $(BUILD_DIR)

install: $(CGI_TARGET)
	mkdir -p $(DESTDIR)/www/cgi-bin
	mkdir -p $(DESTDIR)/www/html
	mkdir -p $(DESTDIR)/etc/config
	mkdir -p $(DESTDIR)/etc/lighttpd
	cp $(CGI_TARGET) $(DESTDIR)/www/cgi-bin/
	chmod +x $(DESTDIR)/www/cgi-bin/cpe_cgi
	cp -r ../css $(DESTDIR)/www/html/
	cp -r ../js $(DESTDIR)/www/html/
	cp ../index.html $(DESTDIR)/www/html/
	cp ../config/lighttpd.conf $(DESTDIR)/etc/lighttpd/ 2>/dev/null || true

.PHONY: all clean install

print-%:
	@echo $* = $($*)
