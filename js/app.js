const CPE_API = {
    baseUrl: '/cgi-bin/cpe_cgi',
    
    request: function(action, method, data) {
        return new Promise((resolve, reject) => {
            const url = `${this.baseUrl}?action=${action}`;
            const options = {
                method: method || 'GET',
                headers: {
                    'Content-Type': 'application/json'
                }
            };
            
            if (data && method === 'POST') {
                options.body = JSON.stringify(data);
            }
            
            fetch(url, options)
                .then(response => response.json())
                .then(result => {
                    if (result.code === 200) {
                        resolve(result.data);
                    } else {
                        reject(new Error(result.message));
                    }
                })
                .catch(error => {
                    reject(error);
                });
        });
    },
    
    get: function(action) {
        return this.request(action, 'GET');
    },
    
    post: function(action, data) {
        return this.request(action, 'POST', data);
    },
    
    getStatus: function() {
        return this.get('status');
    },
    
    getTrafficStats: function() {
        return this.get('traffic_stats');
    },
    
    getDeviceList: function() {
        return this.get('device_list');
    },
    
    getLanConfig: function() {
        return this.get('lan_get');
    },
    
    setLanConfig: function(config) {
        return this.post('lan_set', config);
    },
    
    getCellularConfig: function() {
        return this.get('cellular_get');
    },
    
    setCellularConfig: function(config) {
        return this.post('cellular_set', config);
    },
    
    getApnList: function() {
        return this.get('apn_list');
    },
    
    addApn: function(apn) {
        return this.post('apn_add', apn);
    },
    
    editApn: function(apn) {
        return this.post('apn_edit', apn);
    },
    
    deleteApn: function(id) {
        return this.post('apn_delete', { id: id });
    },
    
    activateApn: function(id) {
        return this.post('apn_activate', { id: id });
    },
    
    getWlanApConfig: function() {
        return this.get('wlan_ap_get');
    },
    
    setWlanApConfig: function(config) {
        return this.post('wlan_ap_set', config);
    },
    
    getWlanStaConfig: function() {
        return this.get('wlan_sta_get');
    },
    
    setWlanStaConfig: function(config) {
        return this.post('wlan_sta_set', config);
    },
    
    scanWifi: function() {
        return this.get('wifi_scan');
    },
    
    getFirewallConfig: function() {
        return this.get('firewall_get');
    },
    
    setFirewallConfig: function(config) {
        return this.post('firewall_set', config);
    },
    
    addPortForward: function(rule) {
        return this.post('port_forward_add', rule);
    },
    
    deletePortForward: function(id) {
        return this.post('port_forward_delete', { id: id });
    },
    
    getVpnConfig: function(type) {
        return this.get(`vpn_get&type=${type}`);
    },
    
    setVpnConfig: function(config) {
        return this.post('vpn_set', config);
    },
    
    vpnConnect: function(type) {
        return this.post('vpn_connect', { type: type });
    },
    
    vpnDisconnect: function(type) {
        return this.post('vpn_disconnect', { type: type });
    },
    
    getIotConfig: function() {
        return this.get('iot_get');
    },
    
    setIotConfig: function(config) {
        return this.post('iot_set', config);
    },
    
    getSystemConfig: function() {
        return this.get('system_get');
    },
    
    setSystemConfig: function(config) {
        return this.post('system_set', config);
    },
    
    reboot: function() {
        return this.post('reboot', {});
    },
    
    factoryReset: function() {
        return this.post('factory_reset', {});
    },
    
    sendAtCommand: function(command, timeout) {
        return this.post('at_send', { command: command, timeout: timeout || 3000 });
    },
    
    login: function(username, password) {
        return this.post('login', { username: username, password: password });
    },
    
    changePassword: function(oldPassword, newPassword, confirmPassword) {
        return this.post('password_change', {
            old_password: oldPassword,
            new_password: newPassword,
            confirm_password: confirmPassword
        });
    },
    
    setLinkType: function(linkType) {
        return this.post('link_type_set', { link_type: linkType });
    },
    
    getVersionInfo: function() {
        return this.get('version_get');
    },
    
    checkUpgrade: function() {
        return this.get('upgrade_check');
    },
    
    startUpgrade: function() {
        return this.post('upgrade_start', {});
    },
    
    getUpgradeProgress: function() {
        return this.get('upgrade_progress');
    },
    
    uploadFirmware: function(formData) {
        return new Promise((resolve, reject) => {
            fetch(`${this.baseUrl}?action=firmware_upload`, {
                method: 'POST',
                body: formData
            })
            .then(response => response.json())
            .then(result => {
                if (result.code === 200) {
                    resolve(result.data);
                } else {
                    reject(new Error(result.message));
                }
            })
            .catch(error => reject(error));
        });
    }
};

