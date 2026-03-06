# 5G CPE Web配置系统规格说明

## Why
为5G CPE设备提供一套完整的Web前端配置界面和后台配置服务，使用户能够通过浏览器方便地管理和配置设备，包括网络设置、APN管理、WLAN配置、防火墙设置、VPN配置、IOT网关和系统管理等功能。

## What Changes
- 完善前端JavaScript与后端CGI的JSON数据接口
- 定义完整的API接口数据格式
- 设计后台服务架构（基于lighttpd）
- 完善CGI接口实现（C语言开发）
- 实现模块化的控制层设计

## Impact
- Affected specs: 前端页面交互、后端API接口、配置存储、模组控制
- Affected code: `backend/src/cgi_handler.c`, `backend/src/api_backend.c`, `js/app.js`, 配置文件格式

---

## ADDED Requirements

### Requirement: 整体系统架构
系统 SHALL 采用三层架构设计：前端展示层、CGI接口层、后台服务层。

#### Scenario: 系统架构层次
- **WHEN** 用户访问Web配置页面
- **THEN** 前端通过AJAX调用CGI接口
- **AND** CGI接口调用后台服务处理业务逻辑
- **AND** 后台服务通过AT指令或API控制模组

#### Scenario: 技术栈
- **WHEN** 系统部署运行
- **THEN** HTTP服务使用lighttpd
- **AND** CGI程序使用C语言开发
- **AND** 前端使用原生JavaScript
- **AND** 数据交换格式为JSON

---

### Requirement: HTTP服务器配置（lighttpd）
系统 SHALL 使用lighttpd作为HTTP服务器，支持CGI和静态文件服务。

#### Scenario: lighttpd配置
- **WHEN** lighttpd服务启动
- **THEN** 监听80端口（可配置）
- **AND** 静态文件目录指向/www
- **AND** CGI程序目录指向/www/cgi-bin
- **AND** 支持跨域请求（CORS）

#### Scenario: CGI配置
- **WHEN** 请求到达CGI程序
- **THEN** 正确设置环境变量
- **AND** 支持POST和GET方法
- **AND** 正确处理Content-Type

---

### Requirement: 统一API响应格式
系统 SHALL 使用统一的JSON响应格式。

#### Scenario: 成功响应格式
- **WHEN** API调用成功
- **THEN** 返回格式为：
```json
{
    "code": 200,
    "message": "success",
    "data": { ... }
}
```

#### Scenario: 错误响应格式
- **WHEN** API调用失败
- **THEN** 返回格式为：
```json
{
    "code": 500,
    "message": "错误描述",
    "data": null
}
```

#### Scenario: HTTP状态码定义
- **WHEN** 处理请求
- **THEN** 使用以下状态码：
  - 200: 成功
  - 400: 请求参数错误
  - 404: 资源不存在
  - 500: 服务器内部错误

---

### Requirement: 设备状态API
系统 SHALL 提供设备状态查询接口。

#### Scenario: 获取设备状态
- **WHEN** GET请求 `/cgi-bin/cpe_cgi?action=status`
- **THEN** 返回设备完整状态信息：
```json
{
    "code": 200,
    "message": "success",
    "data": {
        "device_model": "CPE-5G01",
        "firmware_version": "V2.1.5",
        "imei": "861234567890123",
        "iccid": "8986012345678901234",
        "lan_mac": "00:1A:2B:3C:4D:5E",
        "wan_ip": "10.23.45.67",
        "wan_ipv6": "240e:1234:abcd::1",
        "uptime": "3天 14时 32分",
        "signal_strength": -72,
        "network_type": "5G NR",
        "operator_name": "中国移动",
        "rsrp": -72,
        "sinr": 15,
        "tx_bytes": "125.4 MB",
        "rx_bytes": "892.7 MB"
    }
}
```

---

### Requirement: LAN配置API
系统 SHALL 提供LAN网络配置接口。

