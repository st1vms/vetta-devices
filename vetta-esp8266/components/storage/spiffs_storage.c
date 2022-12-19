#include <string.h>
#include "esp_err.h"
#include "esp_spiffs.h"
#include "spiffs_storage.h"

static esp_vfs_spiffs_conf_t conf = {
    .base_path = "/spiffs",
    .partition_label = NULL,
    .max_files = 1,
    .format_if_mount_failed = true};

static size_t rlen = 0;

static size_t totalSize = 0;
static size_t usedSize = 0;

static const char *ap_password_filename = "/spiffs/ap";
static char pass[AP_PASSWORD_LENGTH + 1] = {0};

static esp_err_t init_spiffs(void)
{
    static esp_err_t _err;

    totalSize = 0;
    usedSize = 0;

    if (ESP_OK != (_err = esp_vfs_spiffs_register(&conf)) ||
        ESP_OK != (_err = esp_spiffs_info(NULL, &totalSize, &usedSize)) ||
        totalSize <= AP_PASSWORD_LENGTH ||
        usedSize <= AP_PASSWORD_LENGTH)
    {
        return _err;
    }

    return ESP_OK;
}

static const char *__get_chached_password_array(void)
{

    static unsigned char z;
    // Check if pass array is missing NULL termination
    if (0 != (z = pass[AP_PASSWORD_LENGTH]))
    {
        rlen = 0;
        memset(pass, 0, AP_PASSWORD_LENGTH + 1);
        return NULL;
    }

    // Check for zero filled array
    for (unsigned char i = 0; i < AP_PASSWORD_LENGTH; i++)
    {
        z = (pass[i] != 0) ? 1 : 0;
        if (z)
        {
            break;
        }
    }

    if (!z)
    {
        rlen = 0;
        memset(pass, 0, AP_PASSWORD_LENGTH + 1);
        return NULL;
    }

    return (const char *)pass;
}

const char *get_ap_password_cstring(void)
{
    if (rlen == AP_PASSWORD_LENGTH)
    {
        return __get_chached_password_array();
    }

    if (ESP_OK != init_spiffs())
    {
        rlen = 0;
        esp_vfs_spiffs_unregister(NULL);
        return NULL;
    }

    FILE *fp = fopen(ap_password_filename, "rb");
    if (!fp)
    {
        rlen = 0;
        esp_vfs_spiffs_unregister(NULL);
        return NULL;
    }

    memset(pass, 0, AP_PASSWORD_LENGTH + 1);
    rlen = fread(pass, sizeof(char), AP_PASSWORD_LENGTH, fp);
    if (rlen != AP_PASSWORD_LENGTH)
    {
        rlen = 0;
        memset(pass, 0, AP_PASSWORD_LENGTH + 1);
    }

    fclose(fp);
    esp_vfs_spiffs_unregister(NULL);
    return (rlen != AP_PASSWORD_LENGTH) ? NULL : (const char *)pass;
}
