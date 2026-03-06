# 5G CPE Web配置系统 - API接口详细文档

## 目录
1. [通用说明](#通用说明)
2. [认证模块](#认证模块)
3. [设备状态模块](#设备状态模块)
4. [本地网络模块](#本地网络模块)
5. [蜂窝网络模块](#蜂窝网络模块)
6. [APN管理模块](#apn管理模块)
7. [WLAN模块](#wlan模块)
8. [防火墙模块](#防火墙模块)
9. [VPN模块](#vpn模块)
10. [IOT网关模块](#iot网关模块)
11. [系统管理模块](#系统管理模块)
12. [AT调试模块](#at调试模块)
13. [错误码说明](#错误码说明)

---

## 通用说明

### 基础URL
```
http://<device_ip>/cgi-bin/cpe_cgi
```

### 请求方式
- GET请求：通过URL参数传递 `?action=<action_name>`
- POST请求：通过URL参数指定action，请求体传递JSON数据

### 请求头
```
Content-Type: application/json
Accept: application/json
```

### 统一响应格式
```json
{
    "code": 200,
    "message": "操作成功",
    "data": { ... } 或 [ ... ] 或 null
}
```

### 字段说明
| 字段 | 类型 | 说明 |
|------|------|------|
| code | int | 状态码，200表示成功 |
| message | string | 操作结果描述 |
| data | object/array/null | 返回数据 |

---

## 认证模块

### 模块参数定义

#### 登录请求参数
| 参数名 | 类型 | 必填 | 说明 | 约束 |
|--------|------|------|------|------|
| username | string | 是 | 用户名 | 默认admin |
| password | string | 是 | 密码 | 明文传输 |

#### 登录响应参数
| 参数名 | 类型 | 说明 |
|--------|------|------|
| session_id | string | 会话ID |
| expires | int | 过期时间戳 |
| username | string | 用户名 |

#### 修改密码请求参数
| 参数名 | 类型 | 必填 | 说明 | 约束 |
|--------|------|------|------|------|
| old_password | string | 是 | 原密码 | - |
| new_password | string | 是 | 新密码 | 长度6-32位 |
| confirm_password | string | 是 | 确认新密码 | 需与new_password一致 |

### 接口列表

#### 1. 用户登录
**接口**: `POST /cgi-bin/cpe_cgi?action=login`

**请求示例**:
```json
{
    "username": "admin",
    "password": "admin123"
}
```

**成功响应**:
```json
{
    "code": 200,
    "message": "登录成功",
    "data": {
        "session_id": "abc123def456",
        "expires": 1705305600,
        "username": "admin"
    }
}
```

**失败响应**:
```json
{
    "code": 401,
    "message": "用户名或密码错误",
    "data": null
}
```

#### 2. 用户登出
**接口**: `POST /cgi-bin/cpe_cgi?action=logout`

**请求参数**: 无

**成功响应**:
```json
{
    "code": 200,
    "message": "已退出登录",
    "data": null
}
```

#### 3. 修改密码
**接口**: `POST /cgi-bin/cpe_cgi?action=password_change`

**请求示例**:
```json
{
    "old_password": "admin123",
    "new_password": "newpass123",
    "confirm_password": "newpass123"
}
```

**成功响应**:
```json
{
    "code": 200,
    "message": "密码修改成功，请重新登录",
    "data": null
}
```

---

## 设备状态模块

### 模块参数定义

#### 设备状态响应参数
| 参数名 | 类型 | 说明 |
|--------|------|------|
| device_model | string | 设备型号 |
| firmware_version | string | 固件版本 |
| imei | string | 设备IMEI号 |
| iccid | string | SIM卡ICCID |
| imsi | string | SIM卡IMSI |
| lan_mac | string | LAN口MAC地址 |
| wan_ip | string | WAN口IPv4地址 |
| wan_ipv6 | string | WAN口IPv6地址 |
| uptime | string | 运行时间（可读格式） |
| uptime_seconds | int | 运行时间（秒） |
| signal_strength | int | 信号强度(dBm) |
| network_type | string | 网络类型：5G NR/LTE/WCDMA |
| network_mode | string | 网络模式：SA/NSA |
| operator_name | string | 运营商名称 |
| mcc | string | 移动国家代码 |
| mnc | string | 移动网络代码 |
| rsrp | int | 参考信号接收功率(dBm) |
| rsrq | int | 参考信号接收质量(dB) |
| sinr | int | 信噪比(dB) |
| band | string | 当前频段 |
| earfcn | int | 绝对频率信道号 |
| pci | int | 物理小区ID |
| cell_id | string | 小区ID |
| tx_bytes | string | 发送流量 |
| rx_bytes | string | 接收流量 |
| tx_rate | int | 发送速率(bps) |
| rx_rate | int | 接收速率(bps) |
| temperature | int | 模组温度(℃) |
| sim_status | string | SIM卡状态：ready/absent/locked |
| connected_devices | int | 已连接设备数 |

#### 流量统计响应参数
| 参数名 | 类型 | 说明 |
|--------|------|------|
| total_tx_bytes | int | 总发送字节数 |
| total_rx_bytes | int | 总接收字节数 |
| tx_rate | int | 发送速率(bps) |
| rx_rate | int | 接收速率(bps) |
| connected_devices | int | 已连接设备数 |
| session_start | int | 会话开始时间戳 |

#### 连接设备参数
| 参数名 | 类型 | 说明 |
|--------|------|------|
| ip | string | 设备IP地址 |
| mac | string | 设备MAC地址 |
| hostname | string | 主机名 |
| interface | string | 连接接口 |
| connected_time | int | 连接时间戳 |

### 接口列表

#### 1. 获取设备状态
**接口**: `GET /cgi-bin/cpe_cgi?action=status`

**请求参数**: 无

**成功响应**:
```json
{
    "code": 200,
    "message": "success",
    "data": {
        "device_model": "CPE-5G01",
        "firmware_version": "V2.1.5",
        "imei": "861234567890123",
        "iccid": "8986012345678901234",
        "imsi": "460001234567890",
        "lan_mac": "00:1A:2B:3C:4D:5E",
        "wan_ip": "10.23.45.67",
        "wan_ipv6": "240e:1234:abcd::1",
        "uptime": "3天 14时 32分",
        "uptime_seconds": 308520,
        "signal_strength": -72,
        "network_type": "5G NR",
        "network_mode": "SA",
        "operator_name": "中国移动",
        "mcc": "460",
        "mnc": "00",
        "rsrp": -72,
        "rsrq": -10,
        "sinr": 15,
        "band": "n78",
        "earfcn": 627264,
        "pci": 120,
        "cell_id": "0x12345678",
        "tx_bytes": "125.4 MB",
        "rx_bytes": "892.7 MB",
        "tx_rate": 45000,
        "rx_rate": 78000,
        "temperature": 45,
        "sim_status": "ready",
        "connected_devices": 4
    }
}
```

#### 2. 获取流量统计
**接口**: `GET /cgi-bin/cpe_cgi?action=traffic_stats`

**成功响应**:
```json
{
    "code": 200,
    "message": "success",
    "data": {
        "total_tx_bytes": 125400000,
        "total_rx_bytes": 892700000,
        "tx_rate": 45000,
        "rx_rate": 78000,
        "connected_devices": 4,
        "session_start": 1705303800
    }
}
```

#### 3. 获取连接设备列表
**接口**: `GET /cgi-bin/cpe_cgi?action=device_list`

**成功响应**:
```json
{
    "code": 200,
    "message": "success",
    "data": {
        "count": 4,
        "devices": [
            {
                "ip": "192.168.1.100",
                "mac": "AA:BB:CC:DD:EE:01",
                "hostname": "iPhone",
                "interface": "wlan0",
                "connected_time": 1705303800
            }
        ]
    }
}
```

---

## 本地网络模块

### 模块参数定义

#### LAN配置参数
| 参数名 | 类型 | 必填 | 说明 | 约束 | 默认值 |
|--------|------|------|------|------|--------|
| work_mode | string | 否 | 工作模式 | router/bridge/mobile | router |
| ip | string | 是 | LAN IP地址 | 有效IPv4地址 | 192.168.1.1 |
| netmask | string | 是 | 子网掩码 | 有效子网掩码 | 255.255.255.0 |
| dhcp_enabled | bool | 否 | DHCP开关 | true/false | true |
| dhcp_start | string | 条件必填 | DHCP起始IP | 与ip同网段 | 192.168.1.100 |
| dhcp_end | string | 条件必填 | DHCP结束IP | 与ip同网段 | 192.168.1.200 |
| dhcp_lease | int | 否 | DHCP租期 | 60-86400秒 | 86400 |
| dns1 | string | 否 | 主DNS | 有效IPv4地址 | 8.8.8.8 |
| dns2 | string | 否 | 备DNS | 有效IPv4地址 | 8.8.4.4 |

### 接口列表

#### 1. 获取LAN配置
**接口**: `GET /cgi-bin/cpe_cgi?action=lan_get`

**请求参数**: 无

**成功响应**:
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

#### 2. 设置LAN配置
**接口**: `POST /cgi-bin/cpe_cgi?action=lan_set`

**请求示例**:
```json
{
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
```

**成功响应**:
```json
{
    "code": 200,
    "message": "LAN配置已保存，部分设置需要重启后生效",
    "data": {
        "restart_required": true
    }
}
```

---

## 蜂窝网络模块

### 模块参数定义

#### 蜂窝网络配置参数
| 参数名 | 类型 | 必填 | 说明 | 约束 | 默认值 |
|--------|------|------|------|------|--------|
| network_mode | int | 否 | 网络模式 | 0-自动, 1-5G SA, 2-5G SA+NSA, 3-LTE | 0 |
| bands_5g | array | 否 | 5G频段列表 | n1/n3/n5/n7/n8/n28/n38/n41/n77/n78/n79 | [] |
| bands_lte | array | 否 | LTE频段列表 | B1/B3/B5/B7/B8/B20/B28/B38/B40/B41 | [] |
| airplane_mode | bool | 否 | 飞行模式 | true/false | false |
| data_roaming | bool | 否 | 数据漫游 | true/false | false |
| hw_accel | bool | 否 | 硬件加速 | true/false | true |
| ims_enabled | bool | 否 | IMS开关 | true/false | true |
| preferred_plmn | string | 否 | 首选PLMN | MCCMNC格式(6位数字) | "" |

#### network_mode枚举值
| 值 | 说明 |
|----|------|
| 0 | 自动（5G SA+NSA/LTE） |
| 1 | 5G SA |
| 2 | 5G SA+NSA |
| 3 | LTE |

#### 小区信息参数
| 参数名 | 类型 | 说明 |
|--------|------|------|
| cell_type | string | 小区类型 |
| mcc | string | 移动国家代码 |
| mnc | string | 移动网络代码 |
| tac | int | 跟踪区码 |
| cell_id | string | 小区ID |
| pci | int | 物理小区ID |
| band | string | 频段 |
| earfcn | int | 绝对频率信道号 |
| rsrp | int | RSRP值 |
| rsrq | int | RSRQ值 |
| sinr | int | SINR值 |
| cqi | int | 信道质量指示 |
| dl_bandwidth | int | 下行带宽(MHz) |
| ul_bandwidth | int | 上行带宽(MHz) |

### 接口列表

#### 1. 获取蜂窝网络配置
**接口**: `GET /cgi-bin/cpe_cgi?action=cellular_get`

**请求参数**: 无

**成功响应**:
```json
{
    "code": 200,
    "message": "success",
    "data": {
        "network_mode": 0,
        "network_mode_str": "自动（5G SA+NSA/LTE）",
        "bands_5g": ["n1", "n3", "n5", "n7", "n8", "n28", "n38", "n41", "n77", "n78", "n79"],
        "bands_lte": ["B1", "B3", "B5", "B7", "B8", "B20", "B28", "B38", "B40", "B41"],
        "airplane_mode": false,
        "data_roaming": false,
        "hw_accel": true,
        "ims_enabled": true,
        "preferred_plmn": ""
    }
}
```

#### 2. 设置蜂窝网络配置
**接口**: `POST /cgi-bin/cpe_cgi?action=cellular_set`

**请求示例**:
```json
{
    "network_mode": 0,
    "bands_5g": ["n78", "n41"],
    "bands_lte": ["B1", "B3", "B5", "B7", "B8"],
    "airplane_mode": false,
    "data_roaming": false,
    "hw_accel": true,
    "ims_enabled": true
}
```

**成功响应**:
```json
{
    "code": 200,
    "message": "蜂窝网络配置已保存",
    "data": null
}
```

#### 3. 获取小区信息
**接口**: `GET /cgi-bin/cpe_cgi?action=cell_info`

**成功响应**:
```json
{
    "code": 200,
    "message": "success",
    "data": {
        "cell_type": "NR5G-SA",
        "mcc": "460",
        "mnc": "00",
        "tac": 12345,
        "cell_id": "0x12345678",
        "pci": 120,
        "band": "n78",
        "earfcn": 627264,
        "rsrp": -72,
        "rsrq": -10,
        "sinr": 15,
        "cqi": 12,
        "dl_bandwidth": 100,
        "ul_bandwidth": 100
    }
}
```

---

## APN管理模块

### 模块参数定义

#### APN配置参数
| 参数名 | 类型 | 必填 | 说明 | 约束 | 默认值 |
|--------|------|------|------|------|--------|
| id | int | 条件必填 | APN唯一标识 | 系统自动生成(新增时无需) | - |
| name | string | 是 | APN名称 | 最大64字符 | - |
| username | string | 否 | 用户名 | 最大64字符 | "" |
| password | string | 否 | 密码 | 最大64字符 | "" |
| auth_type | int | 否 | 认证类型 | 0-无, 1-PAP, 2-CHAP | 0 |
| bearer_type | int | 否 | 承载类型 | 0-IP, 1-IPv4v6, 2-Ethernet | 0 |
| is_default | bool | 否 | 是否默认APN | true/false | false |
| is_active | bool | 否 | 是否已激活 | true/false | false |
| apn_types | array | 否 | APN类型 | default/mms/supl/dun | ["default"] |
| mcc | string | 否 | 移动国家代码 | 3位数字 | "" |
| mnc | string | 否 | 移动网络代码 | 2-3位数字 | "" |

#### auth_type枚举值
| 值 | 说明 |
|----|------|
| 0 | 无认证 |
| 1 | PAP认证 |
| 2 | CHAP认证 |

#### bearer_type枚举值
| 值 | 说明 |
|----|------|
| 0 | IP |
| 1 | IPv4v6 |
| 2 | Ethernet |

### 接口列表

#### 1. 获取APN列表
**接口**: `GET /cgi-bin/cpe_cgi?action=apn_list`

**请求参数**: 无

**成功响应**:
```json
{
    "code": 200,
    "message": "success",
    "data": [
        {
            "id": 1,
            "name": "cmnet",
            "username": "",
            "password": "******",
            "auth_type": 0,
            "bearer_type": 1,
            "is_default": true,
            "is_active": true,
            "apn_types": ["default", "supl"],
            "mcc": "460",
            "mnc": "00"
        }
    ]
}
```

#### 2. 添加APN
**接口**: `POST /cgi-bin/cpe_cgi?action=apn_add`

**请求示例**:
```json
{
    "name": "3gnet",
    "username": "",
    "password": "",
    "auth_type": 0,
    "bearer_type": 1,
    "is_default": false,
    "apn_types": ["default"]
}
```

**成功响应**:
```json
{
    "code": 200,
    "message": "APN添加成功",
    "data": {
        "id": 3
    }
}
```

#### 3. 编辑APN
**接口**: `POST /cgi-bin/cpe_cgi?action=apn_edit`

**请求示例**:
```json
{
    "id": 3,
    "name": "3gnet",
    "username": "user",
    "password": "pass",
    "auth_type": 1,
    "bearer_type": 1,
    "is_default": false,
    "apn_types": ["default"]
}
```

**成功响应**:
```json
{
    "code": 200,
    "message": "APN更新成功",
    "data": null
}
```

#### 4. 删除APN
**接口**: `POST /cgi-bin/cpe_cgi?action=apn_delete`

**请求参数**:
| 参数名 | 类型 | 必填 | 说明 |
|--------|------|------|------|
| id | int | 是 | APN ID |

**请求示例**:
```json
{
    "id": 3
}
```

**成功响应**:
```json
{
    "code": 200,
    "message": "APN删除成功",
    "data": null
}
```

#### 5. 激活APN
**接口**: `POST /cgi-bin/cpe_cgi?action=apn_activate`

**请求参数**:
| 参数名 | 类型 | 必填 | 说明 |
|--------|------|------|------|
| id | int | 是 | APN ID |

**请求示例**:
```json
{
    "id": 2
}
```

**成功响应**:
```json
{
    "code": 200,
    "message": "APN激活成功",
    "data": {
        "ip_address": "10.23.45.67",
        "ipv6_address": "240e:1234:abcd::1"
    }
}
```

---

## WLAN模块

### 模块参数定义

#### WLAN AP配置参数
| 参数名 | 类型 | 必填 | 说明 | 约束 | 默认值 |
|--------|------|------|------|------|--------|
| enabled_2g4 | bool | 否 | 2.4G开关 | true/false | true |
| enabled_5g | bool | 否 | 5G开关 | true/false | true |
| ssid_2g4 | string | 条件必填 | 2.4G SSID | 最大32字符 | CPE_5G_24G |
| ssid_5g | string | 条件必填 | 5G SSID | 最大32字符 | CPE_5G_5G |
| password_2g4 | string | 条件必填 | 2.4G密码 | 8-63字符 | - |
| password_5g | string | 条件必填 | 5G密码 | 8-63字符 | - |
| channel_2g4 | int | 否 | 2.4G信道 | 0=自动, 1-13 | 0 |
| channel_5g | int | 否 | 5G信道 | 0=自动, 36-165 | 0 |
| encryption | int | 否 | 加密方式 | 0-无, 3-WPA, 4-WPA2, 6-WPA3 | 4 |
| hidden_2g4 | bool | 否 | 隐藏2.4G SSID | true/false | false |
| hidden_5g | bool | 否 | 隐藏5G SSID | true/false | false |
| max_clients | int | 否 | 最大客户端数 | 1-64 | 32 |
| tx_power_2g4 | int | 否 | 2.4G发射功率 | 1-20 dBm | 20 |
| tx_power_5g | int | 否 | 5G发射功率 | 1-23 dBm | 23 |
| bandwidth_2g4 | int | 否 | 2.4G频宽 | 20/40 MHz | 20 |
| bandwidth_5g | int | 否 | 5G频宽 | 20/40/80/160 MHz | 80 |

#### encryption枚举值
| 值 | 说明 |
|----|------|
| 0 | 无加密 |
| 3 | WPA |
| 4 | WPA2 |
| 6 | WPA3 |

#### WLAN STA配置参数
| 参数名 | 类型 | 必填 | 说明 | 约束 | 默认值 |
|--------|------|------|------|------|--------|
| sta_enabled | bool | 否 | STA模式开关 | true/false | false |
| target_ssid | string | 条件必填 | 目标SSID | 最大32字符 | - |
| target_password | string | 条件必填 | 目标密码 | 8-63字符 | - |
| security_type | int | 否 | 安全类型 | 0-无, 3-WPA, 4-WPA2, 6-WPA3 | 4 |
| band | int | 否 | 频段 | 0-自动, 1-2.4G, 2-5G | 0 |
| wan_type | int | 否 | WAN类型 | 0-DHCP, 1-静态IP, 2-PPPoE | 0 |
| wan_mac | string | 否 | WAN MAC地址 | MAC格式 | 自动 |
| nat_enabled | bool | 否 | NAT开关 | true/false | true |

#### WiFi扫描结果参数
| 参数名 | 类型 | 说明 |
|--------|------|------|
| ssid | string | SSID名称 |
| bssid | string | BSSID地址 |
| signal | int | 信号强度(0-100) |
| channel | int | 信道号 |
| frequency | int | 频率(MHz) |
| security | string | 安全类型 |
| encryption | int | 加密类型 |
| band | string | 频段 |
| is_5g | bool | 是否5G |

### 接口列表

#### 1. 获取WLAN AP配置
**接口**: `GET /cgi-bin/cpe_cgi?action=wlan_ap_get`

**请求参数**: 无

**成功响应**:
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
        "encryption": 4,
        "hidden_2g4": false,
        "hidden_5g": false,
        "max_clients": 32,
        "tx_power_2g4": 20,
        "tx_power_5g": 23,
        "bandwidth_2g4": 20,
        "bandwidth_5g": 80
    }
}
```

#### 2. 设置WLAN AP配置
**接口**: `POST /cgi-bin/cpe_cgi?action=wlan_ap_set`

**请求示例**:
```json
{
    "enabled_2g4": true,
    "enabled_5g": true,
    "ssid_2g4": "CPE_5G_24G",
    "ssid_5g": "CPE_5G_5G",
    "password_2g4": "12345678",
    "password_5g": "12345678",
    "channel_2g4": 0,
    "channel_5g": 0,
    "encryption": 4,
    "hidden_2g4": false,
    "hidden_5g": false,
    "max_clients": 32,
    "bandwidth_2g4": 20,
    "bandwidth_5g": 80
}
```

**成功响应**:
```json
{
    "code": 200,
    "message": "WLAN配置已保存",
    "data": null
}
```

#### 3. 获取WLAN STA配置
**接口**: `GET /cgi-bin/cpe_cgi?action=wlan_sta_get`

**成功响应**:
```json
{
    "code": 200,
    "message": "success",
    "data": {
        "sta_enabled": false,
        "target_ssid": "",
        "target_password": "********",
        "security_type": 4,
        "band": 0,
        "wan_type": 0,
        "wan_mac": "00:1A:2B:3C:4D:5E",
        "nat_enabled": true,
        "connected": false,
        "signal": 0,
        "bssid": ""
    }
}
```

#### 4. 设置WLAN STA配置
**接口**: `POST /cgi-bin/cpe_cgi?action=wlan_sta_set`

**请求示例**:
```json
{
    "sta_enabled": true,
    "target_ssid": "MyHome_WiFi",
    "target_password": "mypassword",
    "security_type": 4,
    "band": 0,
    "wan_type": 0,
    "nat_enabled": true
}
```

**成功响应**:
```json
{
    "code": 200,
    "message": "WLAN STA配置已保存",
    "data": null
}
```

#### 5. WiFi扫描
**接口**: `GET /cgi-bin/cpe_cgi?action=wifi_scan`

**请求参数**:
| 参数名 | 类型 | 必填 | 说明 |
|--------|------|------|------|
| band | int | 否 | 扫描频段：0-全部，1-2.4G，2-5G |

**成功响应**:
```json
{
    "code": 200,
    "message": "success",
    "data": {
        "scan_time": "2024-01-15 10:30:00",
        "count": 3,
        "networks": [
            {
                "ssid": "MyWiFi_5G",
                "bssid": "AA:BB:CC:DD:EE:FF",
                "signal": 85,
                "channel": 36,
                "frequency": 5180,
                "security": "WPA2",
                "encryption": 4,
                "band": "5G",
                "is_5g": true
            }
        ]
    }
}
```

#### 6. STA连接
**接口**: `POST /cgi-bin/cpe_cgi?action=wlan_sta_connect`

**请求参数**:
| 参数名 | 类型 | 必填 | 说明 |
|--------|------|------|------|
| ssid | string | 是 | 目标SSID |
| password | string | 条件必填 | 密码 |

**请求示例**:
```json
{
    "ssid": "MyWiFi",
    "password": "mypassword"
}
```

**成功响应**:
```json
{
    "code": 200,
    "message": "正在连接...",
    "data": {
        "status": "connecting"
    }
}
```

#### 7. STA断开
**接口**: `POST /cgi-bin/cpe_cgi?action=wlan_sta_disconnect`

**请求参数**: 无

**成功响应**:
```json
{
    "code": 200,
    "message": "已断开连接",
    "data": null
}
```

---

## 防火墙模块

### 模块参数定义

#### 防火墙配置参数
| 参数名 | 类型 | 必填 | 说明 | 约束 | 默认值 |
|--------|------|------|------|------|--------|
| enabled | bool | 否 | 防火墙开关 | true/false | true |
| dmz_enabled | bool | 否 | DMZ开关 | true/false | false |
| dmz_ip | string | 条件必填 | DMZ主机IP | 有效IPv4地址 | "" |
| spi_enabled | bool | 否 | SPI防火墙 | true/false | true |
| dos_enabled | bool | 否 | DoS防护 | true/false | true |
| arp_proxy | bool | 否 | ARP代理 | true/false | false |
| ping_wan | bool | 否 | 允许WAN口Ping | true/false | false |

#### 端口转发规则参数
| 参数名 | 类型 | 必填 | 说明 | 约束 | 默认值 |
|--------|------|------|------|------|--------|
| id | int | 条件必填 | 规则ID | 系统自动生成(新增时无需) | - |
| protocol | string | 是 | 协议 | TCP/UDP/TCP_UDP | - |
| external_port | int | 是 | 外部端口 | 1-65535 | - |
| internal_ip | string | 是 | 内部IP地址 | 有效IPv4地址 | - |
| internal_port | int | 是 | 内部端口 | 1-65535 | - |
| description | string | 否 | 描述 | 最大64字符 | "" |
| enabled | bool | 否 | 是否启用 | true/false | true |

### 接口列表

#### 1. 获取防火墙配置
**接口**: `GET /cgi-bin/cpe_cgi?action=firewall_get`

**请求参数**: 无

**成功响应**:
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
        "ping_wan": false,
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

#### 2. 设置防火墙配置
**接口**: `POST /cgi-bin/cpe_cgi?action=firewall_set`

**请求示例**:
```json
{
    "enabled": true,
    "dmz_enabled": false,
    "dmz_ip": "",
    "spi_enabled": true,
    "dos_enabled": true,
    "arp_proxy": false,
    "ping_wan": false
}
```

**成功响应**:
```json
{
    "code": 200,
    "message": "防火墙配置已保存",
    "data": null
}
```

#### 3. 添加端口转发
**接口**: `POST /cgi-bin/cpe_cgi?action=port_forward_add`

**请求示例**:
```json
{
    "protocol": "TCP",
    "external_port": 8080,
    "internal_ip": "192.168.1.100",
    "internal_port": 80,
    "description": "Web Server",
    "enabled": true
}
```

**成功响应**:
```json
{
    "code": 200,
    "message": "端口转发规则添加成功",
    "data": {
        "id": 2
    }
}
```

#### 4. 编辑端口转发
**接口**: `POST /cgi-bin/cpe_cgi?action=port_forward_edit`

**请求示例**:
```json
{
    "id": 2,
    "protocol": "TCP",
    "external_port": 8080,
    "internal_ip": "192.168.1.101",
    "internal_port": 8080,
    "description": "Web Server Updated",
    "enabled": true
}
```

**成功响应**:
```json
{
    "code": 200,
    "message": "端口转发规则更新成功",
    "data": null
}
```

#### 5. 删除端口转发
**接口**: `POST /cgi-bin/cpe_cgi?action=port_forward_delete`

**请求参数**:
| 参数名 | 类型 | 必填 | 说明 |
|--------|------|------|------|
| id | int | 是 | 规则ID |

**请求示例**:
```json
{
    "id": 1
}
```

**成功响应**:
```json
{
    "code": 200,
    "message": "端口转发规则删除成功",
    "data": null
}
```

---

## VPN模块

### 模块参数定义

#### VPN类型枚举
| 值 | 说明 |
|----|------|
| 0 | PPTP |
| 1 | L2TP |
| 2 | GRE |
| 3 | EOIP |
| 4 | IPSec |

#### PPTP/L2TP配置参数
| 参数名 | 类型 | 必填 | 说明 | 约束 | 默认值 |
|--------|------|------|------|------|--------|
| type | int | 是 | VPN类型 | 0-PPTP, 1-L2TP | - |
| enabled | bool | 否 | 开关 | true/false | false |
| server | string | 是 | 服务器地址 | 域名或IP | - |
| username | string | 是 | 用户名 | 最大64字符 | - |
| password | string | 是 | 密码 | 最大64字符 | - |
| mppe | bool | 否 | MPPE加密 | true/false | true |

#### GRE隧道配置参数
| 参数名 | 类型 | 必填 | 说明 | 约束 | 默认值 |
|--------|------|------|------|------|--------|
| type | int | 是 | VPN类型 | 固定为2 | - |
| enabled | bool | 否 | 开关 | true/false | false |
| local_ip | string | 是 | 本地IP | 有效IPv4地址 | - |
| remote_ip | string | 是 | 远程IP | 有效IPv4地址 | - |
| tunnel_key | string | 否 | 隧道密钥 | 最大64字符 | "" |
| mtu | int | 否 | MTU | 576-1500 | 1476 |
| keepalive | bool | 否 | 保活 | true/false | true |
| keepalive_interval | int | 否 | 保活间隔 | 1-3600秒 | 10 |

#### EOIP配置参数
| 参数名 | 类型 | 必填 | 说明 | 约束 | 默认值 |
|--------|------|------|------|------|--------|
| type | int | 是 | VPN类型 | 固定为3 | - |
| enabled | bool | 否 | 开关 | true/false | false |
| tunnel_id | int | 是 | 隧道ID | 1-65535 | 1 |
| remote_ip | string | 是 | 远程IP | 有效IPv4地址 | - |
| local_mac | string | 否 | 本地MAC | MAC格式 | 自动 |
| tunnel_key | string | 否 | 隧道密钥 | 最大64字符 | "" |
| mtu | int | 否 | MTU | 576-1500 | 1500 |
| arp_proxy | bool | 否 | ARP代理 | true/false | false |

#### IPSec配置参数
| 参数名 | 类型 | 必填 | 说明 | 约束 | 默认值 |
|--------|------|------|------|------|--------|
| type | int | 是 | VPN类型 | 固定为4 | - |
| enabled | bool | 否 | 开关 | true/false | false |
| remote_gateway | string | 是 | 远程网关 | 域名或IP | - |
| preshared_key | string | 是 | 预共享密钥 | 最大64字符 | - |
| local_id | string | 否 | 本地ID | 最大64字符 | "" |
| remote_id | string | 否 | 远程ID | 最大64字符 | "" |
| ike_encryption | string | 否 | IKE加密算法 | AES-128/AES-256/3DES | AES-256 |
| ike_authentication | string | 否 | IKE认证算法 | SHA-1/SHA-256/SHA-384 | SHA-256 |
| ike_dh_group | int | 否 | IKE DH组 | 1/2/5/14/15/16 | 14 |
| ike_lifetime | int | 否 | IKE生命周期 | 600-86400秒 | 3600 |
| esp_encryption | string | 否 | ESP加密算法 | AES-128/AES-256/3DES | AES-256 |
| esp_authentication | string | 否 | ESP认证算法 | SHA-1/SHA-256/SHA-384 | SHA-256 |
| esp_lifetime | int | 否 | ESP生命周期 | 600-86400秒 | 3600 |

### 接口列表

#### 1. 获取VPN配置
**接口**: `GET /cgi-bin/cpe_cgi?action=vpn_get&type=<vpn_type>`

**URL参数**:
| 参数名 | 类型 | 必填 | 说明 |
|--------|------|------|------|
| type | string | 是 | VPN类型：pptp/l2tp/gre/eoip/ipsec |

**PPTP/L2TP响应示例**:
```json
{
    "code": 200,
    "message": "success",
    "data": {
        "type": 0,
        "enabled": false,
        "server": "vpn.example.com",
        "username": "user",
        "password": "********",
        "mppe": true,
        "status": "disconnected",
        "local_ip": "",
        "remote_ip": "",
        "connect_time": 0
    }
}
```

**IPSec响应示例**:
```json
{
    "code": 200,
    "message": "success",
    "data": {
        "type": 4,
        "enabled": false,
        "remote_gateway": "203.0.113.1",
        "preshared_key": "********",
        "local_id": "",
        "remote_id": "",
        "ike_encryption": "AES-256",
        "ike_authentication": "SHA-256",
        "ike_dh_group": 14,
        "ike_lifetime": 3600,
        "esp_encryption": "AES-256",
        "esp_authentication": "SHA-256",
        "esp_lifetime": 3600,
        "status": "down"
    }
}
```

#### 2. 设置VPN配置
**接口**: `POST /cgi-bin/cpe_cgi?action=vpn_set`

**PPTP请求示例**:
```json
{
    "type": 0,
    "enabled": true,
    "server": "vpn.example.com",
    "username": "user",
    "password": "mypassword",
    "mppe": true
}
```

**IPSec请求示例**:
```json
{
    "type": 4,
    "enabled": true,
    "remote_gateway": "203.0.113.1",
    "preshared_key": "secret123",
    "local_id": "cpe.local",
    "remote_id": "vpn.remote",
    "ike_encryption": "AES-256",
    "ike_authentication": "SHA-256",
    "ike_dh_group": 14,
    "ike_lifetime": 3600,
    "esp_encryption": "AES-256",
    "esp_authentication": "SHA-256",
    "esp_lifetime": 3600
}
```

**成功响应**:
```json
{
    "code": 200,
    "message": "VPN配置已保存",
    "data": null
}
```

#### 3. VPN连接
**接口**: `POST /cgi-bin/cpe_cgi?action=vpn_connect`

**请求参数**:
| 参数名 | 类型 | 必填 | 说明 |
|--------|------|------|------|
| type | int | 是 | VPN类型 |

**请求示例**:
```json
{
    "type": 0
}
```

**成功响应**:
```json
{
    "code": 200,
    "message": "正在连接VPN...",
    "data": {
        "status": "connecting"
    }
}
```

#### 4. VPN断开
**接口**: `POST /cgi-bin/cpe_cgi?action=vpn_disconnect`

**请求参数**:
| 参数名 | 类型 | 必填 | 说明 |
|--------|------|------|------|
| type | int | 是 | VPN类型 |

**请求示例**:
```json
{
    "type": 0
}
```

**成功响应**:
```json
{
    "code": 200,
    "message": "VPN已断开",
    "data": null
}
```

---

## IOT网关模块

### 模块参数定义

#### IOT配置参数
| 参数名 | 类型 | 必填 | 说明 | 约束 | 默认值 |
|--------|------|------|------|------|--------|
| enabled | bool | 否 | IOT服务开关 | true/false | false |
| protocol | int | 否 | 协议类型 | 0-MQTT, 1-CoAP, 2-HTTP | 0 |
| server | string | 是 | 服务器地址 | 域名或IP | - |
| port | int | 是 | 端口号 | 1-65535 | 1883 |
| client_id | string | 否 | 客户端ID | 最大64字符 | 自动生成 |
| username | string | 否 | 用户名 | 最大64字符 | "" |
| password | string | 否 | 密码 | 最大64字符 | "" |
| keepalive | int | 否 | 心跳间隔 | 10-3600秒 | 60 |
| qos | int | 否 | QoS级别 | 0/1/2 | 1 |
| clean_session | bool | 否 | 清除会话 | true/false | true |
| publish_topic | string | 否 | 发布主题 | 最大128字符 | "" |
| subscribe_topic | string | 否 | 订阅主题 | 最大128字符 | "" |

#### protocol枚举值
| 值 | 说明 |
|----|------|
| 0 | MQTT |
| 1 | CoAP |
| 2 | HTTP |

#### IOT设备参数
| 参数名 | 类型 | 说明 |
|--------|------|------|
| id | string | 设备ID |
| name | string | 设备名称 |
| type | string | 设备类型 |
| status | string | 设备状态 |
| last_seen | int | 最后在线时间戳 |

### 接口列表

#### 1. 获取IOT配置
**接口**: `GET /cgi-bin/cpe_cgi?action=iot_get`

**请求参数**: 无

**成功响应**:
```json
{
    "code": 200,
    "message": "success",
    "data": {
        "enabled": true,
        "protocol": 0,
        "server": "iot.example.com",
        "port": 1883,
        "client_id": "cpe_iot_001",
        "username": "",
        "password": "********",
        "keepalive": 60,
        "qos": 1,
        "clean_session": true,
        "connected": true,
        "publish_topic": "cpe/data",
        "subscribe_topic": "cpe/cmd",
        "devices": [
            {
                "id": "IOT-001",
                "name": "SmartSensor-01",
                "type": "sensor",
                "status": "online",
                "last_seen": 1705303800
            }
        ]
    }
}
```

#### 2. 设置IOT配置
**接口**: `POST /cgi-bin/cpe_cgi?action=iot_set`

**请求示例**:
```json
{
    "enabled": true,
    "protocol": 0,
    "server": "iot.example.com",
    "port": 1883,
    "client_id": "cpe_iot_001",
    "username": "user",
    "password": "pass",
    "keepalive": 60,
    "qos": 1,
    "clean_session": true,
    "publish_topic": "cpe/data",
    "subscribe_topic": "cpe/cmd"
}
```

**成功响应**:
```json
{
    "code": 200,
    "message": "IOT配置已保存",
    "data": null
}
```

---

## 系统管理模块

### 模块参数定义

#### 系统配置参数
| 参数名 | 类型 | 必填 | 说明 | 约束 | 默认值 |
|--------|------|------|------|------|--------|
| device_name | string | 否 | 设备名称 | 最大64字符 | "5G CPE" |
| timezone | string | 否 | 时区 | 时区标识 | "Asia/Shanghai" |
| ntp_server | string | 否 | NTP服务器 | 域名或IP | "pool.ntp.org" |
| web_port | int | 否 | Web管理端口 | 1-65535 | 80 |
| admin_user | string | 否 | 管理员用户名 | 最大32字符 | "admin" |
| https_enabled | bool | 否 | HTTPS开关 | true/false | false |
| remote_manage | bool | 否 | 远程管理开关 | true/false | false |
| language | string | 否 | 语言 | zh_CN/en_US | "zh_CN" |

#### 固件更新参数
| 参数名 | 类型 | 说明 |
|--------|------|------|
| current_version | string | 当前版本 |
| latest_version | string | 最新版本 |
| update_available | bool | 是否有更新 |
| release_notes | string | 更新说明 |
| file_size | int | 文件大小(字节) |
| release_date | string | 发布日期 |

### 接口列表

#### 1. 获取系统配置
**接口**: `GET /cgi-bin/cpe_cgi?action=system_get`

**请求参数**: 无

**成功响应**:
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
        "remote_manage": false,
        "firmware_version": "V2.1.5",
        "latest_version": "V2.1.5",
        "update_available": false,
        "language": "zh_CN"
    }
}
```

#### 2. 设置系统配置
**接口**: `POST /cgi-bin/cpe_cgi?action=system_set`

**请求示例**:
```json
{
    "device_name": "My 5G CPE",
    "timezone": "Asia/Shanghai",
    "ntp_server": "pool.ntp.org",
    "language": "zh_CN"
}
```

**成功响应**:
```json
{
    "code": 200,
    "message": "系统配置已保存",
    "data": null
}
```

#### 3. 设备重启
**接口**: `POST /cgi-bin/cpe_cgi?action=reboot`

**请求参数**: 无

**成功响应**:
```json
{
    "code": 200,
    "message": "设备正在重启...",
    "data": null
}
```

#### 4. 恢复出厂设置
**接口**: `POST /cgi-bin/cpe_cgi?action=factory_reset`

**请求参数**: 无

**成功响应**:
```json
{
    "code": 200,
    "message": "正在恢复出厂设置，设备将重启...",
    "data": null
}
```

#### 5. 检查固件更新
**接口**: `GET /cgi-bin/cpe_cgi?action=firmware_check`

**请求参数**: 无

**成功响应**:
```json
{
    "code": 200,
    "message": "success",
    "data": {
        "current_version": "V2.1.5",
        "latest_version": "V2.2.0",
        "update_available": true,
        "release_notes": "1. 优化5G连接稳定性\n2. 修复已知问题\n3. 新增VPN功能",
        "file_size": 15728640,
        "release_date": "2024-01-10"
    }
}
```

#### 6. 固件升级
**接口**: `POST /cgi-bin/cpe_cgi?action=firmware_upgrade`

**请求**: multipart/form-data格式，包含固件文件

**请求参数**:
| 参数名 | 类型 | 必填 | 说明 |
|--------|------|------|------|
| firmware | file | 是 | 固件文件(.bin) |

**成功响应**:
```json
{
    "code": 200,
    "message": "固件上传成功，正在升级...",
    "data": {
        "progress": 0
    }
}
```

#### 7. 获取升级进度
**接口**: `GET /cgi-bin/cpe_cgi?action=upgrade_progress`

**请求参数**: 无

**成功响应**:
```json
{
    "code": 200,
    "message": "success",
    "data": {
        "status": "upgrading",
        "progress": 45,
        "message": "正在写入固件..."
    }
}
```

#### 8. 导出配置
**接口**: `GET /cgi-bin/cpe_cgi?action=config_export`

**响应**: 返回配置文件下载

#### 9. 导入配置
**接口**: `POST /cgi-bin/cpe_cgi?action=config_import`

**请求**: multipart/form-data格式，包含配置文件

---

## AT调试模块

### 模块参数定义

#### AT指令请求参数
| 参数名 | 类型 | 必填 | 说明 | 约束 | 默认值 |
|--------|------|------|------|------|--------|
| command | string | 是 | AT指令 | 不含\r\n | - |
| timeout | int | 否 | 超时时间 | 100-30000 ms | 3000 |

#### AT指令响应参数
| 参数名 | 类型 | 说明 |
|--------|------|------|
| command | string | 发送的指令 |
| response | string | 响应内容 |
| duration_ms | int | 执行耗时(毫秒) |
| success | bool | 是否成功 |

### 接口列表

#### 1. 发送AT指令
**接口**: `POST /cgi-bin/cpe_cgi?action=at_send`

**请求示例**:
```json
{
    "command": "AT+CSQ",
    "timeout": 3000
}
```

**成功响应**:
```json
{
    "code": 200,
    "message": "success",
    "data": {
        "command": "AT+CSQ",
        "response": "+CSQ: 25,0\n\nOK",
        "duration_ms": 50,
        "success": true
    }
}
```

**失败响应**:
```json
{
    "code": 200,
    "message": "success",
    "data": {
        "command": "AT+INVALID",
        "response": "ERROR",
        "duration_ms": 100,
        "success": false
    }
}
```

#### 2. 获取常用AT指令列表
**接口**: `GET /cgi-bin/cpe_cgi?action=at_commands`

**请求参数**: 无

**成功响应**:
```json
{
    "code": 200,
    "message": "success",
    "data": {
        "commands": [
            {
                "name": "查询信号强度",
                "command": "AT+CSQ",
                "description": "返回信号强度和误码率"
            },
            {
                "name": "查询网络注册",
                "command": "AT+CREG?",
                "description": "返回网络注册状态"
            },
            {
                "name": "查询运营商",
                "command": "AT+COPS?",
                "description": "返回当前运营商信息"
            }
        ]
    }
}
```

---

## 错误码说明

### HTTP状态码
| 状态码 | 说明 |
|--------|------|
| 200 | 成功 |
| 400 | 请求参数错误 |
| 401 | 未授权/登录过期 |
| 403 | 禁止访问 |
| 404 | 资源不存在 |
| 500 | 服务器内部错误 |
| 503 | 服务暂时不可用 |

### 业务错误码（code字段）
| 错误码 | 说明 |
|--------|------|
| 200 | 成功 |
| 400 | 参数错误 |
| 401 | 认证失败 |
| 403 | 权限不足 |
| 404 | 资源不存在 |
| 409 | 资源冲突 |
| 500 | 服务器内部错误 |
| 501 | 功能未实现 |
| 503 | 服务不可用 |
| 1001 | SIM卡未插入 |
| 1002 | SIM卡已锁定 |
| 1003 | 网络未注册 |
| 1004 | APN配置错误 |
| 1005 | PDP激活失败 |
| 2001 | WiFi启动失败 |
| 2002 | WiFi连接失败 |
| 2003 | WiFi密码错误 |
| 3001 | VPN连接失败 |
| 3002 | VPN认证失败 |
| 4001 | 固件校验失败 |
| 4002 | 固件版本不兼容 |

---

## 参数类型汇总

### 布尔类型参数
所有布尔类型参数统一使用 `true` 或 `false`，不使用 0/1。

### 整数类型参数
所有整数类型参数直接使用数字，不加引号。

### 字符串类型参数
所有字符串类型参数使用双引号包裹。

### 数组类型参数
数组类型参数使用JSON数组格式，如 `["n78", "n41"]`。

### 时间戳参数
时间戳统一使用Unix时间戳（秒级），整数类型。

---

## 接口调用示例

### JavaScript (fetch)
```javascript
// GET请求
fetch('/cgi-bin/cpe_cgi?action=status')
    .then(response => response.json())
    .then(data => console.log(data));

// POST请求
fetch('/cgi-bin/cpe_cgi?action=lan_set', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({
        ip: '192.168.1.1',
        netmask: '255.255.255.0',
        dhcp_enabled: true
    })
})
.then(response => response.json())
.then(data => console.log(data));
```

### curl
```bash
# GET请求
curl "http://192.168.1.1/cgi-bin/cpe_cgi?action=status"

# POST请求
curl -X POST "http://192.168.1.1/cgi-bin/cpe_cgi?action=lan_set" \
    -H "Content-Type: application/json" \
    -d '{"ip":"192.168.1.1","netmask":"255.255.255.0","dhcp_enabled":true}'
```