#### Scenario: 获取LAN配置
- **WHEN** GET请求 `/cgi-bin/cpe_cgi?action=lan_get`
- **THEN** 返回LAN配置：
```json
{
    "code": 200,
    "message": "success",
    "data": {
        "work_mode": "router",
        "ip": "192.168.1.1",
        "netmask": "255.255.255.0",
        "dhcp_enabled": true,
        "dhcp_start": "192.168.1.100",
        "dhcp_end": "192.168.1.200",
        "dhcp_lease": 86400,
        "dns1": "8.8.8.8",
        "dns2": "8.8.4.4"
    }
}
```

#### Scenario: 设置LAN配置
- **WHEN** POST请求 `/cgi-bin/cpe_cgi?action=lan_set`
- **AND** 请求体包含JSON数据
- **THEN** 更新LAN配置并返回结果

---

### Requirement: 蜂窝网络配置API
系统 SHALL 提供蜂窝网络配置接口。

#### Scenario: 获取网络模式配置
- **WHEN** GET请求 `/cgi-bin/cpe_cgi?action=cellular_get`
- **THEN** 返回网络配置：
```json
{
    "code": 200,
    "message": "success",
    "data": {
        "network_mode": 0,
        "network_mode_str": "自动（5G SA+NSA/LTE）",
        "bands_5g": ["n1", "n3", "n5", "n7", "n8", "n78"],
        "bands_lte": ["B1", "B3", "B5", "B7", "B8"],
        "airplane_mode": false,
        "data_roaming": false,
        "hw_accel": true,
        "ims_enabled": true
    }
}
```

#### Scenario: 设置网络模式
- **WHEN** POST请求 `/cgi-bin/cpe_cgi?action=cellular_set`
- **THEN** 更新网络模式配置

---

### Requirement: APN管理API
系统 SHALL 提供APN管理接口，支持增删改查和激活操作。

#### Scenario: 获取APN列表
- **WHEN** GET请求 `/cgi-bin/cpe_cgi?action=apn_list`
- **THEN** 返回APN列表：
```json
{
    "code": 200,
    "message": "success",
    "data": [
        {
            "id": 1,
            "name": "cmnet",
            "username": "",
            "password": "",
            "auth_type": 0,
            "bearer_type": 1,
            "is_default": true,
            "is_active": true
        }
    ]
}
```

#### Scenario: 添加APN
- **WHEN** POST请求 `/cgi-bin/cpe_cgi?action=apn_add`
- **AND** 请求体包含APN配置
- **THEN** 创建新APN并返回结果

#### Scenario: 编辑APN
- **WHEN** POST请求 `/cgi-bin/cpe_cgi?action=apn_edit`
- **THEN** 更新指定APN配置

#### Scenario: 删除APN
- **WHEN** POST请求 `/cgi-bin/cpe_cgi?action=apn_delete`
- **AND** 请求包含apn_id
- **THEN** 删除指定APN

#### Scenario: 激活APN
- **WHEN** POST请求 `/cgi-bin/cpe_cgi?action=apn_activate`
- **AND** 请求包含apn_id
- **THEN** 激活指定APN并建立PDP上下文

---

### Requirement: WLAN配置API
系统 SHALL 提供WLAN AP和STA模式配置接口。

#### Scenario: 获取WLAN AP配置
- **WHEN** GET请求 `/cgi-bin/cpe_cgi?action=wlan_ap_get`
- **THEN** 返回WLAN AP配置：
```json
{
    "code": 200,
    "message": "success",
    "data": {
        "enabled_2g4": true,
        "enabled_5g": true,
        "ssid_2g4": "CPE_5G_24G",
        "ssid_5g": "CPE_5G_5G",
        "password_2g4": "********",
        "password_5g": "********",
        "channel_2g4": 0,
        "channel_5g": 0,
        "encryption": 4
    }
}
```

#### Scenario: 设置WLAN AP配置
- **WHEN** POST请求 `/cgi-bin/cpe_cgi?action=wlan_ap_set`
- **THEN** 更新WLAN AP配置

#### Scenario: 获取WLAN STA配置
- **WHEN** GET请求 `/cgi-bin/cpe_cgi?action=wlan_sta_get`
- **THEN** 返回STA模式配置

#### Scenario: 设置WLAN STA配置
- **WHEN** POST请求 `/cgi-bin/cpe_cgi?action=wlan_sta_set`
- **THEN** 更新STA模式配置

