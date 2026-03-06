#!/bin/bash

BUILD_SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
PROJECT_ROOT=$(dirname "$BUILD_SCRIPT_DIR")
BACKEND_DIR="${PROJECT_ROOT}/backend"

TARGET_ARCH=${TARGET_ARCH:-"aarch64"}
CROSS_COMPILE=${CROSS_COMPILE:-""}
BUILD_TYPE=${BUILD_TYPE:-"release"}
DESTDIR=${DESTDIR:-"${PROJECT_ROOT}/deploy/rootfs"}

print_banner() {
    echo "========================================"
    echo "  5G CPE Web Build System"
    echo "========================================"
    echo "Target Architecture: ${TARGET_ARCH}"
    echo "Cross Compile: ${CROSS_COMPILE:-native}"
    echo "Build Type: ${BUILD_TYPE}"
    echo "Deploy Dir: ${DESTDIR}"
    echo "========================================"
}

detect_cross_compiler() {
    if [ -z "$CROSS_COMPILE" ]; then
        case "$TARGET_ARCH" in
            aarch64|arm64)
                if command -v aarch64-linux-gnu-gcc &> /dev/null; then
                    CROSS_COMPILE="aarch64-linux-gnu-"
                elif [ -d "/opt/toolchain/aarch64-linux-gnu/bin" ]; then
                    export PATH="/opt/toolchain/aarch64-linux-gnu/bin:$PATH"
                    CROSS_COMPILE="aarch64-linux-gnu-"
                fi
                ;;
            arm|armhf)
                if command -v arm-linux-gnueabihf-gcc &> /dev/null; then
                    CROSS_COMPILE="arm-linux-gnueabihf-"
                elif [ -d "/opt/toolchain/arm-linux-gnueabihf/bin" ]; then
                    export PATH="/opt/toolchain/arm-linux-gnueabihf/bin:$PATH"
                    CROSS_COMPILE="arm-linux-gnueabihf-"
                fi
                ;;
            x86_64|x86)
                CROSS_COMPILE=""
                ;;
            *)
                echo "Unknown target architecture: $TARGET_ARCH"
                exit 1
                ;;
        esac
    fi
    
    if [ -n "$CROSS_COMPILE" ]; then
        CC="${CROSS_COMPILE}gcc"
        if ! command -v $CC &> /dev/null; then
            echo "Error: Cross compiler $CC not found!"
            exit 1
        fi
        echo "Using cross compiler: $CC"
    fi
}

build_backend() {
    echo "[1/3] Building backend CGI..."
    cd "${BACKEND_DIR}"
    
    DEBUG_FLAG=0
    if [ "$BUILD_TYPE" = "debug" ]; then
        DEBUG_FLAG=1
    fi
    
    make clean
    make CROSS_COMPILE="${CROSS_COMPILE}" DEBUG=${DEBUG_FLAG}
    
    if [ $? -ne 0 ]; then
        echo "Error: Backend build failed!"
        exit 1
    fi
    
    echo "Backend build successful!"
}

build_frontend() {
    echo "[2/3] Preparing frontend files..."
    
    mkdir -p "${DESTDIR}/www/html"
    
    cp -r "${PROJECT_ROOT}/css" "${DESTDIR}/www/html/"
    cp -r "${PROJECT_ROOT}/js" "${DESTDIR}/www/html/"
    cp "${PROJECT_ROOT}/index.html" "${DESTDIR}/www/html/"
    
    echo "Frontend files prepared!"
}

install_to_rootfs() {
    echo "[3/3] Installing to rootfs..."
    
    cd "${BACKEND_DIR}"
    make install DESTDIR="${DESTDIR}"
    
    mkdir -p "${DESTDIR}/etc/config"
    mkdir -p "${DESTDIR}/var/log"
    mkdir -p "${DESTDIR}/var/run"
    
    echo "Installation complete!"
}

create_package() {
    echo "Creating deployment package..."
    
    local TIMESTAMP=$(date +%Y%m%d_%H%M%S)
    local PACKAGE_NAME="5gcpe_web_${TARGET_ARCH}_${TIMESTAMP}.tar.gz"
    
    cd "${PROJECT_ROOT}/deploy"
    tar -czvf "${PACKAGE_NAME}" -C "${DESTDIR}" .
    
    echo "Package created: ${PROJECT_ROOT}/deploy/${PACKAGE_NAME}"
}

show_usage() {
    echo "Usage: $0 [options] [command]"
    echo ""
    echo "Commands:"
    echo "  build       Build the project (default)"
    echo "  clean       Clean build files"
    echo "  package     Build and create deployment package"
    echo "  help        Show this help message"
    echo ""
    echo "Options:"
    echo "  TARGET_ARCH=aarch64    Target architecture (aarch64, arm, x86_64)"
    echo "  CROSS_COMPILE=         Cross compiler prefix"
    echo "  BUILD_TYPE=release     Build type (release, debug)"
    echo "  DESTDIR=               Deployment directory"
    echo ""
    echo "Examples:"
    echo "  $0                                    # Native build"
    echo "  TARGET_ARCH=aarch64 $0                # Cross compile for ARM64"
    echo "  CROSS_COMPILE=aarch64-linux-gnu- $0  # Specify cross compiler"
    echo "  BUILD_TYPE=debug $0                   # Debug build"
    echo "  $0 package                            # Build and package"
}

main() {
    local COMMAND=${1:-"build"}
    
    case "$COMMAND" in
        build)
            print_banner
            detect_cross_compiler
            build_backend
            build_frontend
            install_to_rootfs
            echo ""
            echo "Build completed successfully!"
            ;;
        clean)
            cd "${BACKEND_DIR}"
            make clean
            rm -rf "${DESTDIR}"
            echo "Clean completed!"
            ;;
        package)
            print_banner
            detect_cross_compiler
            build_backend
            build_frontend
            install_to_rootfs
            create_package
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
