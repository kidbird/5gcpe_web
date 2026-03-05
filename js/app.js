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
        });
    });

    refreshBtn.addEventListener('click', function() {
        this.classList.add('refreshing');
        setTimeout(() => {
            this.classList.remove('refreshing');
            showNotification('数据已刷新', 'success');
        }, 1000);
    });

    saveBtn.addEventListener('click', function() {
        showNotification('设置已保存', 'success');
    });

    const togglePasswordBtns = document.querySelectorAll('.toggle-password');
    togglePasswordBtns.forEach(btn => {
        btn.addEventListener('click', function() {
            const input = this.previousElementSibling;
            if (input.type === 'password') {
                input.type = 'text';
                this.innerHTML = `
                    <svg viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
                        <path d="M17.94 17.94C16.2306 19.243 14.1491 19.9649 12 20C5 20 1 12 1 12C2.24389 9.68192 3.96914 7.65663 6.06 6.06M9.9 4.24C10.5883 4.0789 11.2931 3.99836 12 4C19 4 23 12 23 12C22.393 13.1356 21.6691 14.2048 20.84 15.19M14.12 14.12C13.8454 14.4148 13.5141 14.6512 13.1462 14.8151C12.7782 14.9791 12.3809 15.0673 11.9781 15.0744C11.5753 15.0815 11.1752 15.0074 10.8016 14.8565C10.4281 14.7056 10.0887 14.4811 9.80385 14.1962C9.51897 13.9113 9.29439 13.572 9.14351 13.1984C8.99262 12.8249 8.91853 12.4247 8.92563 12.0219C8.93274 11.6191 9.02091 11.2218 9.18488 10.8538C9.34884 10.4859 9.58525 10.1546 9.88 9.88" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/>
                        <path d="M1 1L23 23" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/>
                    </svg>
                `;
            } else {
                input.type = 'password';
                this.innerHTML = `
                    <svg viewBox="0 0 24 24" fill="none" xmlns="http://www.w3.org/2000/svg">
                        <path d="M1 12S5 4 12 4C19 4 23 12 23 12S19 20 12 20C5 20 1 12 1 12Z" stroke="currentColor" stroke-width="2"/>
                        <circle cx="12" cy="12" r="3" stroke="currentColor" stroke-width="2"/>
                    </svg>
                `;
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
            if (vpnStatus.classList.contains('disconnected')) {
                vpnStatus.textContent = '连接中...';
                vpnStatus.classList.remove('disconnected');
                vpnStatus.classList.add('connecting');
                
                setTimeout(() => {
                    vpnStatus.textContent = '已连接';
                    vpnStatus.classList.remove('connecting');
                    vpnStatus.classList.add('connected');
                    showNotification('VPN连接成功', 'success');
                }, 2000);
            } else {
                vpnStatus.textContent = '断开连接中...';
                setTimeout(() => {
                    vpnStatus.textContent = '未连接';
                    vpnStatus.classList.remove('connected');
                    vpnStatus.classList.add('disconnected');
                    showNotification('VPN已断开', 'success');
                }, 1000);
            }
        });
    }

    const restartBtn = document.querySelector('#page-system .btn-primary');
    if (restartBtn) {
        restartBtn.addEventListener('click', function() {
            if (confirm('确定要重启设备吗？')) {
                showNotification('设备正在重启...', 'success');
            }
        });
    }

    const resetBtn = document.querySelector('#page-system .btn-secondary');
    if (resetBtn) {
        resetBtn.addEventListener('click', function() {
            if (confirm('确定要恢复出厂设置吗？所有配置将被清除！')) {
                showNotification('正在恢复出厂设置...', 'success');
            }
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
        document.getElementById('apn-type-default').checked = true;
        document.getElementById('apn-type-mms').checked = false;
        document.getElementById('apn-type-supl').checked = false;
        document.getElementById('apn-type-dun').checked = false;
        
        if (mode === 'edit' && apnId) {
            title.textContent = '编辑APN';
            editingApnId = apnId;
            const apnItem = apnList.querySelector(`[data-apn="${apnId}"]`);
            if (apnItem) {
                const name = apnItem.querySelector('.apn-name').textContent.replace(' (已激活)', '');
                document.getElementById('apn-name-input').value = name;
                
                const details = apnItem.querySelectorAll('.apn-value');
                if (details[1]) document.getElementById('apn-user-input').value = details[1].textContent === '-' ? '' : details[1].textContent;
                if (details[2]) document.getElementById('apn-pass-input').value = '';
                if (details[3]) {
                    const authMap = { '无': 'none', 'PAP': 'pap', 'CHAP': 'chap' };
                    document.getElementById('apn-auth-select').value = authMap[details[3].textContent] || 'none';
                }
                if (details[4]) document.getElementById('apn-bearer-select').value = details[4].textContent;
                
                const isActive = apnItem.querySelector('.apn-status').classList.contains('enabled');
                document.getElementById('apn-active-check').checked = isActive;
            }
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
        
        const user = document.getElementById('apn-user-input').value || '-';
        const pass = document.getElementById('apn-pass-input').value || '-';
        const auth = document.getElementById('apn-auth-select').value;
        const authText = auth === 'none' ? '无' : auth.toUpperCase();
        const bearer = document.getElementById('apn-bearer-select').value;
        const isActive = document.getElementById('apn-active-check').checked;
        
        if (editingApnId) {
            const apnItem = apnList.querySelector(`[data-apn="${editingApnId}"]`);
            if (apnItem) {
                apnItem.querySelector('.apn-name').textContent = name + (isActive ? ' (已激活)' : '');
                apnItem.querySelector('.apn-status').textContent = isActive ? '已激活' : '未激活';
                apnItem.querySelector('.apn-status').className = 'apn-status ' + (isActive ? 'enabled' : 'disabled');
                
                const details = apnItem.querySelectorAll('.apn-value');
                details[0].textContent = name;
                details[1].textContent = user;
                details[2].textContent = pass === '-' ? '-' : '******';
                details[3].textContent = authText;
                details[4].textContent = bearer;
            }
        } else {
            const newId = Date.now();
            const newApnHtml = `
                <div class="apn-item" data-apn="${newId}">
                    <div class="apn-header">
                        <span class="apn-name">${name}${isActive ? ' (已激活)' : ''}</span>
                        <span class="apn-status ${isActive ? 'enabled' : 'disabled'}">${isActive ? '已激活' : '未激活'}</span>
                    </div>
                    <div class="apn-details">
                        <div class="apn-detail-row">
                            <span class="apn-label">APN名称</span>
                            <span class="apn-value">${name}</span>
                        </div>
                        <div class="apn-detail-row">
                            <span class="apn-label">用户名</span>
                            <span class="apn-value">${user}</span>
                        </div>
                        <div class="apn-detail-row">
                            <span class="apn-label">密码</span>
                            <span class="apn-value">${pass === '-' ? '-' : '******'}</span>
                        </div>
                        <div class="apn-detail-row">
                            <span class="apn-label">认证类型</span>
                            <span class="apn-value">${authText}</span>
                        </div>
                        <div class="apn-detail-row">
                            <span class="apn-label">承载类型</span>
                            <span class="apn-value">${bearer}</span>
                        </div>
                    </div>
                    <div class="apn-actions">
                        <button class="btn btn-primary btn-sm edit-apn-btn">编辑</button>
                        <button class="btn btn-secondary btn-sm delete-apn-btn">删除</button>
                    </div>
                </div>
            `;
            apnList.insertAdjacentHTML('beforeend', newApnHtml);
            
            const newItem = apnList.querySelector(`[data-apn="${newId}"]`);
            newItem.querySelector('.edit-apn-btn').addEventListener('click', function() {
                openApnModal('edit', newId);
            });
            newItem.querySelector('.delete-apn-btn').addEventListener('click', function() {
                if (confirm('确定要删除此APN吗？')) {
                    newItem.remove();
                    showNotification('APN已删除', 'success');
                }
            });
        }
        
        closeApnModal();
        showNotification(editingApnId ? 'APN已更新' : 'APN已添加', 'success');
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

    document.querySelectorAll('.edit-apn-btn').forEach(btn => {
        btn.addEventListener('click', function() {
            const apnItem = this.closest('.apn-item');
            const apnId = apnItem.getAttribute('data-apn');
            openApnModal('edit', apnId);
        });
    });

    document.querySelectorAll('.delete-apn-btn').forEach(btn => {
        btn.addEventListener('click', function() {
            const apnItem = this.closest('.apn-item');
            if (confirm('确定要删除此APN吗？')) {
                apnItem.remove();
                showNotification('APN已删除', 'success');
            }
        });
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
            const portItem = portList.querySelector(`[data-port="${portId}"]`);
            if (portItem) {
                const info = portItem.querySelector('.port-info').textContent;
                const match = info.match(/(TCP|UDP|TCP\/UDP):(\d+)\s*→\s*(\d+\.\d+\.\d+\.\d+):(\d+)/);
                if (match) {
                    document.getElementById('port-protocol').value = match[1];
                    document.getElementById('port-external').value = match[2];
                    document.getElementById('port-internal-ip').value = match[3];
                    document.getElementById('port-internal').value = match[4];
                }
            }
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
        const enabled = document.getElementById('port-enabled').checked;
        
        if (!external || !internalIp || !internal) {
            showNotification('请填写完整信息', 'error');
            return;
        }
        
        const portInfo = `${protocol}:${external} → ${internalIp}:${internal}`;
        
        if (editingPortId) {
            const portItem = portList.querySelector(`[data-port="${editingPortId}"]`);
            if (portItem) {
                portItem.querySelector('.port-info').textContent = portInfo;
            }
        } else {
            const newId = Date.now();
            const newPortHtml = `
                <div class="port-forward-item" data-port="${newId}">
                    <span class="port-info">${portInfo}</span>
                    <div class="port-actions">
                        <button class="btn btn-primary btn-sm edit-port-btn">编辑</button>
                        <button class="btn btn-secondary btn-sm delete-port-btn">删除</button>
                    </div>
                </div>
            `;
            portList.insertAdjacentHTML('beforeend', newPortHtml);
            
            const newItem = portList.querySelector(`[data-port="${newId}"]`);
            newItem.querySelector('.edit-port-btn').addEventListener('click', function() {
                openPortModal('edit', newId);
            });
            newItem.querySelector('.delete-port-btn').addEventListener('click', function() {
                if (confirm('确定要删除此端口转发规则吗？')) {
                    newItem.remove();
                    showNotification('端口转发规则已删除', 'success');
                }
            });
        }
        
        closePortModal();
        showNotification(editingPortId ? '端口转发规则已更新' : '端口转发规则已添加', 'success');
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

    document.querySelectorAll('.edit-port-btn').forEach(btn => {
        btn.addEventListener('click', function() {
            const portItem = this.closest('.port-forward-item');
            const portId = portItem.getAttribute('data-port');
            openPortModal('edit', portId);
        });
    });

    document.querySelectorAll('.delete-port-btn').forEach(btn => {
        btn.addEventListener('click', function() {
            const portItem = this.closest('.port-forward-item');
            if (confirm('确定要删除此端口转发规则吗？')) {
                portItem.remove();
                showNotification('端口转发规则已删除', 'success');
            }
        });
    });

    document.getElementById('at-send-btn').addEventListener('click', function() {
        const atCommand = document.getElementById('at-command').value.trim();
        if (!atCommand) {
            showNotification('请输入AT指令', 'error');
            return;
        }
        
        const resultDiv = document.getElementById('at-result');
        resultDiv.innerHTML = `<pre>发送: ${atCommand}\n\n接收: OK\n+CME ERROR: 0</pre>`;
    });

    document.getElementById('at-clear-btn').addEventListener('click', function() {
        document.getElementById('at-command').value = '';
        document.getElementById('at-result').innerHTML = '<pre>等待发送AT指令...</pre>';
    });
});