#### Scenario: WiFi扫描
- **WHEN** GET请求 `/cgi-bin/cpe_cgi?action=wifi_scan`
- **THEN** 返回扫描到的WiFi列表：
```json
{
    "code": 200,
    "message": "success",
    "data": {
        "scan_time": "2024-01-15 10:30:00",
        "networks": [
            {
                "ssid": "MyWiFi",
                "bssid": "AA:BB:CC:DD:EE:FF",
                "signal": 85,
                "channel": 6,
                "security": "WPA2"
            }
        ]
    }
}
```

---

### Requirement: 防火墙配置API
系统 SHALL 提供防火墙和端口转发配置接口。

#### Scenario: 获取防火墙配置
- **WHEN** GET请求 `/cgi-bin/cpe_cgi?action=firewall_get`
- **THEN** 返回防火墙配置：
```json
{
    "code": 200,
    "message": "success",
    "data": {
        "enabled": true,
        "dmz_enabled": false,
        "dmz_ip": "",
        "spi_enabled": true,
        "dos_enabled": true,
        "arp_proxy": false,
        "port_forwards": [
            {
                "id": 1,
                "protocol": "TCP",
                "external_port": 8080,
                "internal_ip": "192.168.1.100",
                "internal_port": 80,
                "description": "Web Server",
                "enabled": true
            }
        ]
    }
}
```

#### Scenario: 设置防火墙配置
- **WHEN** POST请求 `/cgi-bin/cpe_cgi?action=firewall_set`
- **THEN** 更新防火墙配置

#### Scenario: 添加端口转发
- **WHEN** POST请求 `/cgi-bin/cpe_cgi?action=port_forward_add`
- **THEN** 添加端口转发规则

#### Scenario: 删除端口转发
- **WHEN** POST请求 `/cgi-bin/cpe_cgi?action=port_forward_delete`
- **THEN** 删除指定端口转发规则

---

### Requirement: VPN配置API
系统 SHALL 提供VPN配置接口（PPTP/L2TP/GRE/EOIP/IPSec）。

#### Scenario: 获取VPN配置
- **WHEN** GET请求 `/cgi-bin/cpe_cgi?action=vpn_get&type=pptp`
- **THEN** 返回指定类型VPN配置：
```json
{
    "code": 200,
    "message": "success",
    "data": {
        "type": "pptp",
        "enabled": false,
        "server": "vpn.example.com",
        "username": "user",
        "password": "********",
        "status": "disconnected",
        "local_ip": "",
        "remote_ip": ""
    }
}
```

#### Scenario: 设置VPN配置
- **WHEN** POST请求 `/cgi-bin/cpe_cgi?action=vpn_set`
- **THEN** 更新VPN配置

#### Scenario: VPN连接/断开
- **WHEN** POST请求 `/cgi-bin/cpe_cgi?action=vpn_connect` 或 `vpn_disconnect`
- **THEN** 执行VPN连接或断开操作

---

### Requirement: IOT网关配置API
系统 SHALL 提供IOT网关配置接口。

#### Scenario: 获取IOT配置
- **WHEN** GET请求 `/cgi-bin/cpe_cgi?action=iot_get`
- **THEN** 返回IOT配置：
```json
{
    "code": 200,
    "message": "success",
    "data": {
        "enabled": true,
        "protocol": "MQTT",
        "server": "iot.example.com",
        "port": 1883,
        "client_id": "cpe_iot_001",
        "username": "",
        "password": "",
        "connected": true,
        "devices": [
            {
                "id": "IOT-001",
                "name": "SmartSensor-01",
                "type": "sensor",
                "status": "online"
            }
        ]
    }
}
```

#### Scenario: 设置IOT配置
- **WHEN** POST请求 `/cgi-bin/cpe_cgi?action=iot_set`
- **THEN** 更新IOT配置

---

### Requirement: 系统管理API
系统 SHALL 提供系统设置和管理接口。

