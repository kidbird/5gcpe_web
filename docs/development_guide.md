# 5G CPE Web 开发环境搭建指南

## 一、环境概述

### 开发架构
```
┌─────────────────┐      SSH/SCP       ┌─────────────────┐
│   开发服务器     │ ◄───────────────► │   嵌入式设备     │
│  (交叉编译环境)  │      网络部署       │   (目标平台)     │
└─────────────────┘                     └─────────────────┘
        │                                       │
        │ 编译输出                               │ HTTP访问
        ▼                                       ▼
┌─────────────────┐                     ┌─────────────────┐
│   开发主机       │ ◄───────────────► │   浏览器调试     │
│   (PC/Mac)      │      网络访问       │                 │
└─────────────────┘                     └─────────────────┘
```

### 目录结构
```
/Volumes/kidbird/5GCPE_web/
├── backend/                 # 后端CGI代码
│   ├── include/            # 头文件
│   ├── src/                # 源代码
│   ├── build/              # 编译输出
│   └── Makefile            # 构建配置
├── css/                    # 样式文件
├── js/                     # JavaScript代码
├── config/                 # 配置文件
│   └── lighttpd.conf       # Web服务器配置
├── scripts/                # 构建和部署脚本
│   ├── build.sh            # 构建脚本
│   ├── deploy.sh           # 部署脚本
│   └── debug.sh            # 调试脚本
├── deploy/                 # 部署输出目录
│   └── rootfs/             # 目标文件系统
└── index.html              # 主页面
```

## 二、开发服务器配置

### 1. 安装交叉编译工具链

#### Ubuntu/Debian
```bash
# ARM 64位 (aarch64)
sudo apt-get update
sudo apt-get install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu

# ARM 32位 (armhf)
sudo apt-get install gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf

# 调试工具
sudo apt-get install gdb-multiarch

# 部署工具
sudo apt-get install sshpass openssh-client
```

#### 使用厂商工具链
```bash
# 解压厂商提供的工具链
tar -xzf toolchain-aarch64.tar.gz -C /opt/

# 设置环境变量
export PATH=/opt/toolchain/aarch64-linux-gnu/bin:$PATH
export CROSS_COMPILE=aarch64-linux-gnu-
```

### 2. 验证编译器
```bash
# 检查编译器版本
aarch64-linux-gnu-gcc --version

# 测试编译
cd backend
make CROSS_COMPILE=aarch64-linux-gnu- clean all

# 检查生成的二进制文件
file build/cpe_cgi
# 输出: build/cpe_cgi: ELF 64-bit LSB executable, ARM aarch64, version 1 (SYSV)...
```

## 三、构建系统使用

### 1. 本地构建 (x86_64)
```bash
# 进入项目目录
cd /Volumes/kidbird/5GCPE_web

# 赋予脚本执行权限
chmod +x scripts/*.sh

# 本地构建
./scripts/build.sh

# 调试版本
BUILD_TYPE=debug ./scripts/build.sh
```

### 2. 交叉编译
```bash
# ARM 64位
TARGET_ARCH=aarch64 ./scripts/build.sh

# 指定交叉编译器
CROSS_COMPILE=aarch64-linux-gnu- ./scripts/build.sh

# 调试版本
TARGET_ARCH=aarch64 BUILD_TYPE=debug ./scripts/build.sh
```

### 3. 创建发布包
```bash
# 构建并打包
TARGET_ARCH=aarch64 ./scripts/build.sh package
```

## 四、设备部署

### 1. 网络配置

#### 方式一: 直连设备
```
开发主机 (PC) ──── 网线 ──── 嵌入式设备
   │                              │
   └─ IP: 192.168.1.100           └─ IP: 192.168.1.1
```

```bash
# 配置开发主机IP (macOS)
sudo ifconfig en0 192.168.1.100 netmask 255.255.255.0

# 测试连接
ping 192.168.1.1
```

#### 方式二: 通过交换机/路由器
```
开发主机 ──── 交换机 ──── 嵌入式设备
   │              │              │
   └─ DHCP        └─ Router      └─ DHCP
```

### 2. SSH配置

#### 密钥认证 (推荐)
```bash
# 生成SSH密钥
ssh-keygen -t rsa -b 4096

# 复制公钥到设备
ssh-copy-id root@192.168.1.1

# 测试连接
ssh root@192.168.1.1 "echo OK"
```

#### 密码认证
```bash
# 使用sshpass
export DEVICE_PASS="your_password"
sshpass -p "$DEVICE_PASS" ssh root@192.168.1.1
```

### 3. 部署到设备
```bash
# 基本部署
DEVICE_IP=192.168.1.1 ./scripts/deploy.sh

# 使用密码
DEVICE_IP=192.168.1.1 DEVICE_PASS=admin123 ./scripts/deploy.sh

# 指定部署目录
DEVICE_IP=192.168.1.1 DEPLOY_DIR=/opt/www ./scripts/deploy.sh
```

## 五、调试方法

### 1. 快速调试流程
```bash
# 修改代码后快速部署
./scripts/debug.sh quick

# 测试API
./scripts/debug.sh test status
./scripts/debug.sh test lan_get

# 查看日志
./scripts/debug.sh logs
```

### 2. GDB远程调试

#### 设备端启动gdbserver
```bash
# SSH到设备
ssh root@192.168.1.1

# 停止web服务
killall lighttpd

# 启动gdbserver
gdbserver :1234 /www/cgi-bin/cpe_cgi
```

