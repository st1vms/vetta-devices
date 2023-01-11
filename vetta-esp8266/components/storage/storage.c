#include <string.h>
#include <sys/unistd.h>
#include "esp_err.h"
#include "esp_spiffs.h"
#include "storage.h"

static esp_vfs_spiffs_conf_t conf = {
    .base_path = "/spiffs",
    .partition_label = NULL,
    .max_files = 4,
    .format_if_mount_failed = false};

static size_t totalSize = 0;
static size_t usedSize = 0;

static spiffs_string_t user_ap_password_string = {
    .filename = "/spiffs/uap.txt",
    .string_len = 0,
    .string_array = {0},
    .is_cached = 0};

static spiffs_string_t user_ap_ssid_string = {
    .filename = "/spiffs/ssid.txt",
    .string_len = 0,
    .string_array = {0},
    .is_cached = 0};

static esp_err_t init_spiffs(void)
{
    static esp_err_t _err;

    totalSize = 0;
    usedSize = 0;

    if (ESP_OK != (_err = esp_vfs_spiffs_register(&conf)) ||
        ESP_OK != (_err = esp_spiffs_info(NULL, &totalSize, &usedSize)) ||
        !totalSize)
    {
        return _err;
    }

    return ESP_OK;
}

static esp_err_t read_spiffs_string(spiffs_string_t *out_string)
{

    if (NULL == out_string ||
        NULL == out_string->filename ||
        NULL == out_string->string_array ||
        out_string->string_len == 0 ||
        out_string->string_len > MAX_STRING_LENGTH)
    {
        return ESP_ERR_INVALID_ARG;
    }

    if (out_string->is_cached)
    {
        return ESP_OK;
    }

    FILE *fp = fopen(out_string->filename, "r");
    if (!fp)
    {
        return ESP_FAIL;
    }

    memset(out_string->string_array, 0, out_string->string_len + 1);

    if (out_string->string_len != fread(out_string->string_array, sizeof(uint8_t), out_string->string_len, fp))
    {
        memset(out_string->string_array, 0, out_string->string_len + 1);
        fclose(fp);
        return ESP_FAIL;
    }

    fclose(fp);
    out_string->is_cached = 1;
    return ESP_OK;
}

static esp_err_t write_spiffs_string(const char *filename, const uint8_t *str, size_t str_len)
{

    if (NULL == filename ||
        NULL == str ||
        0 == str_len ||
        str_len > MAX_STRING_LENGTH)
    {
        return ESP_ERR_INVALID_ARG;
    }

    FILE *fp = fopen(filename, "w");
    if (!fp)
    {
        return ESP_FAIL;
    }

    if (str_len != fwrite(str, sizeof(uint8_t), str_len, fp))
    {
        fclose(fp);
        unlink(filename);
        return ESP_FAIL;
    }

    fclose(fp);
    return ESP_OK;
}

spiffs_string_t * get_user_ap_password_string(void)
{
    static esp_err_t _err;

    if (ESP_OK == (_err = init_spiffs()))
    {
        if (ESP_OK == (_err = read_spiffs_string(&user_ap_password_string)))
        {
            esp_vfs_spiffs_unregister(NULL);
            return &user_ap_password_string;
        }
        esp_vfs_spiffs_unregister(NULL);
    }
    return NULL;
}

spiffs_string_t * get_user_ap_ssid_string(void)
{

    static esp_err_t _err;

    if (ESP_OK == (_err = init_spiffs()))
    {
        if (ESP_OK == (_err = read_spiffs_string(&user_ap_ssid_string)))
        {
            esp_vfs_spiffs_unregister(NULL);
            return &user_ap_ssid_string;
        }
        esp_vfs_spiffs_unregister(NULL);
    }
    return NULL;
}

esp_err_t save_user_ap_password(const uint8_t *ap_pwd, size_t pwd_length)
{
    static esp_err_t _err;

    if (ESP_OK == (_err = init_spiffs()))
    {
        if (ESP_OK == (_err = write_spiffs_string(user_ap_password_string.filename, ap_pwd, pwd_length)))
        {
            user_ap_password_string.string_len = pwd_length;
            user_ap_password_string.is_cached = 0;
            esp_vfs_spiffs_unregister(NULL);
            return ESP_OK;
        }
        esp_vfs_spiffs_unregister(NULL);
    }
    return _err;
}

esp_err_t save_user_ap_ssid(const uint8_t *ap_ssid, size_t ssid_length)
{
    static esp_err_t _err;

    if (ESP_OK == (_err = init_spiffs()))
    {
        if (ESP_OK == (_err = write_spiffs_string(user_ap_ssid_string.filename, ap_ssid, ssid_length)))
        {
            user_ap_ssid_string.string_len = ssid_length;
            user_ap_ssid_string.is_cached = 0;
            esp_vfs_spiffs_unregister(NULL);
            return ESP_OK;
        }
        esp_vfs_spiffs_unregister(NULL);
    }
    return _err;
}

void spiffs_data_reset(void)
{
    static esp_err_t _err;
    if (ESP_OK == (_err = init_spiffs()))
    {

        // Unlink Wifi STA credentials

        unlink(user_ap_ssid_string.filename);
        user_ap_ssid_string.is_cached = 0;

        unlink(user_ap_password_string.filename);
        user_ap_password_string.is_cached = 0;

        esp_vfs_spiffs_unregister(NULL);
    }
}