#### Scenario: 获取系统配置
- **WHEN** GET请求 `/cgi-bin/cpe_cgi?action=system_get`
- **THEN** 返回系统配置：
```json
{
    "code": 200,
    "message": "success",
    "data": {
        "device_name": "5G CPE Pro",
        "timezone": "Asia/Shanghai",
        "ntp_server": "pool.ntp.org",
        "web_port": 80,
        "admin_user": "admin",
        "https_enabled": false,
        "firmware_version": "V2.1.5",
        "latest_version": "V2.1.5"
    }
}
```

#### Scenario: 设置系统配置
- **WHEN** POST请求 `/cgi-bin/cpe_cgi?action=system_set`
- **THEN** 更新系统配置

#### Scenario: 设备重启
- **WHEN** POST请求 `/cgi-bin/cpe_cgi?action=reboot`
- **THEN** 执行设备重启

#### Scenario: 恢复出厂设置
- **WHEN** POST请求 `/cgi-bin/cpe_cgi?action=factory_reset`
- **THEN** 恢复出厂设置并重启

#### Scenario: 固件升级
- **WHEN** POST请求 `/cgi-bin/cpe_cgi?action=firmware_upgrade`
- **AND** 包含固件文件
- **THEN** 执行固件升级

---

### Requirement: AT指令调试API
系统 SHALL 提供AT指令调试接口。

#### Scenario: 发送AT指令
- **WHEN** POST请求 `/cgi-bin/cpe_cgi?action=at_send`
- **AND** 请求体包含：
```json
{
    "command": "AT+CSQ"
}
```
- **THEN** 返回AT指令响应：
```json
{
    "code": 200,
    "message": "success",
    "data": {
        "command": "AT+CSQ",
        "response": "+CSQ: 25,0\n\nOK",
        "duration_ms": 50
    }
}
```

---

### Requirement: 实时数据推送（WebSocket）
系统 SHALL 支持WebSocket实时数据推送。

#### Scenario: WebSocket连接
- **WHEN** 客户端连接 `ws://device_ip:8888/ws`
- **THEN** 建立WebSocket连接
- **AND** 定期推送状态数据

#### Scenario: 状态推送
- **WHEN** WebSocket连接建立
- **THEN** 每5秒推送一次状态更新：
```json
{
    "type": "status_update",
    "timestamp": 1705303800,
    "data": {
        "signal_strength": -72,
        "rsrp": -75,
        "sinr": 15,
        "tx_rate": 45000,
        "rx_rate": 78000
    }
}
```

---

### Requirement: 配置文件管理
系统 SHALL 使用JSON格式存储配置文件。

#### Scenario: 配置文件路径
- **WHEN** 系统启动
- **THEN** 从 `/etc/cpe/config.json` 加载配置
- **AND** 配置变更时自动保存

#### Scenario: 配置文件格式
- **WHEN** 读取配置文件
- **THEN** 格式为：
```json
{
    "lan": {
        "ip": "192.168.1.1",
        "netmask": "255.255.255.0",
        "dhcp_enabled": true
    },
    "cellular": {
        "network_mode": 0,
        "apns": []
    },
    "wlan": {
        "ap": {},
        "sta": {}
    },
    "firewall": {},
    "vpn": {},
    "iot": {},
    "system": {}
}
```

---

### Requirement: 安全认证
系统 SHALL 提供基本的安全认证机制。

#### Scenario: 登录认证
- **WHEN** 用户首次访问
- **THEN** 重定向到登录页面
- **AND** 验证用户名密码
- **AND** 设置Session Cookie

#### Scenario: 会话管理
- **WHEN** 用户登录成功
- **THEN** 创建会话，有效期30分钟
- **AND** 每次请求刷新会话时间

#### Scenario: 密码修改
- **WHEN** POST请求 `/cgi-bin/cpe_cgi?action=password_change`
- **THEN** 更新管理员密码

---

## MODIFIED Requirements

### Requirement: 后端服务模块化设计
系统 SHALL 采用模块化的后端服务架构，支持多种控制方式。

#### Scenario: 控制层抽象
- **WHEN** 系统初始化
- **THEN** 根据配置选择控制模式（AT/API）
- **AND** 加载对应的控制模块

#### Scenario: 模组适配
- **WHEN** 检测到模组类型
- **THEN** 加载对应的AT指令表
- **AND** 使用正确的指令格式

---

## REMOVED Requirements

无移除的需求。
