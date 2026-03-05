#include "cpe_control.h"
#include "module_at.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

extern CpeControlOps* at_backend_create(void);
extern CpeControlOps* api_backend_create(void);

static CpeControlOps *g_control_ops = NULL;
static CpeConfig g_config;

const char* cpe_control_get_mode_name(CpeControlMode mode)
{
    switch (mode) {
        case CPE_CTRL_MODE_AT:
            return "AT Command";
        case CPE_CTRL_MODE_API:
            return "OpenCPU API";
        default:
            return "Unknown";
    }
}

CpeControlMode cpe_control_get_mode_from_string(const char *mode_str)
{
    if (strcmp(mode_str, "at") == 0 || strcmp(mode_str, "AT") == 0) {
        return CPE_CTRL_MODE_AT;
    } else if (strcmp(mode_str, "api") == 0 || strcmp(mode_str, "API") == 0) {
        return CPE_CTRL_MODE_API;
    }
    return CPE_CTRL_MODE_AT;
}

CpeControlOps* cpe_control_create(CpeControlMode mode)
{
    switch (mode) {
        case CPE_CTRL_MODE_AT:
            return at_backend_create();
        case CPE_CTRL_MODE_API:
            return api_backend_create();
        default:
            return NULL;
    }
}

void cpe_control_destroy(CpeControlOps *ops)
{
    if (ops) {
        if (ops->close) {
            ops->close();
        }
        if (ops->context) {
            free(ops->context);
        }
        free(ops);
    }
}

CpeCtrlResult cpe_control_init(CpeControlOps *ops, const char *config_file)
{
    CpeConfig config;

    if (config_file) {
        if (cpe_config_load(config_file, &config) != 0) {
            config.mode = CPE_CTRL_MODE_AT;
            strcpy(config.at_device, "/dev/ttyUSB2");
            strcpy(config.api_socket, "127.0.0.1");
            config.api_port = 9000;
            config.debug_enabled = false;
        }
    } else {
        config.mode = CPE_CTRL_MODE_AT;
        strcpy(config.at_device, "/dev/ttyUSB2");
        strcpy(config.api_socket, "127.0.0.1");
        config.api_port = 9000;
        config.debug_enabled = false;
    }

    memcpy(&g_config, &config, sizeof(CpeConfig));

    if (ops && ops->init) {
        if (config.mode == CPE_CTRL_MODE_AT) {
            return ops->init(config.at_device);
        } else {
            return ops->init(NULL);
        }
    }

    return CTRL_OK;
}

static const char* trim(char *str)
{
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return str;

    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';

    return str;
}

int cpe_config_load(const char *config_file, CpeConfig *config)
{
    FILE *fp = fopen(config_file ? config_file : "/etc/cpe/cpe.conf", "r");
    if (!fp) {
        return -1;
    }

    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        char *p = strchr(line, '#');
        if (p) *p = '\0';

        p = strchr(line, '=');
        if (!p) continue;

        *p = '\0';
        char *key = trim(line);
        char *value = trim(p + 1);

        if (strcmp(key, "mode") == 0) {
            config->mode = cpe_control_get_mode_from_string(value);
        } else if (strcmp(key, "module_type") == 0) {
            config->module_type = module_type_from_string(value);
        } else if (strcmp(key, "at_device") == 0) {
            strncpy(config->at_device, value, sizeof(config->at_device) - 1);
        } else if (strcmp(key, "api_host") == 0 || strcmp(key, "api_socket") == 0) {
            strncpy(config->api_socket, value, sizeof(config->api_socket) - 1);
        } else if (strcmp(key, "api_port") == 0) {
            config->api_port = atoi(value);
        } else if (strcmp(key, "debug") == 0) {
            config->debug_enabled = (strcmp(value, "1") == 0 || strcmp(value, "true") == 0);
        }
    }

    fclose(fp);
    return 0;
}

int cpe_config_save(const char *config_file, const CpeConfig *config)
{
    FILE *fp = fopen(config_file ? config_file : "/etc/cpe/cpe.conf", "w");
    if (!fp) {
        return -1;
    }

    fprintf(fp, "# CPE Control Configuration\n\n");
    fprintf(fp, "# Control mode: at or api\n");
    fprintf(fp, "mode=%s\n\n", config->mode == CPE_CTRL_MODE_AT ? "at" : "api");

    fprintf(fp, "# 5G Module type\n");
    fprintf(fp, "module_type=%s\n\n", module_type_to_string(config->module_type));

    fprintf(fp, "# AT command device (used in AT mode)\n");
    fprintf(fp, "at_device=%s\n\n", config->at_device);

    fprintf(fp, "# API server (used in API mode)\n");
    fprintf(fp, "api_host=%s\n", config->api_socket);
    fprintf(fp, "api_port=%d\n\n", config->api_port);

    fprintf(fp, "# Debug mode: 1 or 0\n");
    fprintf(fp, "debug=%d\n", config->debug_enabled ? 1 : 0);

    fclose(fp);
    return 0;
}

CpeControlOps* cpe_get_control_ops(void)
{
    return g_control_ops;
}

int cpe_control_setup(CpeControlMode mode, const char *config_file)
{
    if (g_control_ops) {
        cpe_control_destroy(g_control_ops);
        g_control_ops = NULL;
    }

    g_control_ops = cpe_control_create(mode);
    if (!g_control_ops) {
        return -1;
    }

    CpeCtrlResult ret = cpe_control_init(g_control_ops, config_file);
    if (ret != CTRL_OK) {
        cpe_control_destroy(g_control_ops);
        g_control_ops = NULL;
        return -1;
    }

    return 0;
}

void cpe_control_cleanup(void)
{
    if (g_control_ops) {
        cpe_control_destroy(g_control_ops);
        g_control_ops = NULL;
    }
}
