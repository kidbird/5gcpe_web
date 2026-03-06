#!/bin/bash

DEPLOY_SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
PROJECT_ROOT=$(dirname "$DEPLOY_SCRIPT_DIR")

DEVICE_IP=${DEVICE_IP:-"192.168.1.1"}
DEVICE_USER=${DEVICE_USER:-"root"}
DEVICE_PASS=${DEVICE_PASS:-""}
DEPLOY_DIR=${DEPLOY_DIR:-"/www"}
LOCAL_ROOTFS=${LOCAL_ROOTFS:-"${PROJECT_ROOT}/deploy/rootfs"}

print_banner() {
    echo "========================================"
    echo "  5G CPE Web Deployment Tool"
    echo "========================================"
    echo "Device IP: ${DEVICE_IP}"
    echo "Device User: ${DEVICE_USER}"
    echo "Deploy Dir: ${DEPLOY_DIR}"
    echo "========================================"
}

check_ssh_connection() {
    echo "Checking SSH connection to ${DEVICE_IP}..."
    
    if ssh -o ConnectTimeout=5 -o BatchMode=yes ${DEVICE_USER}@${DEVICE_IP} "echo OK" 2>/dev/null; then
        echo "SSH connection OK (key-based auth)"
        return 0
    fi
    
    if [ -n "$DEVICE_PASS" ]; then
        if command -v sshpass &> /dev/null; then
            echo "SSH connection OK (password auth)"
            return 0
        else
            echo "Error: sshpass not installed. Install with: apt-get install sshpass"
            return 1
        fi
    fi
    
    echo "Error: Cannot connect to device. Check SSH key or set DEVICE_PASS"
    return 1
}

deploy_files() {
    echo "Deploying files to device..."
    
    local SSH_CMD="ssh"
    local SCP_CMD="scp"
    
    if [ -n "$DEVICE_PASS" ]; then
        SSH_CMD="sshpass -p '${DEVICE_PASS}' ssh"
        SCP_CMD="sshpass -p '${DEVICE_PASS}' scp"
    fi
    
    echo "Stopping lighttpd..."
    $SSH_CMD ${DEVICE_USER}@${DEVICE_IP} "killall lighttpd 2>/dev/null || true"
    
    echo "Creating directories..."
    $SSH_CMD ${DEVICE_USER}@${DEVICE_IP} "mkdir -p ${DEPLOY_DIR}/cgi-bin ${DEPLOY_DIR}/html ${DEPLOY_DIR}/html/css ${DEPLOY_DIR}/html/js"
    
    echo "Copying CGI program..."
    $SCP_CMD "${LOCAL_ROOTFS}/www/cgi-bin/cpe_cgi" ${DEVICE_USER}@${DEVICE_IP}:${DEPLOY_DIR}/cgi-bin/
    
    echo "Copying web files..."
    $SCP_CMD -r "${LOCAL_ROOTFS}/www/html/"* ${DEVICE_USER}@${DEVICE_IP}:${DEPLOY_DIR}/html/
    
    echo "Setting permissions..."
    $SSH_CMD ${DEVICE_USER}@${DEVICE_IP} "chmod +x ${DEPLOY_DIR}/cgi-bin/cpe_cgi"
    
    echo "Starting lighttpd..."
    $SSH_CMD ${DEVICE_USER}@${DEVICE_IP} "lighttpd -f /etc/lighttpd/lighttpd.conf"
    
    echo "Deployment complete!"
}

restart_web_server() {
    echo "Restarting web server..."
    
    local SSH_CMD="ssh"
    if [ -n "$DEVICE_PASS" ]; then
        SSH_CMD="sshpass -p '${DEVICE_PASS}' ssh"
    fi
    
    $SSH_CMD ${DEVICE_USER}@${DEVICE_IP} "killall lighttpd 2>/dev/null; sleep 1; lighttpd -f /etc/lighttpd/lighttpd.conf"
    
    echo "Web server restarted!"
}

view_logs() {
    local SSH_CMD="ssh"
    if [ -n "$DEVICE_PASS" ]; then
        SSH_CMD="sshpass -p '${DEVICE_PASS}' ssh"
    fi
    
    $SSH_CMD ${DEVICE_USER}@${DEVICE_IP} "tail -f /var/log/lighttpd/error.log"
}

show_usage() {
    echo "Usage: $0 [command] [options]"
    echo ""
    echo "Commands:"
    echo "  deploy      Deploy files to device (default)"
    echo "  restart     Restart web server on device"
    echo "  logs        View web server logs"
    echo "  help        Show this help message"
    echo ""
    echo "Options:"
    echo "  DEVICE_IP=192.168.1.1     Device IP address"
    echo "  DEVICE_USER=root          Device SSH username"
    echo "  DEVICE_PASS=              Device SSH password"
    echo "  DEPLOY_DIR=/www           Deployment directory on device"
    echo ""
    echo "Examples:"
    echo "  $0                                    # Deploy to default device"
    echo "  DEVICE_IP=192.168.1.100 $0            # Deploy to specific IP"
    echo "  DEVICE_PASS=admin123 $0 deploy        # Deploy with password"
}

main() {
    local COMMAND=${1:-"deploy"}
    
    case "$COMMAND" in
        deploy)
            print_banner
            check_ssh_connection || exit 1
            deploy_files
            ;;
        restart)
            restart_web_server
            ;;
        logs)
            view_logs
            ;;
        help|--help|-h)
            show_usage
            ;;
        *)
            echo "Unknown command: $COMMAND"
            show_usage
            exit 1
            ;;
    esac
}

main "$@"
