#!/bin/bash

DEBUG_SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
PROJECT_ROOT=$(dirname "$DEBUG_SCRIPT_DIR")

DEVICE_IP=${DEVICE_IP:-"192.168.1.1"}
DEVICE_USER=${DEVICE_USER:-"root"}
DEVICE_PASS=${DEVICE_PASS:-""}
CGI_PATH=${CGI_PATH:-"/www/cgi-bin/cpe_cgi"}

SSH_CMD="ssh"
SCP_CMD="scp"

if [ -n "$DEVICE_PASS" ]; then
    SSH_CMD="sshpass -p '${DEVICE_PASS}' ssh"
    SCP_CMD="sshpass -p '${DEVICE_PASS}' scp"
fi

print_banner() {
    echo "========================================"
    echo "  5G CPE Web Debug Tool"
    echo "========================================"
    echo "Device IP: ${DEVICE_IP}"
    echo "CGI Path: ${CGI_PATH}"
    echo "========================================"
}

quick_deploy() {
    echo "[Quick Deploy] Building and deploying CGI..."
    
    cd "${PROJECT_ROOT}/backend"
    make clean && make CROSS_COMPILE="${CROSS_COMPILE:-}" DEBUG=1
    
    if [ $? -ne 0 ]; then
        echo "Build failed!"
        exit 1
    fi
    
    $SSH_CMD ${DEVICE_USER}@${DEVICE_IP} "killall lighttpd 2>/dev/null || true"
    $SCP_CMD "${PROJECT_ROOT}/backend/build/cpe_cgi" ${DEVICE_USER}@${DEVICE_IP}:${CGI_PATH}
    $SSH_CMD ${DEVICE_USER}@${DEVICE_IP} "chmod +x ${CGI_PATH} && lighttpd -f /etc/lighttpd/lighttpd.conf"
    
    echo "Quick deploy complete!"
}

gdb_remote() {
    echo "Starting remote GDB session..."
    
    echo "Deploying gdbserver..."
    $SSH_CMD ${DEVICE_USER}@${DEVICE_IP} "killall cpe_cgi 2>/dev/null; killall gdbserver 2>/dev/null"
    
    $SSH_CMD ${DEVICE_USER}@${DEVICE_IP} "gdbserver :1234 ${CGI_PATH}" &
    GDBSERVER_PID=$!
    
    sleep 2
    
    echo "Connecting GDB..."
    gdb -ex "target remote ${DEVICE_IP}:1234" \
        -ex "file ${PROJECT_ROOT}/backend/build/cpe_cgi" \
        -ex "break main"
    
    kill $GDBSERVER_PID 2>/dev/null
}

strace_cgi() {
    echo "Running CGI with strace..."
    
    local ACTION=${1:-"status"}
    
    $SSH_CMD ${DEVICE_USER}@${DEVICE_IP} "strace -f -o /tmp/cgi_strace.log ${CGI_PATH} 'action=${ACTION}'"
    $SCP_CMD ${DEVICE_USER}@${DEVICE_IP}:/tmp/cgi_strace.log /tmp/cgi_strace.log
    
    echo "Strace log saved to /tmp/cgi_strace.log"
    cat /tmp/cgi_strace.log
}

watch_logs() {
    echo "Watching logs on device..."
    
    $SSH_CMD ${DEVICE_USER}@${DEVICE_IP} "tail -f /var/log/lighttpd/error.log /var/log/messages 2>/dev/null" &
    
    echo "Press Ctrl+C to stop..."
    wait
}

test_api() {
    local ACTION=${1:-"status"}
    
    echo "Testing API: ${ACTION}"
    echo "================================"
    
    curl -s "http://${DEVICE_IP}/cgi-bin/cpe_cgi?action=${ACTION}" | python3 -m json.tool 2>/dev/null || \
    curl -s "http://${DEVICE_IP}/cgi-bin/cpe_cgi?action=${ACTION}"
    
    echo ""
}

monitor_resources() {
    echo "Monitoring device resources..."
    
    $SSH_CMD ${DEVICE_USER}@${DEVICE_IP} "while true; do clear; echo '=== CPU/MEM ==='; top -bn1 | head -5; echo ''; echo '=== Disk ==='; df -h; echo ''; echo '=== Network ==='; ifconfig eth0 | grep -E 'RX|TX'; sleep 2; done"
}

reboot_device() {
    echo "Rebooting device..."
    $SSH_CMD ${DEVICE_USER}@${DEVICE_IP} "reboot"
}

show_usage() {
    echo "Usage: $0 [command]"
    echo ""
    echo "Commands:"
    echo "  quick       Quick build and deploy CGI (with debug symbols)"
    echo "  gdb         Start remote GDB debugging"
    echo "  strace      Run CGI with strace"
    echo "  logs        Watch device logs in real-time"
    echo "  test [api]  Test API endpoint"
    echo "  monitor     Monitor device resources"
    echo "  reboot      Reboot device"
    echo "  help        Show this help"
    echo ""
    echo "Environment Variables:"
    echo "  DEVICE_IP       Device IP address (default: 192.168.1.1)"
    echo "  DEVICE_USER     SSH username (default: root)"
    echo "  DEVICE_PASS     SSH password"
    echo "  CGI_PATH        CGI path on device (default: /www/cgi-bin/cpe_cgi)"
    echo "  CROSS_COMPILE   Cross compiler prefix"
    echo ""
    echo "Examples:"
    echo "  $0 quick                    # Quick deploy"
    echo "  $0 test status              # Test status API"
    echo "  $0 test lan_get             # Test LAN config API"
    echo "  DEVICE_IP=192.168.1.100 $0  # Specify device IP"
}

main() {
    local COMMAND=${1:-"help"}
    
    print_banner
    
    case "$COMMAND" in
        quick)
            quick_deploy
            ;;
        gdb)
            gdb_remote
            ;;
        strace)
            strace_cgi "$2"
            ;;
        logs)
            watch_logs
            ;;
        test)
            test_api "$2"
            ;;
        monitor)
            monitor_resources
            ;;
        reboot)
            reboot_device
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
