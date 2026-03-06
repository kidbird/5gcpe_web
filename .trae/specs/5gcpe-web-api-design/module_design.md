# 5G CPE Web配置系统 - 模块设计文档

## 目录
1. [系统架构概述](#系统架构概述)
2. [模块划分](#模块划分)
3. [CGI处理模块](#cgi处理模块)
4. [AT指令控制模块](#at指令控制模块)
5. [配置管理模块](#配置管理模块)
6. [WebSocket推送模块](#websocket推送模块)
7. [安全认证模块](#安全认证模块)
8. [数据结构定义](#数据结构定义)
9. [配置文件格式](#配置文件格式)
10. [lighttpd配置](#lighttpd配置)

---

## 系统架构概述

### 整体架构图

```
┌─────────────────────────────────────────────────────────────────────┐
│                           前端展示层                                  │
│  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐       │
│  │设备状态  │ │网络配置  │ │WLAN配置 │ │防火墙   │ │系统管理 │       │
│  └────┬────┘ └────┬────┘ └────┬────┘ └────┬────┘ └────┬────┘       │
│       │           │           │           │           │             │
│       └───────────┴───────────┴─────┬─────┴───────────┘             │
│                                     │ AJAX/WebSocket                │
└─────────────────────────────────────┼───────────────────────────────┘
                                      │
                                      ▼
┌─────────────────────────────────────────────────────────────────────┐
│                         HTTP服务层 (lighttpd)                        │
│  ┌──────────────────────┐    ┌──────────────────────┐              │
│  │   静态文件服务        │    │    CGI模块           │              │
│  │   /www/*.html        │    │   /www/cgi-bin/      │              │
│  │   /www/css/*.css     │    │   /www/cgi-bin/cpe_cgi│              │
│  │   /www/js/*.js       │    │                      │              │
│  └──────────────────────┘    └──────────┬───────────┘              │
└─────────────────────────────────────────┼───────────────────────────┘
                                          │
                                          ▼
┌─────────────────────────────────────────────────────────────────────┐
│                          CGI接口层                                   │
│  ┌──────────────────────────────────────────────────────────────┐  │
│  │                     cgi_handler.c                             │  │
│  │  ┌────────────┐ ┌────────────┐ ┌────────────┐ ┌────────────┐ │  │
│  │  │ 请求解析   │ │ 路由分发   │ │ 响应生成   │ │ JSON处理   │ │  │
│  │  └────────────┘ └────────────┘ └────────────┘ └────────────┘ │  │
│  └──────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────┼───────────────────────────┘
                                          │
                                          ▼
┌─────────────────────────────────────────────────────────────────────┐
│                          业务逻辑层                                  │
│  ┌──────────────┐ ┌──────────────┐ ┌──────────────┐                │
│  │ config.c     │ │ cpe_control.c│ │ 安全认证     │                │
│  │ config_json.c│ │              │ │              │                │
│  │ 配置读写     │ │ 设备控制     │ │ 会话管理     │                │
│  └──────┬───────┘ └──────┬───────┘ └──────────────┘                │
└─────────┼────────────────┼──────────────────────────────────────────┘
          │                │
          ▼                ▼
┌─────────────────┐ ┌─────────────────────────────────────────────────┐
│ 配置文件        │ │                  控制适配层                      │
│ /etc/cpe/       │ │  ┌──────────────┐  ┌──────────────┐            │
│ config.json     │ │  │ at_backend.c │  │ api_backend.c│            │
└─────────────────┘ │  │ AT指令方式   │  │ API接口方式  │            │
                    │  └──────┬───────┘  └──────┬───────┘            │
                    └─────────┼─────────────────┼─────────────────────┘
                              │                 │
                              ▼                 ▼
                    ┌─────────────────┐ ┌─────────────────┐
                    │   AT串口        │ │   API Socket    │
                    │  /dev/ttyUSB*   │ │   TCP:9000      │
                    └────────┬────────┘ └────────┬────────┘
                             │                   │
                             ▼                   ▼
                    ┌─────────────────────────────────────┐
                    │           5G模组                    │
                    │   (Quectel/Fibocom/Simcom/ZTE)     │
                    └─────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────┐
│                      WebSocket服务 (独立进程)                        │
│  ┌──────────────────────────────────────────────────────────────┐  │
│  │                      ws_server.c                              │  │
│  │  ┌────────────┐ ┌────────────┐ ┌────────────┐               │  │
│  │  │ WS连接管理 │ │ 状态推送   │ │ 事件广播   │               │  │
│  │  └────────────┘ └────────────┘ └────────────┘               │  │
│  └──────────────────────────────────────────────────────────────┘  │
│                           端口: 8888                                │
└─────────────────────────────────────────────────────────────────────┘
```

### 数据流向

```
用户操作 → 前端JS → AJAX请求 → lighttpd → CGI程序 → 业务逻辑 → 控制层 → 模组
                ↓
           JSON响应 ← CGI程序 ← 业务逻辑 ← 控制层 ← 模组响应
                ↓
           前端更新显示
```

---

## 模块划分

### 文件结构

```
5GCPE_web/
├── www/                        # Web根目录
│   ├── index.html             # 主页面
│   ├── css/
│   │   └── style.css          # 样式文件
│   ├── js/
│   │   └── app.js             # 前端逻辑
│   └── cgi-bin/
│       └── cpe_cgi            # CGI程序（编译后）
│
├── backend/
│   ├── include/               # 头文件
│   │   ├── cpe.h              # 主头文件，通用定义
│   │   ├── cpe_control.h      # 控制层接口定义
│   │   ├── module_at.h        # 模组AT指令表
│   │   └── websocket.h        # WebSocket定义
│   │
│   └── src/                   # 源文件
│       ├── cgi_handler.c      # CGI主入口，请求路由
│       ├── config.c           # 配置文件读写
│       ├── config_json.c      # JSON配置解析
│       ├── cpe_control.c      # 控制层管理
│       ├── at_backend.c       # AT指令后端实现
│       ├── api_backend.c      # API接口后端实现
│       ├── at_command.c       # AT指令底层操作
│       ├── module_at.c        # 模组AT指令表
│       ├── websocket.c        # WebSocket实现
│       └── ws_server.c        # WebSocket服务
│
├── config/                    # 配置文件目录（设备上）
│   ├── lighttpd.conf          # lighttpd配置
│   └── cpe/
│       └── config.json        # CPE配置文件
│
└── Makefile                   # 编译脚本
```

---

## CGI处理模块

### 模块职责
- 接收HTTP请求
- 解析请求参数和JSON数据
- 路由到对应的处理函数
- 生成JSON响应

### 核心函数

```c
/* cgi_handler.c */

/* 主入口函数 */
int main(int argc, char *argv[]);

/* 请求解析 */
int parse_query_string(const char *query, char *action, int len);
int parse_post_data(char *post_data, char *key, char *value, int max_len);
int parse_json_body(const char *json, const char *key, char *value, int max_len);

/* 响应生成 */
void cgi_response_json(int code, const char *message, const char *data);
void cgi_response_error(int code, const char *message);
void cgi_response_success(const char *data);

/* 请求处理器 */
typedef void (*request_handler_t)(void);
typedef void (*post_handler_t)(const char *json_data);

/* 路由表 */
typedef struct {
    const char *action;
    request_handler_t get_handler;
    post_handler_t post_handler;
} RouteEntry;

/* 各功能处理函数 */
void handle_status_request(void);
void handle_lan_get_request(void);
void handle_lan_set_request(const char *json);
void handle_cellular_get_request(void);
void handle_cellular_set_request(const char *json);
void handle_apn_list_request(void);
void handle_apn_add_request(const char *json);
void handle_apn_edit_request(const char *json);
void handle_apn_delete_request(const char *json);
void handle_apn_activate_request(const char *json);
void handle_wlan_ap_get_request(void);
void handle_wlan_ap_set_request(const char *json);
void handle_wlan_sta_get_request(void);
void handle_wlan_sta_set_request(const char *json);
void handle_wifi_scan_request(void);
void handle_firewall_get_request(void);
void handle_firewall_set_request(const char *json);
void handle_port_forward_add_request(const char *json);
void handle_port_forward_edit_request(const char *json);
void handle_port_forward_delete_request(const char *json);
void handle_vpn_get_request(void);
void handle_vpn_set_request(const char *json);
void handle_vpn_connect_request(const char *json);
void handle_vpn_disconnect_request(const char *json);
void handle_iot_get_request(void);
void handle_iot_set_request(const char *json);
void handle_system_get_request(void);
void handle_system_set_request(const char *json);
void handle_reboot_request(void);
void handle_factory_reset_request(void);
void handle_firmware_check_request(void);
void handle_firmware_upgrade_request(void);
void handle_at_send_request(const char *json);
void handle_login_request(const char *json);
void handle_logout_request(void);
void handle_password_change_request(const char *json);
```

### 路由表设计

```c
static const RouteEntry route_table[] = {
    /* 设备状态 */
    { "status",           handle_status_request,      NULL },
    { "traffic_stats",    handle_traffic_stats_request, NULL },
    { "device_list",      handle_device_list_request, NULL },
    
    /* LAN配置 */
    { "lan_get",          handle_lan_get_request,     NULL },
    { "lan_set",          NULL,                       handle_lan_set_request },
    
    /* 蜂窝网络 */
    { "cellular_get",     handle_cellular_get_request, NULL },
    { "cellular_set",     NULL,                       handle_cellular_set_request },
    { "cell_info",        handle_cell_info_request,   NULL },
    
    /* APN管理 */
    { "apn_list",         handle_apn_list_request,    NULL },
    { "apn_add",          NULL,                       handle_apn_add_request },
    { "apn_edit",         NULL,                       handle_apn_edit_request },
    { "apn_delete",       NULL,                       handle_apn_delete_request },
    { "apn_activate",     NULL,                       handle_apn_activate_request },
    
    /* WLAN配置 */
    { "wlan_ap_get",      handle_wlan_ap_get_request, NULL },
    { "wlan_ap_set",      NULL,                       handle_wlan_ap_set_request },
    { "wlan_sta_get",     handle_wlan_sta_get_request, NULL },
    { "wlan_sta_set",     NULL,                       handle_wlan_sta_set_request },
    { "wifi_scan",        handle_wifi_scan_request,   NULL },
    { "wlan_sta_connect", NULL,                       handle_wlan_sta_connect_request },
    { "wlan_sta_disconnect", NULL,                    handle_wlan_sta_disconnect_request },
    
    /* 防火墙 */
    { "firewall_get",     handle_firewall_get_request, NULL },
    { "firewall_set",     NULL,                       handle_firewall_set_request },
    { "port_forward_add", NULL,                       handle_port_forward_add_request },
    { "port_forward_edit", NULL,                      handle_port_forward_edit_request },
    { "port_forward_delete", NULL,                    handle_port_forward_delete_request },
    
    /* VPN */
    { "vpn_get",          handle_vpn_get_request,     NULL },
    { "vpn_set",          NULL,                       handle_vpn_set_request },
    { "vpn_connect",      NULL,                       handle_vpn_connect_request },
    { "vpn_disconnect",   NULL,                       handle_vpn_disconnect_request },
    
    /* IOT */
    { "iot_get",          handle_iot_get_request,     NULL },
    { "iot_set",          NULL,                       handle_iot_set_request },
    
    /* 系统管理 */
    { "system_get",       handle_system_get_request,  NULL },
    { "system_set",       NULL,                       handle_system_set_request },
    { "reboot",           NULL,                       handle_reboot_request },
    { "factory_reset",    NULL,                       handle_factory_reset_request },
    { "firmware_check",   handle_firmware_check_request, NULL },
    { "firmware_upgrade", NULL,                       handle_firmware_upgrade_request },
    { "config_export",    handle_config_export_request, NULL },
    { "config_import",    NULL,                       handle_config_import_request },
    
    /* AT调试 */
    { "at_send",          NULL,                       handle_at_send_request },
    { "at_commands",      handle_at_commands_request, NULL },
    
    /* 认证 */
    { "login",            NULL,                       handle_login_request },
    { "logout",           NULL,                       handle_logout_request },
    { "password_change",  NULL,                       handle_password_change_request },
    
    { NULL, NULL, NULL }  /* 结束标记 */
};
```

---

## AT指令控制模块

### 模块职责
- 管理AT串口通信
- 发送AT指令并解析响应
- 支持多种5G模组适配

### 核心数据结构

```c
/* 模组类型枚举 */
typedef enum {
    MODULE_TYPE_UNKNOWN = 0,
    MODULE_TYPE_QUECTEL_RG500U_SERIES,
    MODULE_TYPE_QUECTEL_RM500Q,
    MODULE_TYPE_FIBOCOM_FM150,
    MODULE_TYPE_FIBOCOM_FM350,
    MODULE_TYPE_SIMCOM_SIM8200,
    MODULE_TYPE_ZTE_MG8250,
    MODULE_TYPE_MAX
} ModuleType;

/* AT指令类型枚举 */
typedef enum {
    CMD_TEST = 0,
    CMD_GET_IMEI,
    CMD_GET_ICCID,
    CMD_GET_IMSI,
    CMD_GET_SIGNAL,
    CMD_GET_OPERATOR,
    CMD_GET_NETMODE,
    CMD_SET_NETMODE_AUTO,
    CMD_SET_NETMODE_5G_SA,
    CMD_SET_NETMODE_5G_NSA,
    CMD_SET_NETMODE_LTE,
    CMD_SET_APN,
    CMD_ACTIVATE_PDP,
    CMD_DEACTIVATE_PDP,
    CMD_GET_CELL_INFO,
    CMD_REBOOT,
    CMD_FACTORY_RESET,
    CMD_WLAN_ENABLE,
    CMD_WLAN_DISABLE,
    CMD_WLAN_SET_AP,
    CMD_WLAN_SET_STA,
    CMD_WLAN_SCAN,
    CMD_GET_TEMP,
    CMD_GET_VERSION,
    CMD_GET_RSRP,
    CMD_GET_SINR,
    CMD_SET_BAND,
    CMD_GET_BAND,
    CMD_MAX
} CommandType;

/* AT指令表项 */
typedef struct {
    const char *name;           /* 指令名称 */
    const char *at_cmd;         /* AT指令模板 */
    bool has_params;            /* 是否有参数 */
    const char *response_prefix;/* 响应前缀 */
} AtCommandEntry;

/* 模组AT指令表 */
typedef struct {
    ModuleType type;
    const char *name;
    const char *manufacturer;
    AtCommandEntry commands[CMD_MAX];
    bool supports_5g;
    bool supports_wifi;
    bool supports_gps;
} ModuleAtTable;
```

### 核心函数

```c
/* at_command.c */

/* 初始化AT串口 */
int at_command_init(const char *device);

/* 关闭AT串口 */
void at_command_close(void);

/* 发送AT指令 */
int at_command_send(const char *command, char *response, int timeout_ms);

/* 执行AT指令（格式化） */
int at_command_exec(const char *fmt, ...);

/* 获取信号强度 */
int at_get_signal_strength(void);

/* 获取网络注册状态 */
int at_get_network_registration(void);

/* 获取运营商名称 */
int at_get_operator(char *operator_name, int len);

/* 设置APN */
int at_set_apn(const char *apn, const char *username, const char *password, int auth_type);

/* 激活PDP上下文 */
int at_activate_pdp_context(void);

/* 去激活PDP上下文 */
int at_deactivate_pdp_context(void);

/* 获取IMSI */
int at_get_imsi(char *imsi, int len);

/* 获取ICCID */
int at_get_iccid(char *iccid, int len);

/* 获取IMEI */
int at_get_imei(char *imei, int len);

/* 设置网络模式 */
int at_set_network_mode(int mode);

/* 重启模组 */
int at_reboot(void);
```

```c
/* module_at.c */

/* 模组类型转换 */
const char* module_type_to_string(ModuleType type);
ModuleType module_type_from_string(const char *str);

/* 获取模组名称 */
const char* module_get_name(ModuleType type);

/* 获取AT指令 */
const char* module_get_at_command(ModuleType type, CommandType cmd);
const AtCommandEntry* module_get_command_entry(ModuleType type, CommandType cmd);

/* 构建AT指令 */
int module_build_command(ModuleType type, CommandType cmd, char *buf, int len, ...);

/* 功能支持检查 */
bool module_supports_5g(ModuleType type);
bool module_supports_wifi(ModuleType type);
bool module_supports_gps(ModuleType type);
```

### 模组AT指令表示例

```c
/* Quectel RG500U系列指令表 */
static const ModuleAtTable module_tables[] = {
    [MODULE_TYPE_QUECTEL_RG500U_SERIES] = {
        .type = MODULE_TYPE_QUECTEL_RG500U_SERIES,
        .name = "RG500U Series",
        .manufacturer = "Quectel",
        .supports_5g = true,
        .supports_wifi = true,
        .supports_gps = true,
        .commands = {
            [CMD_TEST]           = { "test",    "AT",           false, "" },
            [CMD_GET_IMEI]       = { "imei",    "AT+GSN",       false, "" },
            [CMD_GET_ICCID]      = { "iccid",   "AT+QCCID",     false, "+QCCID:" },
            [CMD_GET_IMSI]       = { "imsi",    "AT+CIMI",      false, "" },
            [CMD_GET_SIGNAL]     = { "signal",  "AT+CSQ",       false, "+CSQ:" },
            [CMD_GET_OPERATOR]   = { "operator","AT+COPS?",     false, "+COPS:" },
            [CMD_GET_NETMODE]    = { "netmode", "AT+CNMP?",     false, "+CNMP:" },
            [CMD_SET_NETMODE_AUTO] = { "auto",  "AT+CNMP=2",    false, "" },
            [CMD_SET_NETMODE_5G_SA] = { "5g_sa","AT+CNMP=71",   false, "" },
            [CMD_SET_NETMODE_5G_NSA] = { "5g_nsa","AT+CNMP=70", false, "" },
            [CMD_SET_NETMODE_LTE] = { "lte",    "AT+CNMP=61",   false, "" },
            [CMD_SET_APN]        = { "apn",     "AT+CGDCONT",   true,  "" },
            [CMD_ACTIVATE_PDP]   = { "pdp_on",  "AT+CGACT=1",   true,  "" },
            [CMD_DEACTIVATE_PDP] = { "pdp_off", "AT+CGACT=0",   true,  "" },
            [CMD_GET_CELL_INFO]  = { "cell",    "AT+QENG=\"servingcell\"", false, "+QENG:" },
            [CMD_REBOOT]         = { "reboot",  "AT+CFUN=1,1",  false, "" },
            [CMD_FACTORY_RESET]  = { "reset",   "AT&F",         false, "" },
            [CMD_WLAN_ENABLE]    = { "wifi_on", "AT+QWIFI=1",   false, "" },
            [CMD_WLAN_DISABLE]   = { "wifi_off","AT+QWIFI=0",   false, "" },
            [CMD_WLAN_SET_AP]    = { "ap_set",  "AT+QAPCONFIG", true,  "" },
            [CMD_WLAN_SET_STA]   = { "sta_set", "AT+QWSTACONF", true,  "" },
            [CMD_WLAN_SCAN]      = { "scan",    "AT+QWSCAN",    false, "+QWSCAN:" },
            [CMD_GET_TEMP]       = { "temp",    "AT+QTEMP",     false, "+QTEMP:" },
            [CMD_GET_RSRP]       = { "rsrp",    "AT+QCSQ",      false, "+QCSQ:" },
        }
    },
    /* 其他模组表... */
};
```

---

## 配置管理模块

### 模块职责
- 加载和保存配置文件
- 提供配置项的读写接口
- 配置项校验

### 核心数据结构

```c
/* config.c / config_json.c */

/* LAN配置 */
typedef struct {
    char    work_mode[16];      /* 工作模式 */
    char    ip[16];             /* IP地址 */
    char    netmask[16];        /* 子网掩码 */
    bool    dhcp_enabled;       /* DHCP开关 */
    char    dhcp_start[16];     /* DHCP起始IP */
    char    dhcp_end[16];       /* DHCP结束IP */
    int     dhcp_lease;         /* DHCP租期 */
    char    dns1[32];           /* 主DNS */
    char    dns2[32];           /* 备DNS */
} LanConfig;

/* APN配置 */
typedef struct {
    int     id;                 /* APN ID */
    char    name[64];           /* APN名称 */
    char    username[64];       /* 用户名 */
    char    password[64];       /* 密码 */
    int     auth_type;          /* 认证类型 */
    int     bearer_type;        /* 承载类型 */
    bool    is_default;         /* 是否默认 */
    bool    is_active;          /* 是否激活 */
    char    apn_types[64];      /* APN类型 */
    char    mcc[4];             /* MCC */
    char    mnc[4];             /* MNC */
} ApnConfig;

/* 蜂窝网络配置 */
typedef struct {
    int     network_mode;       /* 网络模式 */
    char    bands_5g[256];      /* 5G频段 */
    char    bands_lte[256];     /* LTE频段 */
    bool    airplane_mode;      /* 飞行模式 */
    bool    data_roaming;       /* 数据漫游 */
    bool    hw_accel;           /* 硬件加速 */
    bool    ims_enabled;        /* IMS开关 */
    char    preferred_plmn[8];  /* 首选PLMN */
} CellularConfig;

/* WLAN AP配置 */
typedef struct {
    bool    enabled_2g4;        /* 2.4G开关 */
    bool    enabled_5g;         /* 5G开关 */
    char    ssid_2g4[64];       /* 2.4G SSID */
    char    ssid_5g[64];        /* 5G SSID */
    char    password_2g4[64];   /* 2.4G密码 */
    char    password_5g[64];    /* 5G密码 */
    int     channel_2g4;        /* 2.4G信道 */
    int     channel_5g;         /* 5G信道 */
    int     encryption;         /* 加密方式 */
    bool    hidden_2g4;         /* 隐藏2.4G SSID */
    bool    hidden_5g;          /* 隐藏5G SSID */
    int     max_clients;        /* 最大客户端 */
    int     bandwidth_2g4;      /* 2.4G频宽 */
    int     bandwidth_5g;       /* 5G频宽 */
} WlanApConfig;

/* WLAN STA配置 */
typedef struct {
    bool    sta_enabled;        /* STA开关 */
    char    target_ssid[64];    /* 目标SSID */
    char    target_password[64];/* 目标密码 */
    int     security_type;      /* 安全类型 */
    int     band;               /* 频段 */
    int     wan_type;           /* WAN类型 */
    char    wan_mac[18];        /* WAN MAC */
    bool    nat_enabled;        /* NAT开关 */
} WlanStaConfig;

/* 端口转发规则 */
typedef struct {
    int     id;                 /* 规则ID */
    char    protocol[8];        /* 协议 */
    int     external_port;      /* 外部端口 */
    char    internal_ip[16];    /* 内部IP */
    int     internal_port;      /* 内部端口 */
    char    description[64];    /* 描述 */
    bool    enabled;            /* 是否启用 */
} PortForwardRule;

/* 防火墙配置 */
typedef struct {
    bool    enabled;            /* 防火墙开关 */
    bool    dmz_enabled;        /* DMZ开关 */
    char    dmz_ip[16];         /* DMZ IP */
    bool    spi_enabled;        /* SPI开关 */
    bool    dos_enabled;        /* DoS防护 */
    bool    arp_proxy;          /* ARP代理 */
    bool    ping_wan;           /* WAN Ping */
    PortForwardRule port_forwards[32]; /* 端口转发 */
    int     port_forward_count; /* 规则数量 */
} FirewallConfig;

/* VPN配置 */
typedef struct {
    char    type[16];           /* VPN类型 */
    bool    enabled;            /* 开关 */
    char    server[128];        /* 服务器 */
    char    username[64];       /* 用户名 */
    char    password[64];       /* 密码 */
    /* ... 其他VPN参数 */
} VpnConfig;

/* IOT配置 */
typedef struct {
    bool    enabled;            /* IOT开关 */
    char    protocol[16];       /* 协议 */
    char    server[128];        /* 服务器 */
    int     port;               /* 端口 */
    char    client_id[64];      /* 客户端ID */
    char    username[64];       /* 用户名 */
    char    password[64];       /* 密码 */
    int     keepalive;          /* 心跳间隔 */
    int     qos;                /* QoS */
    char    publish_topic[128]; /* 发布主题 */
    char    subscribe_topic[128];/* 订阅主题 */
} IotConfig;

/* 系统配置 */
typedef struct {
    char    device_name[64];    /* 设备名称 */
    char    timezone[64];       /* 时区 */
    char    ntp_server[128];    /* NTP服务器 */
    int     web_port;           /* Web端口 */
    char    admin_user[32];     /* 管理员用户名 */
    char    admin_pass[128];    /* 管理员密码(加密) */
    bool    https_enabled;      /* HTTPS开关 */
    bool    remote_manage;      /* 远程管理 */
    char    language[16];       /* 语言 */
} SystemConfig;

/* 完整配置 */
typedef struct {
    LanConfig       lan;
    CellularConfig  cellular;
    ApnConfig       apns[MAX_APN_COUNT];
    int             apn_count;
    WlanApConfig    wlan_ap;
    WlanStaConfig   wlan_sta;
    FirewallConfig  firewall;
    VpnConfig       vpn[4];     /* PPTP/L2TP, GRE, EOIP, IPSec */
    IotConfig       iot;
    SystemConfig    system;
} CpeConfig;
```

### 核心函数

```c
/* config.c */

/* 加载配置 */
int config_load(const char *config_file);

/* 保存配置 */
int config_save(const char *config_file);

/* LAN配置 */
int config_get_lan(LanConfig *config);
int config_set_lan(LanConfig *config);

/* APN配置 */
int config_get_apn_list(ApnConfig *apns, int *count);
int config_set_apn(ApnConfig *apn);
int config_delete_apn(int apn_id);
int config_get_active_apn(ApnConfig *apn);

/* 蜂窝网络配置 */
int config_get_cellular(CellularConfig *config);
int config_set_cellular(CellularConfig *config);

/* WLAN配置 */
int config_get_wlan_ap(WlanApConfig *config);
int config_set_wlan_ap(WlanApConfig *config);
int config_get_wlan_sta(WlanStaConfig *config);
int config_set_wlan_sta(WlanStaConfig *config);

/* 防火墙配置 */
int config_get_firewall(FirewallConfig *config);
int config_set_firewall(FirewallConfig *config);
int config_add_port_forward(PortForwardRule *rule);
int config_edit_port_forward(PortForwardRule *rule);
int config_delete_port_forward(int rule_id);

/* VPN配置 */
int config_get_vpn(const char *type, VpnConfig *config);
int config_set_vpn(VpnConfig *config);

/* IOT配置 */
int config_get_iot(IotConfig *config);
int config_set_iot(IotConfig *config);

/* 系统配置 */
int config_get_system(SystemConfig *config);
int config_set_system(SystemConfig *config);
```

```c
/* config_json.c */

/* JSON解析 */
int json_parse_lan(const char *json, LanConfig *config);
int json_parse_apn(const char *json, ApnConfig *config);
int json_parse_cellular(const char *json, CellularConfig *config);
int json_parse_wlan_ap(const char *json, WlanApConfig *config);
int json_parse_wlan_sta(const char *json, WlanStaConfig *config);
int json_parse_firewall(const char *json, FirewallConfig *config);
int json_parse_vpn(const char *json, VpnConfig *config);
int json_parse_iot(const char *json, IotConfig *config);
int json_parse_system(const char *json, SystemConfig *config);

/* JSON生成 */
char* json_generate_lan(LanConfig *config);
char* json_generate_apn_list(ApnConfig *apns, int count);
char* json_generate_cellular(CellularConfig *config);
char* json_generate_wlan_ap(WlanApConfig *config);
char* json_generate_wlan_sta(WlanStaConfig *config);
char* json_generate_firewall(FirewallConfig *config);
char* json_generate_vpn(VpnConfig *config);
char* json_generate_iot(IotConfig *config);
char* json_generate_system(SystemConfig *config);

/* 从文件加载 */
int config_load_json(const char *file, CpeConfig *config);

/* 保存到文件 */
int config_save_json(const char *file, CpeConfig *config);
```

---

## WebSocket推送模块

### 模块职责
- 维护WebSocket连接
- 定期推送状态数据
- 广播事件通知

### 核心数据结构

```c
/* websocket.h */

/* WebSocket客户端 */
typedef struct WsClient {
    int                 fd;
    struct sockaddr_in  addr;
    time_t              connect_time;
    time_t              last_ping;
    char                subsciptions[256];  /* 订阅的主题 */
    struct WsClient     *next;
} WsClient;

/* WebSocket服务器 */
typedef struct {
    int                 listen_fd;
    int                 port;
    WsClient            *clients;
    int                 client_count;
    int                 max_clients;
    pthread_mutex_t     mutex;
    bool                running;
    pthread_t           thread;
} WsServer;

/* 推送消息类型 */
typedef enum {
    WS_MSG_STATUS_UPDATE,
    WS_MSG_TRAFFIC_UPDATE,
    WS_MSG_SIGNAL_UPDATE,
    WS_MSG_EVENT,
    WS_MSG_PING,
    WS_MSG_PONG
} WsMessageType;

/* 推送消息 */
typedef struct {
    WsMessageType   type;
    time_t          timestamp;
    char            data[4096];
} WsMessage;
```

### 核心函数

```c
/* websocket.c */

/* 初始化WebSocket服务器 */
int ws_server_init(WsServer *server, int port);

/* 启动服务器 */
int ws_server_start(WsServer *server);

/* 停止服务器 */
void ws_server_stop(WsServer *server);

/* 处理新连接 */
int ws_handle_new_connection(WsServer *server);

/* 处理客户端消息 */
int ws_handle_client_message(WsServer *server, WsClient *client);

/* 广播消息 */
int ws_broadcast(WsServer *server, const char *message);

/* 发送消息给指定客户端 */
int ws_send(WsClient *client, const char *message);

/* 移除断开的客户端 */
void ws_remove_client(WsServer *server, WsClient *client);

/* 生成状态推送消息 */
char* ws_generate_status_message(void);

/* 生成事件消息 */
char* ws_generate_event_message(const char *event, const char *message, const char *severity);
```

```c
/* ws_server.c */

/* 主服务循环 */
void *ws_server_loop(void *arg);

/* 状态推送线程 */
void *ws_status_pusher(void *arg);

/* 事件处理 */
void ws_on_device_connected(const char *ip, const char *mac);
void ws_on_device_disconnected(const char *ip);
void ws_on_network_changed(const char *old_type, const char *new_type);
void ws_on_vpn_status_changed(const char *type, bool connected);
```

---

## 安全认证模块

### 模块职责
- 用户登录认证
- 会话管理
- 密码加密存储

### 核心数据结构

```c
/* 会话信息 */
typedef struct {
    char    session_id[64];
    char    username[32];
    time_t  create_time;
    time_t  expire_time;
    char    client_ip[16];
} Session;

/* 会话管理器 */
typedef struct {
    Session sessions[16];
    int     session_count;
    pthread_mutex_t mutex;
    int     session_timeout;    /* 秒 */
} SessionManager;
```

### 核心函数

```c
/* auth.c */

/* 密码加密 */
int password_encrypt(const char *plain, char *encrypted, int len);

/* 密码验证 */
int password_verify(const char *plain, const char *encrypted);

/* 创建会话 */
Session* session_create(const char *username, const char *client_ip);

/* 验证会话 */
int session_validate(const char *session_id, const char *client_ip);

/* 刷新会话 */
void session_refresh(const char *session_id);

/* 销毁会话 */
void session_destroy(const char *session_id);

/* 清理过期会话 */
void session_cleanup(void);

/* 检查登录状态 */
int check_login_status(void);
```

---

## 配置文件格式

### /etc/cpe/config.json

```json
{
    "version": "1.0",
    "lan": {
        "work_mode": "router",
        "ip": "192.168.1.1",
        "netmask": "255.255.255.0",
        "dhcp_enabled": true,
        "dhcp_start": "192.168.1.100",
        "dhcp_end": "192.168.1.200",
        "dhcp_lease": 86400,
        "dns1": "8.8.8.8",
        "dns2": "8.8.4.4"
    },
    "cellular": {
        "network_mode": 0,
        "bands_5g": ["n1", "n3", "n5", "n7", "n8", "n78"],
        "bands_lte": ["B1", "B3", "B5", "B7", "B8"],
        "airplane_mode": false,
        "data_roaming": false,
        "hw_accel": true,
        "ims_enabled": true
    },
    "apns": [
        {
            "id": 1,
            "name": "cmnet",
            "username": "",
            "password": "",
            "auth_type": 0,
            "bearer_type": 1,
            "is_default": true,
            "is_active": true,
            "apn_types": ["default"]
        }
    ],
    "wlan": {
        "ap": {
            "enabled_2g4": true,
            "enabled_5g": true,
            "ssid_2g4": "CPE_5G_24G",
            "ssid_5g": "CPE_5G_5G",
            "password_2g4": "encrypted:xxxx",
            "password_5g": "encrypted:xxxx",
            "channel_2g4": 0,
            "channel_5g": 0,
            "encryption": 4,
            "hidden_2g4": false,
            "hidden_5g": false,
            "max_clients": 32,
            "bandwidth_2g4": 20,
            "bandwidth_5g": 80
        },
        "sta": {
            "sta_enabled": false,
            "target_ssid": "",
            "target_password": "",
            "security_type": 4,
            "band": 0,
            "wan_type": 0,
            "wan_mac": "00:1A:2B:3C:4D:5E",
            "nat_enabled": true
        }
    },
    "firewall": {
        "enabled": true,
        "dmz_enabled": false,
        "dmz_ip": "",
        "spi_enabled": true,
        "dos_enabled": true,
        "arp_proxy": false,
        "ping_wan": false,
        "port_forwards": []
    },
    "vpn": {
        "pptp": {
            "enabled": false,
            "server": "",
            "username": "",
            "password": ""
        },
        "gre": {
            "enabled": false,
            "local_ip": "",
            "remote_ip": "",
            "tunnel_key": ""
        },
        "eoip": {
            "enabled": false,
            "tunnel_id": 1,
            "remote_ip": "",
            "tunnel_key": ""
        },
        "ipsec": {
            "enabled": false,
            "remote_gateway": "",
            "preshared_key": ""
        }
    },
    "iot": {
        "enabled": false,
        "protocol": "MQTT",
        "server": "",
        "port": 1883,
        "client_id": "",
        "username": "",
        "password": "",
        "keepalive": 60,
        "qos": 1,
        "publish_topic": "",
        "subscribe_topic": ""
    },
    "system": {
        "device_name": "5G CPE",
        "timezone": "Asia/Shanghai",
        "ntp_server": "pool.ntp.org",
        "web_port": 80,
        "admin_user": "admin",
        "admin_pass": "encrypted:xxxx",
        "https_enabled": false,
        "remote_manage": false,
        "language": "zh_CN"
    }
}
```

### /etc/cpe/cpe.conf

```ini
# CPE控制配置
mode=at
at_device=/dev/ttyUSB2
at_baudrate=115200
api_host=127.0.0.1
api_port=9000
debug=0

# WebSocket配置
ws_port=8888
ws_push_interval=5

# 会话配置
session_timeout=1800
```

---

## lighttpd配置

### /etc/lighttpd/lighttpd.conf

```conf
# 基本配置
server.document-root = "/www"
server.port = 80
server.username = "root"
server.groupname = "root"
server.pid-file = "/var/run/lighttpd.pid"

# 日志配置
server.errorlog = "/var/log/lighttpd/error.log"
accesslog.filename = "/var/log/lighttpd/access.log"

# 索引文件
index-file.names = ( "index.html" )

# MIME类型
mimetype.assign = (
    ".html" => "text/html",
    ".css" => "text/css",
    ".js" => "application/javascript",
    ".json" => "application/json",
    ".png" => "image/png",
    ".jpg" => "image/jpeg",
    ".gif" => "image/gif",
    ".svg" => "image/svg+xml",
    ".ico" => "image/x-icon"
)

# 加载模块
server.modules = (
    "mod_access",
    "mod_accesslog",
    "mod_cgi",
    "mod_setenv"
)

# CGI配置
cgi.assign = ( ".cgi" => "", "cpe_cgi" => "" )
alias.url = ( "/cgi-bin/" => "/www/cgi-bin/" )

# 跨域支持
setenv.add-response-header = (
    "Access-Control-Allow-Origin" => "*",
    "Access-Control-Allow-Methods" => "GET, POST, OPTIONS",
    "Access-Control-Allow-Headers" => "Content-Type"
)

# 静态文件缓存
static-file.exclude-extensions = ( ".cgi", ".fcgi" )

# 压缩
compress.cache-dir = "/var/cache/lighttpd/compress/"
compress.filetype = ( "text/html", "text/css", "application/javascript" )
```

### 启动脚本

```sh
#!/bin/sh
# /etc/init.d/lighttpd

case "$1" in
    start)
        echo "Starting lighttpd..."
        mkdir -p /var/log/lighttpd
        mkdir -p /var/cache/lighttpd/compress
        lighttpd -f /etc/lighttpd/lighttpd.conf
        ;;
    stop)
        echo "Stopping lighttpd..."
        killall lighttpd
        ;;
    restart)
        $0 stop
        sleep 1
        $0 start
        ;;
    *)
        echo "Usage: $0 {start|stop|restart}"
        exit 1
        ;;
esac
```