document.addEventListener('DOMContentLoaded', function() {
    const menuItems = document.querySelectorAll('.menu-item');
    const modulePages = document.querySelectorAll('.module-page');
    const pageTitle = document.getElementById('page-title');
    const refreshBtn = document.getElementById('refresh-btn');
    const saveBtn = document.getElementById('save-btn');

    const pageTitles = {
        'status': '设备状态',
        'lan': '本地网络',
        'cellular': '蜂窝网络',
        'wlan': 'WLAN',
        'firewall': '防火墙',
        'vpn': 'VPN',
        'iot': 'IOT',
        'system': '系统'
    };

    function loadPageData(module) {
        switch (module) {
            case 'status':
                loadDeviceStatus();
                break;
            case 'lan':
                loadLanConfig();
                break;
            case 'cellular':
                loadCellularConfig();
                loadApnList();
                break;
            case 'wlan':
                loadWlanApConfig();
                loadWlanStaConfig();
                break;
            case 'firewall':
                loadFirewallConfig();
                break;
            case 'vpn':
                loadVpnConfig('pptp');
                break;
            case 'iot':
                loadIotConfig();
                break;
            case 'system':
                loadSystemConfig();
                break;
        }
    }

    menuItems.forEach(item => {
        item.addEventListener('click', function() {
            const module = this.getAttribute('data-module');
            
            menuItems.forEach(menuItem => {
                menuItem.classList.remove('active');
            });
            this.classList.add('active');
            
            modulePages.forEach(page => {
                page.classList.remove('active');
            });
            
            const targetPage = document.getElementById('page-' + module);
            if (targetPage) {
                targetPage.classList.add('active');
            }
            
            if (pageTitle && pageTitles[module]) {
                pageTitle.textContent = pageTitles[module];
            }
            
            loadPageData(module);
        });
    });

    refreshBtn.addEventListener('click', function() {
        this.classList.add('refreshing');
        const activeModule = document.querySelector('.menu-item.active');
        if (activeModule) {
            loadPageData(activeModule.getAttribute('data-module'));
        }
        setTimeout(() => {
            this.classList.remove('refreshing');
            showNotification('数据已刷新', 'success');
        }, 500);
    });

    saveBtn.addEventListener('click', function() {
        const activeModule = document.querySelector('.menu-item.active');
        if (activeModule) {
            savePageData(activeModule.getAttribute('data-module'));
        }
    });

    function loadDeviceStatus() {
        CPE_API.getStatus().then(data => {
            updateElement('device-model', data.device_model || '-');
            updateElement('firmware-version', data.firmware_version || '-');
            updateElement('imei', data.imei || '-');
            updateElement('iccid', data.iccid || '-');
            updateElement('lan-mac', data.lan_mac || '-');
            updateElement('wan-ip', data.wan_ip || '-');
            updateElement('uptime', data.uptime || '-');
            updateElement('signal-strength', data.signal_strength + ' dBm' || '-');
            updateElement('network-type', data.network_type || '-');
            updateElement('operator-name', data.operator_name || '-');
            updateElement('rsrp', data.rsrp + ' dBm' || '-');
            updateElement('sinr', data.sinr + ' dB' || '-');
            updateElement('band', data.band || '-');
            updateElement('tx-bytes', data.tx_bytes || '-');
            updateElement('rx-bytes', data.rx_bytes || '-');
            updateElement('connected-devices', data.connected_devices || '0');
            updateElement('active-sim', (data.active_sim === 2 ? 'SIM卡2' : 'SIM卡1') + ' - 已插入');
            
            updateLinkStatus(data.link_type || 'cellular');
            
            const linkTypeSelect = document.getElementById('link-type-select');
            if (linkTypeSelect && data.link_type) {
                linkTypeSelect.value = data.link_type;
            }
            
            const signalBars = document.querySelector('.signal-bars');
            if (signalBars && data.signal_strength) {
                const strength = Math.min(4, Math.max(0, Math.floor((data.signal_strength + 120) / 15)));
                signalBars.className = 'signal-bars strength-' + strength;
            }
        }).catch(error => {
            updateElement('device-model', '5G CPE Pro');
            updateElement('firmware-version', 'V1.0.5');
            updateElement('imei', '861234567890001');
            updateElement('iccid', '89860121801234567890');
            updateElement('lan-mac', '00:1A:2B:3C:4D:5E');
            updateElement('wan-ip', '10.0.0.1');
            updateElement('uptime', '2天 5小时 30分钟');
            updateElement('signal-strength', '-75 dBm');
            updateElement('network-type', '5G SA');
            updateElement('operator-name', '中国移动');
            updateElement('rsrp', '-85 dBm');
            updateElement('sinr', '15 dB');
            updateElement('band', 'n78');
            updateElement('tx-bytes', '1.2 GB');
            updateElement('rx-bytes', '3.5 GB');
            updateElement('connected-devices', '5');
            updateElement('active-sim', 'SIM卡1 - 已插入');
            
            updateLinkStatus('cellular');
            
            const linkTypeSelect = document.getElementById('link-type-select');
            if (linkTypeSelect) {
                linkTypeSelect.value = 'cellular';
            }
            
            const signalBars = document.querySelector('.signal-bars');
            if (signalBars) {
                signalBars.className = 'signal-bars strength-3';
            }
        });
    }
    
    function updateLinkStatus(activeType) {
        const statusTexts = {
            'cellular': '蜂窝网络',
            'wired': '有线网络',
            'bridge': '无线桥接'
        };
        
        const deviceLinkTypeEl = document.getElementById('device-link-type');
        if (deviceLinkTypeEl && statusTexts[activeType]) {
            deviceLinkTypeEl.textContent = statusTexts[activeType];
        }
    }

    function loadLanConfig() {
        CPE_API.getLanConfig().then(data => {
            document.getElementById('lan-ip').value = data.ip || '';
            document.getElementById('lan-netmask').value = data.netmask || '';
            document.getElementById('dhcp-enabled').checked = data.dhcp_enabled;
            document.getElementById('dhcp-start').value = data.dhcp_start || '';
            document.getElementById('dhcp-end').value = data.dhcp_end || '';
            document.getElementById('dhcp-lease').value = data.dhcp_lease || 86400;
            document.getElementById('dns1').value = data.dns1 || '';
            document.getElementById('dns2').value = data.dns2 || '';
        }).catch(error => {
            document.getElementById('lan-ip').value = '192.168.1.1';
            document.getElementById('lan-netmask').value = '255.255.255.0';
            document.getElementById('dhcp-enabled').checked = true;
            document.getElementById('dhcp-start').value = '192.168.1.100';
            document.getElementById('dhcp-end').value = '192.168.1.200';
            document.getElementById('dhcp-lease').value = 86400;
            document.getElementById('dns1').value = '8.8.8.8';
            document.getElementById('dns2').value = '8.8.4.4';
        });
    }

    function loadCellularConfig() {
        CPE_API.getCellularConfig().then(data => {
            document.getElementById('network-mode').value = data.network_mode || 0;
            document.getElementById('airplane-mode').checked = data.airplane_mode;
            document.getElementById('data-roaming').checked = data.data_roaming;
            document.getElementById('hw-accel').checked = data.hw_accel;
            document.getElementById('ims-enabled').checked = data.ims_enabled;
        }).catch(error => {
            document.getElementById('network-mode').value = 0;
            document.getElementById('airplane-mode').checked = false;
            document.getElementById('data-roaming').checked = false;
            document.getElementById('hw-accel').checked = true;
            document.getElementById('ims-enabled').checked = true;
        });
    }

    function loadApnList() {
        CPE_API.getApnList().then(data => {
            const apnList = document.getElementById('apn-list');
            if (!apnList) return;
            
            apnList.innerHTML = '';
            
            if (Array.isArray(data)) {
                data.forEach(apn => {
                    const apnHtml = `
                        <div class="apn-item" data-apn="${apn.id}">
                            <div class="apn-header">
                                <span class="apn-name">${apn.name}${apn.is_active ? ' (已激活)' : ''}</span>
                                <span class="apn-status ${apn.is_active ? 'enabled' : 'disabled'}">${apn.is_active ? '已激活' : '未激活'}</span>
                            </div>
                            <div class="apn-details">
                                <div class="apn-detail-row">
                                    <span class="apn-label">APN名称</span>
                                    <span class="apn-value">${apn.name}</span>
                                </div>
                                <div class="apn-detail-row">
                                    <span class="apn-label">用户名</span>
                                    <span class="apn-value">${apn.username || '-'}</span>
                                </div>
                                <div class="apn-detail-row">
                                    <span class="apn-label">密码</span>
                                    <span class="apn-value">******</span>
                                </div>
                                <div class="apn-detail-row">
                                    <span class="apn-label">认证类型</span>
                                    <span class="apn-value">${apn.auth_type === 0 ? '无' : (apn.auth_type === 1 ? 'PAP' : 'CHAP')}</span>
                                </div>
                                <div class="apn-detail-row">
                                    <span class="apn-label">承载类型</span>
                                    <span class="apn-value">${apn.bearer_type === 0 ? 'IP' : (apn.bearer_type === 1 ? 'IPv4v6' : 'Ethernet')}</span>
                                </div>
                            </div>
                            <div class="apn-actions">
                                <button class="btn btn-primary btn-sm edit-apn-btn">编辑</button>
                                <button class="btn btn-secondary btn-sm delete-apn-btn">删除</button>
                            </div>
                        </div>
                    `;
                    apnList.insertAdjacentHTML('beforeend', apnHtml);
                });
            }
        }).catch(error => {
            const apnList = document.getElementById('apn-list');
            if (!apnList) return;
            apnList.innerHTML = `
                <div class="apn-item" data-apn="1">
                    <div class="apn-header">
                        <span class="apn-name">CMNET (已激活)</span>
                        <span class="apn-status enabled">已激活</span>
                    </div>
                    <div class="apn-details">
                        <div class="apn-detail-row">
                            <span class="apn-label">APN名称</span>
                            <span class="apn-value">cmnet</span>
                        </div>
                        <div class="apn-detail-row">
                            <span class="apn-label">用户名</span>
                            <span class="apn-value">-</span>
                        </div>
                        <div class="apn-detail-row">
                            <span class="apn-label">密码</span>
                            <span class="apn-value">******</span>
                        </div>
                        <div class="apn-detail-row">
                            <span class="apn-label">认证类型</span>
                            <span class="apn-value">无</span>
                        </div>
                        <div class="apn-detail-row">
                            <span class="apn-label">承载类型</span>
                            <span class="apn-value">IPv4v6</span>
                        </div>
                    </div>
                    <div class="apn-actions">
                        <button class="btn btn-primary btn-sm edit-apn-btn">编辑</button>
                        <button class="btn btn-secondary btn-sm delete-apn-btn">删除</button>
                    </div>
                </div>
            `;
        });
    }

    function loadWlanApConfig() {
        CPE_API.getWlanApConfig().then(data => {
            document.getElementById('wlan-2g4-enabled').checked = data.enabled_2g4;
            document.getElementById('wlan-5g-enabled').checked = data.enabled_5g;
            document.getElementById('ssid-2g4').value = data.ssid_2g4 || '';
            document.getElementById('ssid-5g').value = data.ssid_5g || '';
            document.getElementById('password-2g4').value = '';
            document.getElementById('password-5g').value = '';
            document.getElementById('channel-2g4').value = data.channel_2g4 || 0;
            document.getElementById('channel-5g').value = data.channel_5g || 0;
            document.getElementById('encryption').value = data.encryption || 4;
        }).catch(error => {
            document.getElementById('wlan-2g4-enabled').checked = true;
            document.getElementById('wlan-5g-enabled').checked = true;
            document.getElementById('ssid-2g4').value = '5G-CPE-2G4';
            document.getElementById('ssid-5g').value = '5G-CPE-5G';
            document.getElementById('password-2g4').value = '';
            document.getElementById('password-5g').value = '';
            document.getElementById('channel-2g4').value = 0;
            document.getElementById('channel-5g').value = 0;
            document.getElementById('encryption').value = 4;
        });
    }

    function loadWlanStaConfig() {
        CPE_API.getWlanStaConfig().then(data => {
            document.getElementById('sta-enabled').checked = data.sta_enabled;
            document.getElementById('target-ssid').value = data.target_ssid || '';
            document.getElementById('target-password').value = '';
            document.getElementById('wan-type').value = data.wan_type || 0;
            document.getElementById('nat-enabled').checked = data.nat_enabled;
        }).catch(error => {
            document.getElementById('sta-enabled').checked = false;
            document.getElementById('target-ssid').value = '';
            document.getElementById('target-password').value = '';
            document.getElementById('wan-type').value = 0;
            document.getElementById('nat-enabled').checked = true;
        });
    }

    function loadFirewallConfig() {
        CPE_API.getFirewallConfig().then(data => {
            document.getElementById('firewall-enabled').checked = data.enabled;
            document.getElementById('dmz-enabled').checked = data.dmz_enabled;
            document.getElementById('dmz-ip').value = data.dmz_ip || '';
            document.getElementById('spi-enabled').checked = data.spi_enabled;
            document.getElementById('dos-enabled').checked = data.dos_enabled;
            
            const portList = document.getElementById('port-forward-list');
            if (portList && data.port_forwards) {
                portList.innerHTML = '';
                data.port_forwards.forEach(rule => {
                    const portHtml = `
                        <div class="port-forward-item" data-port="${rule.id}">
                            <span class="port-info">${rule.protocol}:${rule.external_port} → ${rule.internal_ip}:${rule.internal_port}</span>
                            <div class="port-actions">
                                <button class="btn btn-primary btn-sm edit-port-btn">编辑</button>
                                <button class="btn btn-secondary btn-sm delete-port-btn">删除</button>
                            </div>
                        </div>
                    `;
                    portList.insertAdjacentHTML('beforeend', portHtml);
                });
            }
        }).catch(error => {
            document.getElementById('dmz-enabled').checked = false;
            document.getElementById('dmz-ip').value = '';
        });
    }

    function loadVpnConfig(type) {
        CPE_API.getVpnConfig(type).then(data => {
            document.getElementById('vpn-enabled').checked = data.enabled;
            document.getElementById('vpn-server').value = data.server || '';
            document.getElementById('vpn-username').value = data.username || '';
            document.getElementById('vpn-password').value = '';
        }).catch(error => {
            document.getElementById('vpn-enabled').checked = false;
            document.getElementById('vpn-server').value = '';
            document.getElementById('vpn-username').value = '';
            document.getElementById('vpn-password').value = '';
        });
    }

    function loadIotConfig() {
        CPE_API.getIotConfig().then(data => {
            document.getElementById('iot-enabled').checked = data.enabled;
            document.getElementById('iot-protocol').value = data.protocol || 0;
            document.getElementById('iot-server').value = data.server || '';
            document.getElementById('iot-port').value = data.port || 1883;
            document.getElementById('iot-client-id').value = data.client_id || '';
            document.getElementById('iot-username').value = data.username || '';
            document.getElementById('iot-password').value = '';
            document.getElementById('iot-keepalive').value = data.keepalive || 60;
            document.getElementById('iot-qos').value = data.qos || 1;
        }).catch(error => {
            document.getElementById('iot-enabled').checked = false;
            document.getElementById('iot-protocol').value = 0;
            document.getElementById('iot-server').value = '';
            document.getElementById('iot-port').value = 1883;
            document.getElementById('iot-client-id').value = '';
            document.getElementById('iot-username').value = '';
            document.getElementById('iot-password').value = '';
            document.getElementById('iot-keepalive').value = 60;
            document.getElementById('iot-qos').value = 1;
        });
    }

    function loadSystemConfig() {
        CPE_API.getSystemConfig().then(data => {
            document.getElementById('system-device-name').value = data.device_name || '';
            document.getElementById('system-timezone').value = data.timezone || 'Asia/Shanghai';
            document.getElementById('system-ntp-server').value = data.ntp_server || '';
            document.getElementById('system-web-port').value = data.web_port || 80;
            document.getElementById('system-ssh-port').value = data.ssh_port || 22;
        }).catch(error => {
            document.getElementById('system-device-name').value = '5G CPE';
            document.getElementById('system-timezone').value = 'Asia/Shanghai';
            document.getElementById('system-ntp-server').value = 'pool.ntp.org';
            document.getElementById('system-web-port').value = 80;
            document.getElementById('system-ssh-port').value = 22;
        });
        
        loadVersionInfo();
        checkUpgradeAvailable();
    }
    
    function loadVersionInfo() {
        CPE_API.getVersionInfo().then(data => {
            document.getElementById('version-model').textContent = data.device_model || '-';
            document.getElementById('version-hardware').textContent = data.hardware_version || '-';
            document.getElementById('version-firmware').textContent = data.firmware_version || '-';
            document.getElementById('version-module').textContent = data.module_model || '-';
            document.getElementById('version-module-fw').textContent = data.module_firmware || '-';
            document.getElementById('version-mac').textContent = data.mac_address || '-';
            document.getElementById('version-serial').textContent = data.serial_number || '-';
            document.getElementById('version-imei').textContent = data.imei || '-';
            document.getElementById('version-kernel').textContent = data.kernel_version || '-';
            document.getElementById('version-webserver').textContent = data.webserver_version || '-';
            document.getElementById('version-cgi').textContent = data.cgi_version || '-';
            document.getElementById('version-build-time').textContent = data.build_time || '-';
            
            document.getElementById('upgrade-current-version').textContent = data.firmware_version || '-';
        }).catch(error => {
            document.getElementById('version-model').textContent = '5G CPE Pro';
            document.getElementById('version-hardware').textContent = 'V2.0';
            document.getElementById('version-firmware').textContent = '1.0.5';
            document.getElementById('version-module').textContent = 'RM500U';
            document.getElementById('version-module-fw').textContent = 'SWI09A01';
            document.getElementById('version-mac').textContent = '00:1A:2B:3C:4D:5E';
            document.getElementById('version-serial').textContent = 'SN202403010001';
            document.getElementById('version-imei').textContent = '861234567890001';
            document.getElementById('version-kernel').textContent = '4.19.125';
            document.getElementById('version-webserver').textContent = '1.0.0';
            document.getElementById('version-cgi').textContent = '1.0.0';
            document.getElementById('version-build-time').textContent = '2024-03-15 10:30:00';
            document.getElementById('upgrade-current-version').textContent = '1.0.5';
        });
    }
    
    function checkUpgradeAvailable() {
        CPE_API.checkUpgrade().then(data => {
            if (data.has_upgrade) {
                document.getElementById('upgrade-latest-version').textContent = data.latest_version || '-';
                document.getElementById('upgrade-new-badge').style.display = 'inline-flex';
                document.getElementById('upgrade-notice').style.display = 'flex';
                document.getElementById('upgrade-notice-text').textContent = 
                    `发现新版本 ${data.latest_version}，建议立即升级`;
                
                document.getElementById('package-version').textContent = data.latest_version || '-';
                document.getElementById('package-size').textContent = formatFileSize(data.package_size);
                document.getElementById('package-date').textContent = data.release_date || '-';
                document.getElementById('package-changelog').textContent = data.changelog || '无更新说明';
                document.getElementById('upgrade-package-section').style.display = 'block';
            } else {
                document.getElementById('upgrade-latest-version').textContent = 
                    document.getElementById('upgrade-current-version').textContent;
                document.getElementById('upgrade-new-badge').style.display = 'none';
                document.getElementById('upgrade-notice').style.display = 'none';
                document.getElementById('upgrade-package-section').style.display = 'none';
            }
        }).catch(error => {
            document.getElementById('upgrade-latest-version').textContent = 
                document.getElementById('upgrade-current-version').textContent;
            document.getElementById('upgrade-new-badge').style.display = 'none';
            document.getElementById('upgrade-notice').style.display = 'none';
            document.getElementById('upgrade-package-section').style.display = 'none';
        });
    }
    
    function formatFileSize(bytes) {
        if (!bytes) return '-';
        const units = ['B', 'KB', 'MB', 'GB'];
        let i = 0;
        while (bytes >= 1024 && i < units.length - 1) {
            bytes /= 1024;
            i++;
        }
        return bytes.toFixed(2) + ' ' + units[i];
    }

    function savePageData(module) {
        switch (module) {
            case 'lan':
                saveLanConfig();
                break;
            case 'cellular':
                saveCellularConfig();
                break;
            case 'wlan':
                saveWlanConfig();
                break;
            case 'firewall':
                saveFirewallConfig();
                break;
            case 'vpn':
                saveVpnConfig();
                break;
            case 'iot':
                saveIotConfig();
                break;
            case 'system':
                const activeTab = document.querySelector('#page-system .vpn-tab.active');
                if (activeTab && activeTab.getAttribute('data-tab') === 'password') {
                    changeAdminPassword();
                } else {
                    saveSystemConfig();
                }
                break;
        }
    }

    function saveLanConfig() {
        const config = {
            ip: document.getElementById('lan-ip').value,
            netmask: document.getElementById('lan-netmask').value,
            dhcp_enabled: document.getElementById('dhcp-enabled').checked,
            dhcp_start: document.getElementById('dhcp-start').value,
            dhcp_end: document.getElementById('dhcp-end').value,
            dhcp_lease: parseInt(document.getElementById('dhcp-lease').value) || 86400,
            dns1: document.getElementById('dns1').value,
            dns2: document.getElementById('dns2').value
        };
        
        CPE_API.setLanConfig(config).then(() => {
            showNotification('LAN配置已保存', 'success');
        }).catch(error => {
            showNotification('保存失败: ' + error.message, 'error');
        });
    }

    function saveCellularConfig() {
        const config = {
            network_mode: parseInt(document.getElementById('network-mode').value) || 0,
            airplane_mode: document.getElementById('airplane-mode').checked,
            data_roaming: document.getElementById('data-roaming').checked,
            hw_accel: document.getElementById('hw-accel').checked,
            ims_enabled: document.getElementById('ims-enabled').checked
        };
        
        CPE_API.setCellularConfig(config).then(() => {
            showNotification('蜂窝网络配置已保存', 'success');
        }).catch(error => {
            showNotification('保存失败: ' + error.message, 'error');
        });
    }

    function saveWlanConfig() {
        const config = {
            enabled_2g4: document.getElementById('wlan-2g4-enabled').checked,
            enabled_5g: document.getElementById('wlan-5g-enabled').checked,
            ssid_2g4: document.getElementById('ssid-2g4').value,
            ssid_5g: document.getElementById('ssid-5g').value,
            password_2g4: document.getElementById('password-2g4').value,
            password_5g: document.getElementById('password-5g').value,
            channel_2g4: parseInt(document.getElementById('channel-2g4').value) || 0,
            channel_5g: parseInt(document.getElementById('channel-5g').value) || 0,
            encryption: parseInt(document.getElementById('encryption').value) || 4
        };
        
        CPE_API.setWlanApConfig(config).then(() => {
            showNotification('WLAN配置已保存', 'success');
        }).catch(error => {
            showNotification('保存失败: ' + error.message, 'error');
        });
    }

    function saveFirewallConfig() {
        const config = {
            enabled: document.getElementById('firewall-enabled').checked,
            dmz_enabled: document.getElementById('dmz-enabled').checked,
            dmz_ip: document.getElementById('dmz-ip').value,
            spi_enabled: document.getElementById('spi-enabled').checked,
            dos_enabled: document.getElementById('dos-enabled').checked
        };
        
        CPE_API.setFirewallConfig(config).then(() => {
            showNotification('防火墙配置已保存', 'success');
        }).catch(error => {
            showNotification('保存失败: ' + error.message, 'error');
        });
    }

    function saveVpnConfig() {
        const activeTab = document.querySelector('#page-vpn .vpn-tab.active');
        const vpnType = activeTab ? activeTab.getAttribute('data-vpn') : 'pptp';
        const typeMap = { 'pptp': 0, 'l2tp': 1, 'gre': 2, 'eoip': 3, 'ipsec': 4 };
        
        const config = {
            type: typeMap[vpnType] || 0,
            enabled: document.getElementById('vpn-enabled').checked,
            server: document.getElementById('vpn-server').value,
            username: document.getElementById('vpn-username').value,
            password: document.getElementById('vpn-password').value
        };
        
        CPE_API.setVpnConfig(config).then(() => {
            showNotification('VPN配置已保存', 'success');
        }).catch(error => {
            showNotification('保存失败: ' + error.message, 'error');
        });
    }

    function saveIotConfig() {
        const config = {
            enabled: document.getElementById('iot-enabled').checked,
            protocol: parseInt(document.getElementById('iot-protocol').value) || 0,
            server: document.getElementById('iot-server').value,
            port: parseInt(document.getElementById('iot-port').value) || 1883,
            client_id: document.getElementById('iot-client-id').value,
            username: document.getElementById('iot-username').value,
            password: document.getElementById('iot-password').value,
            keepalive: parseInt(document.getElementById('iot-keepalive').value) || 60,
            qos: parseInt(document.getElementById('iot-qos').value) || 1
        };
        
        CPE_API.setIotConfig(config).then(() => {
            showNotification('IOT配置已保存', 'success');
        }).catch(error => {
            showNotification('保存失败: ' + error.message, 'error');
        });
    }

    function saveSystemConfig() {
        const config = {
            device_name: document.getElementById('system-device-name').value,
            timezone: document.getElementById('system-timezone').value,
            ntp_server: document.getElementById('system-ntp-server').value,
            web_port: parseInt(document.getElementById('system-web-port').value) || 80,
            ssh_port: parseInt(document.getElementById('system-ssh-port').value) || 22
        };
        
        CPE_API.setSystemConfig(config).then(() => {
            showNotification('系统配置已保存', 'success');
        }).catch(error => {
            showNotification('保存失败: ' + error.message, 'error');
        });
    }
    
    function changeAdminPassword() {
        const oldPassword = document.getElementById('system-old-password').value;
        const newPassword = document.getElementById('system-new-password').value;
        const confirmPassword = document.getElementById('system-new-password-confirm').value;
        
        if (!oldPassword) {
            showNotification('请输入当前密码', 'error');
            return;
        }
        
        if (!newPassword) {
            showNotification('请输入新密码', 'error');
            return;
        }
        
        if (newPassword !== confirmPassword) {
            showNotification('两次输入的新密码不一致', 'error');
            return;
        }
        
        CPE_API.changePassword(oldPassword, newPassword, confirmPassword).then(() => {
            showNotification('密码修改成功', 'success');
            document.getElementById('system-old-password').value = '';
            document.getElementById('system-new-password').value = '';
            document.getElementById('system-new-password-confirm').value = '';
        }).catch(error => {
            showNotification('密码修改失败: ' + error.message, 'error');
        });
    }

    function updateElement(id, value) {
        const el = document.getElementById(id);
        if (el) {
            el.textContent = value;
        }
    }

    const togglePasswordBtns = document.querySelectorAll('.toggle-password');
    togglePasswordBtns.forEach(btn => {
        btn.addEventListener('click', function() {
            const input = this.previousElementSibling;
            if (input.type === 'password') {
                input.type = 'text';
            } else {
                input.type = 'password';
            }
        });
    });

    function showNotification(message, type) {
        const existingNotification = document.querySelector('.notification');
        if (existingNotification) {
            existingNotification.remove();
        }

        const notification = document.createElement('div');
        notification.className = 'notification notification-' + type;
        notification.textContent = message;
        
        const style = document.createElement('style');
        style.textContent = `
            .notification {
                position: fixed;
                top: 90px;
                right: 32px;
                padding: 16px 24px;
                border-radius: 10px;
                font-size: 14px;
                font-weight: 500;
                z-index: 1000;
                animation: slideIn 0.3s ease;
                box-shadow: 0 4px 20px rgba(0, 0, 0, 0.3);
            }
            .notification-success {
                background: #1b5e20;
                color: #4caf50;
                border: 1px solid #4caf50;
            }
            .notification-error {
                background: #b71c1c;
                color: #f44336;
                border: 1px solid #f44336;
            }
            @keyframes slideIn {
                from {
                    opacity: 0;
                    transform: translateX(20px);
                }
                to {
                    opacity: 1;
                    transform: translateX(0);
                }
            }
        `;
        document.head.appendChild(style);
        document.body.appendChild(notification);

        setTimeout(() => {
            notification.style.animation = 'slideIn 0.3s ease reverse';
            setTimeout(() => {
                notification.remove();
                style.remove();
            }, 300);
        }, 2000);
    }

    const vpnConnectBtn = document.querySelector('#page-vpn .btn-primary');
    if (vpnConnectBtn) {
        vpnConnectBtn.addEventListener('click', function() {
            const vpnStatus = document.querySelector('.vpn-status-indicator');
            const activeTab = document.querySelector('#page-vpn .vpn-tab.active');
            const vpnType = activeTab ? activeTab.getAttribute('data-vpn') : 'pptp';
            const typeMap = { 'pptp': 0, 'l2tp': 1, 'gre': 2, 'eoip': 3, 'ipsec': 4 };
            
            if (vpnStatus.classList.contains('disconnected')) {
                vpnStatus.textContent = '连接中...';
                vpnStatus.classList.remove('disconnected');
                vpnStatus.classList.add('connecting');
                
                CPE_API.vpnConnect(typeMap[vpnType]).then(() => {
                    vpnStatus.textContent = '已连接';
                    vpnStatus.classList.remove('connecting');
                    vpnStatus.classList.add('connected');
                    showNotification('VPN连接成功', 'success');
                }).catch(error => {
                    vpnStatus.textContent = '未连接';
                    vpnStatus.classList.remove('connecting');
                    vpnStatus.classList.add('disconnected');
                    showNotification('VPN连接失败: ' + error.message, 'error');
                });
            } else {
                CPE_API.vpnDisconnect(typeMap[vpnType]).then(() => {
                    vpnStatus.textContent = '未连接';
                    vpnStatus.classList.remove('connected');
                    vpnStatus.classList.add('disconnected');
                    showNotification('VPN已断开', 'success');
                }).catch(error => {
                    showNotification('VPN断开失败: ' + error.message, 'error');
                });
            }
        });
    }

    const restartBtn = document.getElementById('btn-reboot');
    if (restartBtn) {
        restartBtn.addEventListener('click', function() {
            if (confirm('确定要重启设备吗？')) {
                CPE_API.reboot().then(() => {
                    showNotification('设备正在重启...', 'success');
                }).catch(error => {
                    showNotification('重启失败: ' + error.message, 'error');
                });
            }
        });
    }

    const resetBtn = document.getElementById('btn-factory-reset');
    if (resetBtn) {
        resetBtn.addEventListener('click', function() {
            if (confirm('确定要恢复出厂设置吗？所有配置将被清除！')) {
                CPE_API.factoryReset().then(() => {
                    showNotification('正在恢复出厂设置...', 'success');
                }).catch(error => {
                    showNotification('恢复出厂设置失败: ' + error.message, 'error');
                });
            }
        });
    }
    
    const systemTabs = document.querySelectorAll('#system-tabs .vpn-tab');
    const systemContents = document.querySelectorAll('#page-system .vpn-content');
    
    systemTabs.forEach(tab => {
        tab.addEventListener('click', function() {
            const tabType = this.getAttribute('data-tab');
            
            systemTabs.forEach(t => t.classList.remove('active'));
            this.classList.add('active');
            
            systemContents.forEach(content => {
                content.classList.remove('active');
            });
            
            const targetContent = document.getElementById('system-tab-' + tabType);
            if (targetContent) {
                targetContent.classList.add('active');
            }
        });
    });
    
    const checkUpgradeBtn = document.getElementById('btn-check-upgrade');
    if (checkUpgradeBtn) {
        checkUpgradeBtn.addEventListener('click', function() {
            this.disabled = true;
            this.innerHTML = '<svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="10"></circle><polyline points="12 6 12 12 16 14"></polyline></svg> 检查中...';
            
            CPE_API.checkUpgrade().then(data => {
                if (data.has_upgrade) {
                    document.getElementById('upgrade-latest-version').textContent = data.latest_version || '-';
                    document.getElementById('upgrade-new-badge').style.display = 'inline-flex';
                    document.getElementById('upgrade-notice').style.display = 'flex';
                    document.getElementById('upgrade-notice-text').textContent = 
                        `发现新版本 ${data.latest_version}，建议立即升级`;
                    
                    document.getElementById('package-version').textContent = data.latest_version || '-';
                    document.getElementById('package-size').textContent = formatFileSize(data.package_size);
                    document.getElementById('package-date').textContent = data.release_date || '-';
                    document.getElementById('package-changelog').textContent = data.changelog || '无更新说明';
                    document.getElementById('upgrade-package-section').style.display = 'block';
                    
                    showNotification('发现新版本 ' + data.latest_version, 'success');
                } else {
                    document.getElementById('upgrade-latest-version').textContent = 
                        document.getElementById('upgrade-current-version').textContent;
                    document.getElementById('upgrade-new-badge').style.display = 'none';
                    document.getElementById('upgrade-notice').style.display = 'none';
                    document.getElementById('upgrade-package-section').style.display = 'none';
                    showNotification('当前已是最新版本', 'success');
                }
            }).catch(error => {
                showNotification('检查更新失败: ' + error.message, 'error');
            }).finally(() => {
                this.disabled = false;
                this.innerHTML = '<svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="10"></circle><polyline points="12 6 12 12 16 14"></polyline></svg> 检查更新';
            });
        });
    }
    
    const startUpgradeBtn = document.getElementById('btn-start-upgrade');
    if (startUpgradeBtn) {
        startUpgradeBtn.addEventListener('click', function() {
            if (!confirm('确定要升级固件吗？升级过程中请勿断电！')) {
                return;
            }
            
            this.disabled = true;
            document.getElementById('upgrade-progress-section').style.display = 'block';
            
            CPE_API.startUpgrade().then(() => {
                showNotification('开始升级...', 'success');
                pollUpgradeProgress();
            }).catch(error => {
                showNotification('升级失败: ' + error.message, 'error');
                this.disabled = false;
            });
        });
    }
    
    let upgradeProgressInterval = null;
    
    function pollUpgradeProgress() {
        if (upgradeProgressInterval) {
            clearInterval(upgradeProgressInterval);
        }
        
        upgradeProgressInterval = setInterval(() => {
            CPE_API.getUpgradeProgress().then(data => {
                updateUpgradeProgress(data);
                
                if (data.status === 'completed') {
                    clearInterval(upgradeProgressInterval);
                    showNotification('升级完成，设备正在重启...', 'success');
                } else if (data.status === 'failed') {
                    clearInterval(upgradeProgressInterval);
                    showNotification('升级失败: ' + data.error, 'error');
                    document.getElementById('btn-start-upgrade').disabled = false;
                }
            }).catch(error => {
                console.log('获取升级进度失败:', error);
            });
        }, 1000);
    }
    
    function updateUpgradeProgress(data) {
        const steps = ['download', 'verify', 'flash', 'reboot'];
        const currentStep = data.current_step || 0;
        
        steps.forEach((step, index) => {
            const stepEl = document.getElementById('step-' + step);
            if (stepEl) {
                if (index < currentStep) {
                    stepEl.classList.remove('active');
                    stepEl.classList.add('completed');
                } else if (index === currentStep) {
                    stepEl.classList.add('active');
                    stepEl.classList.remove('completed');
                } else {
                    stepEl.classList.remove('active', 'completed');
                }
            }
            
            if (step === 'download' && currentStep === 0) {
                const percent = data.download_percent || 0;
                document.getElementById('progress-download').style.width = percent + '%';
                document.getElementById('percent-download').textContent = percent + '%';
            } else if (step === 'verify' && currentStep === 1) {
                const percent = data.verify_percent || 0;
                document.getElementById('progress-verify').style.width = percent + '%';
                document.getElementById('percent-verify').textContent = percent + '%';
            } else if (step === 'flash' && currentStep === 2) {
                const percent = data.flash_percent || 0;
                document.getElementById('progress-flash').style.width = percent + '%';
                document.getElementById('percent-flash').textContent = percent + '%';
            } else if (step === 'reboot' && currentStep === 3) {
                document.getElementById('status-reboot').textContent = '正在重启...';
            }
        });
    }
    
    const fileUploadArea = document.getElementById('file-upload-area');
    const firmwareFileInput = document.getElementById('firmware-file-input');
    const selectedFile = document.getElementById('selected-file');
    const selectedFileName = document.getElementById('selected-file-name');
    const btnClearFile = document.getElementById('btn-clear-file');
    const btnLocalUpgrade = document.getElementById('btn-local-upgrade');
    
    if (fileUploadArea && firmwareFileInput) {
        fileUploadArea.addEventListener('click', () => firmwareFileInput.click());
        
        fileUploadArea.addEventListener('dragover', (e) => {
            e.preventDefault();
            fileUploadArea.classList.add('dragover');
        });
        
        fileUploadArea.addEventListener('dragleave', () => {
            fileUploadArea.classList.remove('dragover');
        });
        
        fileUploadArea.addEventListener('drop', (e) => {
            e.preventDefault();
            fileUploadArea.classList.remove('dragover');
            const files = e.dataTransfer.files;
            if (files.length > 0) {
                handleFirmwareFile(files[0]);
            }
        });
        
        firmwareFileInput.addEventListener('change', (e) => {
            if (e.target.files.length > 0) {
                handleFirmwareFile(e.target.files[0]);
            }
        });
    }
    
    function handleFirmwareFile(file) {
        const validExtensions = ['.bin', '.img'];
        const fileName = file.name.toLowerCase();
        const isValid = validExtensions.some(ext => fileName.endsWith(ext));
        
        if (!isValid) {
            showNotification('请选择 .bin 或 .img 格式的固件文件', 'error');
            return;
        }
        
        selectedFileName.textContent = file.name + ' (' + formatFileSize(file.size) + ')';
        selectedFile.style.display = 'flex';
        fileUploadArea.style.display = 'none';
        btnLocalUpgrade.disabled = false;
    }
    
    if (btnClearFile) {
        btnClearFile.addEventListener('click', () => {
            firmwareFileInput.value = '';
            selectedFile.style.display = 'none';
            fileUploadArea.style.display = 'flex';
            btnLocalUpgrade.disabled = true;
        });
    }
    
    if (btnLocalUpgrade) {
        btnLocalUpgrade.addEventListener('click', function() {
            const file = firmwareFileInput.files[0];
            if (!file) {
                showNotification('请选择固件文件', 'error');
                return;
            }
            
            if (!confirm('确定要上传并升级固件吗？升级过程中请勿断电！')) {
                return;
            }
            
            this.disabled = true;
            this.innerHTML = '<svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="10"></circle><polyline points="12 6 12 12 16 14"></polyline></svg> 上传中...';
            
            const formData = new FormData();
            formData.append('firmware', file);
            
            CPE_API.uploadFirmware(formData).then(() => {
                showNotification('固件上传成功，开始升级...', 'success');
                document.getElementById('upgrade-progress-section').style.display = 'block';
                pollUpgradeProgress();
            }).catch(error => {
                showNotification('上传失败: ' + error.message, 'error');
                this.disabled = false;
                this.innerHTML = '<svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4"></path><polyline points="17 8 12 3 7 8"></polyline><line x1="12" y1="3" x2="12" y2="15"></line></svg> 上传并升级';
            });
        });
    }

    const vpnTabs = document.querySelectorAll('#page-vpn .vpn-tab');
    const vpnContents = document.querySelectorAll('#page-vpn .vpn-content');
    
    vpnTabs.forEach(tab => {
        tab.addEventListener('click', function() {
            const vpnType = this.getAttribute('data-vpn');
            
            vpnTabs.forEach(t => t.classList.remove('active'));
            this.classList.add('active');
            
            vpnContents.forEach(content => {
                content.classList.remove('active');
            });
            
            const targetContent = document.getElementById('vpn-' + vpnType);
            if (targetContent) {
                targetContent.classList.add('active');
            }
            
            const typeMap = { 'pptp': 0, 'l2tp': 1, 'gre': 2, 'eoip': 3, 'ipsec': 4 };
            loadVpnConfig(typeMap[vpnType]);
        });
    });

    const wlanTabs = document.querySelectorAll('#page-wlan .vpn-tab');
    const wlanContents = document.querySelectorAll('#page-wlan .vpn-content');
    
    wlanTabs.forEach(tab => {
        tab.addEventListener('click', function() {
            const wlanType = this.getAttribute('data-wlan');
            
            wlanTabs.forEach(t => t.classList.remove('active'));
            this.classList.add('active');
            
            wlanContents.forEach(content => {
                content.classList.remove('active');
            });
            
            const targetContent = document.getElementById('wlan-' + wlanType);
            if (targetContent) {
                targetContent.classList.add('active');
            }
        });
    });

    const cellularTabs = document.querySelectorAll('#page-cellular .vpn-tab');
    const cellularContents = document.querySelectorAll('#page-cellular .vpn-content');
    
    cellularTabs.forEach(tab => {
        tab.addEventListener('click', function() {
            const cellularType = this.getAttribute('data-cellular');
            
            cellularTabs.forEach(t => t.classList.remove('active'));
            this.classList.add('active');
            
            cellularContents.forEach(content => {
                content.classList.remove('active');
            });
            
            const targetContent = document.getElementById('cellular-' + cellularType);
            if (targetContent) {
                targetContent.classList.add('active');
            }
        });
    });

    const firewallTabs = document.querySelectorAll('#page-firewall .vpn-tab');
    const firewallContents = document.querySelectorAll('#page-firewall .vpn-content');
    
    firewallTabs.forEach(tab => {
        tab.addEventListener('click', function() {
            const firewallType = this.getAttribute('data-firewall');
            
            firewallTabs.forEach(t => t.classList.remove('active'));
            this.classList.add('active');
            
            firewallContents.forEach(content => {
                content.classList.remove('active');
            });
            
            const targetContent = document.getElementById('firewall-' + firewallType);
            if (targetContent) {
                targetContent.classList.add('active');
            }
        });
    });

    const apnModal = document.getElementById('apn-modal');
    const apnList = document.getElementById('apn-list');
    let editingApnId = null;

    function openApnModal(mode, apnId = null) {
        const modal = document.getElementById('apn-modal');
        const title = document.getElementById('apn-modal-title');
        
        document.getElementById('apn-name-input').value = '';
        document.getElementById('apn-user-input').value = '';
        document.getElementById('apn-pass-input').value = '';
        document.getElementById('apn-auth-select').value = 'none';
        document.getElementById('apn-bearer-select').value = 'IP';
        document.getElementById('apn-active-check').checked = false;
        
        if (mode === 'edit' && apnId) {
            title.textContent = '编辑APN';
            editingApnId = apnId;
        } else {
            title.textContent = '添加APN';
            editingApnId = null;
        }
        
        modal.classList.add('show');
    }

    function closeApnModal() {
        document.getElementById('apn-modal').classList.remove('show');
        editingApnId = null;
    }

    function saveApn() {
        const name = document.getElementById('apn-name-input').value.trim();
        if (!name) {
            showNotification('请输入APN名称', 'error');
            return;
        }
        
        const apn = {
            name: name,
            username: document.getElementById('apn-user-input').value,
            password: document.getElementById('apn-pass-input').value,
            auth_type: document.getElementById('apn-auth-select').value === 'none' ? 0 : 
                       (document.getElementById('apn-auth-select').value === 'pap' ? 1 : 2),
            bearer_type: document.getElementById('apn-bearer-select').value === 'IP' ? 0 : 
                        (document.getElementById('apn-bearer-select').value === 'IPv4v6' ? 1 : 2),
            is_active: document.getElementById('apn-active-check').checked
        };
        
        if (editingApnId) {
            apn.id = parseInt(editingApnId);
            CPE_API.editApn(apn).then(() => {
                closeApnModal();
                loadApnList();
                showNotification('APN已更新', 'success');
            }).catch(error => {
                showNotification('更新失败: ' + error.message, 'error');
            });
        } else {
            CPE_API.addApn(apn).then(() => {
                closeApnModal();
                loadApnList();
                showNotification('APN已添加', 'success');
            }).catch(error => {
                showNotification('添加失败: ' + error.message, 'error');
            });
        }
    }

    document.getElementById('add-apn-btn').addEventListener('click', function() {
        openApnModal('add');
    });

    document.getElementById('apn-modal-close').addEventListener('click', closeApnModal);
    document.getElementById('apn-cancel-btn').addEventListener('click', closeApnModal);
    document.getElementById('apn-save-btn').addEventListener('click', saveApn);

    apnModal.addEventListener('click', function(e) {
        if (e.target === apnModal) {
            closeApnModal();
        }
    });

    const portModal = document.getElementById('port-modal');
    const portList = document.getElementById('port-forward-list');
    let editingPortId = null;

    function openPortModal(mode, portId = null) {
        const modal = document.getElementById('port-modal');
        const title = document.getElementById('port-modal-title');
        
        document.getElementById('port-protocol').value = 'TCP';
        document.getElementById('port-external').value = '';
        document.getElementById('port-internal-ip').value = '';
        document.getElementById('port-internal').value = '';
        document.getElementById('port-desc').value = '';
        document.getElementById('port-enabled').checked = true;
        
        if (mode === 'edit' && portId) {
            title.textContent = '编辑端口转发';
            editingPortId = portId;
        } else {
            title.textContent = '添加端口转发';
            editingPortId = null;
        }
        
        modal.classList.add('show');
    }

    function closePortModal() {
        document.getElementById('port-modal').classList.remove('show');
        editingPortId = null;
    }

    function savePort() {
        const protocol = document.getElementById('port-protocol').value;
        const external = document.getElementById('port-external').value.trim();
        const internalIp = document.getElementById('port-internal-ip').value.trim();
        const internal = document.getElementById('port-internal').value.trim();
        const desc = document.getElementById('port-desc').value.trim();
        const enabled = document.getElementById('port-enabled').checked;
        
        if (!external || !internalIp || !internal) {
            showNotification('请填写完整信息', 'error');
            return;
        }
        
        const rule = {
            protocol: protocol,
            external_port: parseInt(external),
            internal_ip: internalIp,
            internal_port: parseInt(internal),
            description: desc,
            enabled: enabled
        };
        
        CPE_API.addPortForward(rule).then(() => {
            closePortModal();
            loadFirewallConfig();
            showNotification('端口转发规则已添加', 'success');
        }).catch(error => {
            showNotification('添加失败: ' + error.message, 'error');
        });
    }

    document.getElementById('add-port-btn').addEventListener('click', function() {
        openPortModal('add');
    });

    document.getElementById('port-modal-close').addEventListener('click', closePortModal);
    document.getElementById('port-cancel-btn').addEventListener('click', closePortModal);
    document.getElementById('port-save-btn').addEventListener('click', savePort);

    portModal.addEventListener('click', function(e) {
        if (e.target === portModal) {
            closePortModal();
        }
    });

    document.getElementById('at-send-btn').addEventListener('click', function() {
        const atCommand = document.getElementById('at-command').value.trim();
        if (!atCommand) {
            showNotification('请输入AT指令', 'error');
            return;
        }
        
        const resultDiv = document.getElementById('at-result');
        resultDiv.innerHTML = `<pre>发送: ${atCommand}\n\n等待响应...</pre>`;
        
        CPE_API.sendAtCommand(atCommand, 5000).then(data => {
            resultDiv.innerHTML = `<pre>发送: ${data.command}\n\n响应: ${data.response}\n\n耗时: ${data.duration_ms}ms\n状态: ${data.success ? '成功' : '失败'}</pre>`;
        }).catch(error => {
            resultDiv.innerHTML = `<pre>发送: ${atCommand}\n\n错误: ${error.message}</pre>`;
        });
    });

    document.getElementById('at-clear-btn').addEventListener('click', function() {
        document.getElementById('at-command').value = '';
        document.getElementById('at-result').innerHTML = '<pre>等待发送AT指令...</pre>';
    });

    const simSwitchBtn = document.getElementById('sim-switch-btn');
    let currentSimValue = 1;
    
    if (simSwitchBtn) {
        simSwitchBtn.addEventListener('click', function() {
            const newSim = currentSimValue === 1 ? 2 : 1;
            currentSimValue = newSim;
            
            CPE_API.setCellularConfig({
                active_sim: newSim
            }).then(() => {
                showNotification('SIM卡切换成功', 'success');
                loadDeviceStatus();
            }).catch(error => {
                showNotification('SIM卡切换失败: ' + error.message, 'error');
                currentSimValue = newSim === 1 ? 2 : 1;
            });
        });
    }

    document.querySelectorAll('.link-type-option').forEach(option => {
        option.addEventListener('click', function(e) {
            e.stopPropagation();
            const linkType = this.getAttribute('data-type');
            
            CPE_API.setLinkType(linkType).then(() => {
                updateLinkStatus(linkType);
                showNotification('链路切换成功', 'success');
            }).catch(error => {
                showNotification('链路切换失败: ' + error.message, 'error');
            });
        });
    });

    document.querySelectorAll('.link-toggle-option').forEach(option => {
        option.addEventListener('click', function(e) {
            e.stopPropagation();
            const linkType = this.getAttribute('data-type');
            
            document.querySelectorAll('.link-toggle-option').forEach(opt => {
                opt.classList.remove('active');
            });
            this.classList.add('active');
            
            CPE_API.setLinkType(linkType).then(() => {
                updateLinkStatus(linkType);
                showNotification('链路切换成功', 'success');
            }).catch(error => {
                showNotification('链路切换失败: ' + error.message, 'error');
            });
        });
    });

    loadDeviceStatus();
});