#### 开发主机连接GDB
```bash
# 使用调试脚本
./scripts/debug.sh gdb

# 或手动连接
gdb-multiarch
(gdb) target remote 192.168.1.1:1234
(gdb) file backend/build/cpe_cgi
(gdb) break main
(gdb) continue
```

### 3. 使用strace跟踪
```bash
# 跟踪CGI执行
./scripts/debug.sh strace status

# 查看系统调用
cat /tmp/cgi_strace.log
```

### 4. 日志调试

#### 设备端查看日志
```bash
ssh root@192.168.1.1
tail -f /var/log/lighttpd/error.log
tail -f /var/log/messages
```

#### CGI代码添加日志
```c
#include <syslog.h>

void log_debug(const char *fmt, ...) {
    openlog("cpe_cgi", LOG_PID | LOG_NDELAY, LOG_LOCAL0);
    va_list args;
    va_start(args, fmt);
    vsyslog(LOG_DEBUG, fmt, args);
    va_end(args);
    closelog();
}
```

## 六、开发流程

### 1. 修改代码
```bash
# 编辑源文件
vim backend/src/cgi_handler.c
```

### 2. 本地测试 (可选)
```bash
# 编译本地版本
./scripts/build.sh

# 本地运行测试
cd backend/build
./cpe_cgi "action=status"
```

### 3. 交叉编译
```bash
# 编译目标平台版本
TARGET_ARCH=aarch64 BUILD_TYPE=debug ./scripts/build.sh
```

### 4. 部署测试
```bash
# 快速部署
DEVICE_IP=192.168.1.1 ./scripts/debug.sh quick
```

### 5. 浏览器测试
```
打开浏览器访问: http://192.168.1.1/
```

### 6. API测试
```bash
# 使用curl测试
curl -u admin:admin123 "http://192.168.1.1/cgi-bin/cpe_cgi?action=status"

# 使用调试脚本
./scripts/debug.sh test status
```

## 七、常见问题

### 1. 编译错误
```bash
# 检查编译器
aarch64-linux-gnu-gcc --version

# 检查头文件路径
make CROSS_COMPILE=aarch64-linux-gnu- print-CFLAGS

# 清理重新编译
make clean && make CROSS_COMPILE=aarch64-linux-gnu-
```

### 2. 部署失败
```bash
# 检查网络连接
ping 192.168.1.1

# 检查SSH连接
ssh -v root@192.168.1.1

# 检查设备磁盘空间
ssh root@192.168.1.1 "df -h"
```

### 3. CGI执行错误
```bash
# 检查文件权限
ssh root@192.168.1.1 "ls -la /www/cgi-bin/"

# 检查执行权限
ssh root@192.168.1.1 "chmod +x /www/cgi-bin/cpe_cgi"

# 手动执行测试
ssh root@192.168.1.1 "/www/cgi-bin/cpe_cgi 'action=status'"
```

### 4. Web服务问题
```bash
# 检查lighttpd状态
ssh root@192.168.1.1 "ps | grep lighttpd"

# 重启服务
ssh root@192.168.1.1 "killall lighttpd; lighttpd -f /etc/lighttpd/lighttpd.conf"

# 检查端口
ssh root@192.168.1.1 "netstat -tlnp | grep 80"
```

## 八、性能优化

### 1. 编译优化
```bash
# 发布版本 (优化体积)
TARGET_ARCH=aarch64 ./scripts/build.sh

# 调试版本 (包含调试信息)
TARGET_ARCH=aarch64 BUILD_TYPE=debug ./scripts/build.sh
```

### 2. 剥离符号表
```bash
# 减小二进制文件大小
aarch64-linux-gnu-strip backend/build/cpe_cgi
```

### 3. 静态链接
```makefile
# Makefile中添加
LDFLAGS = -static -lpthread
```

## 九、自动化CI/CD

### 示例 GitLab CI
```yaml
# .gitlab-ci.yml
stages:
  - build
  - deploy

build_aarch64:
  stage: build
  script:
    - TARGET_ARCH=aarch64 ./scripts/build.sh package
  artifacts:
    paths:
      - deploy/*.tar.gz

deploy_to_device:
  stage: deploy
  script:
    - DEVICE_IP=$DEVICE_IP DEVICE_PASS=$DEVICE_PASS ./scripts/deploy.sh
  when: manual
  needs:
    - build_aarch64
```

## 十、附录

### A. 环境变量说明
| 变量 | 说明 | 默认值 |
|------|------|--------|
| TARGET_ARCH | 目标架构 | aarch64 |
| CROSS_COMPILE | 交叉编译器前缀 | 自动检测 |
| BUILD_TYPE | 构建类型 | release |
| DESTDIR | 部署目录 | deploy/rootfs |
| DEVICE_IP | 设备IP地址 | 192.168.1.1 |
| DEVICE_USER | SSH用户名 | root |
| DEVICE_PASS | SSH密码 | 空 |
| DEPLOY_DIR | 设备部署目录 | /www |

### B. 常用命令速查
```bash
# 构建
./scripts/build.sh                          # 本地构建
TARGET_ARCH=aarch64 ./scripts/build.sh      # 交叉编译
BUILD_TYPE=debug ./scripts/build.sh         # 调试版本

# 部署
DEVICE_IP=192.168.1.1 ./scripts/deploy.sh   # 部署到设备
./scripts/deploy.sh restart                 # 重启服务

# 调试
./scripts/debug.sh quick                    # 快速部署
./scripts/debug.sh test status              # 测试API
./scripts/debug.sh logs                     # 查看日志
./scripts/debug.sh gdb                      # GDB调试
```
