#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "errno.h"
#include <sys/random.h>
#include "esp_err.h"
#include "esp_spiffs.h"
#include "storage.h"

static esp_vfs_spiffs_conf_t conf = {
    .base_path = "/spiffs",
    .partition_label = NULL,
    .max_files = 4,
    .format_if_mount_failed = true};

static size_t totalSize = 0;
static size_t usedSize = 0;

static const char *user_ap_pwd_filename = "/spiffs/uap.txt";
static const char *user_ap_ssid_filename = "/spiffs/ssid.txt";
static const char *lamp_seed_filename = "/spiffs/seed.txt";

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

static esp_err_t read_spiffs_string(const char *filename, spiffs_string_t *out_string)
{

    FILE *fp = fopen(filename, "r");
    if (!fp)
    {
        return ESP_FAIL;
    }

    out_string->string_len = 0;
    memset(out_string->string_array, 0, MAX_SPIFFS_STRING_LENGTH * sizeof(uint8_t));

    int len = fread(out_string->string_array, sizeof(uint8_t), MAX_SPIFFS_STRING_LENGTH, fp);
    if (len <= 0 || len >= MAX_SPIFFS_STRING_LENGTH)
    {
        memset(out_string->string_array, 0, MAX_SPIFFS_STRING_LENGTH * sizeof(uint8_t));
        fclose(fp);
        return ESP_FAIL;
    }
    fclose(fp);

    // NULL terminate string
    out_string->string_array[len] = 0;
    out_string->string_len = len;
    return ESP_OK;
}

static esp_err_t write_spiffs_string(const char *filename, const uint8_t *str, size_t str_len)
{

    if (NULL == filename ||
        NULL == str ||
        0 == str_len ||
        str_len > MAX_SPIFFS_STRING_LENGTH)
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

static esp_err_t read_lamp_seed(uint32_t * out){

    FILE *fp = fopen(lamp_seed_filename, "r");
    if (!fp){
        return ESP_FAIL;
    }

    uint32_t tmp = 0;
    if(1 != fread(&tmp, sizeof(uint32_t), 1, fp))
    {
        fclose(fp);
        unlink(lamp_seed_filename);
        return ESP_FAIL;
    }

    *out = tmp;
    fclose(fp);
    return ESP_OK;
}

static esp_err_t gen_lamp_seed(uint32_t * out){

    uint32_t tmp = 0;
    if(sizeof(uint32_t) == getrandom(&tmp, sizeof(uint32_t), 0)){

        FILE *fp = fopen(lamp_seed_filename, "w");
        if (!fp){
            return ESP_FAIL;
        }

        if(1 != fwrite(&tmp, sizeof(uint32_t), 1, fp))
        {
            fclose(fp);
            unlink(lamp_seed_filename);
            return ESP_FAIL;
        }
        fclose(fp);

        *out = tmp;
        return ESP_OK;
    }
    return ESP_FAIL;
}

esp_err_t get_lamp_seed(uint32_t * out){

    if (ESP_OK == init_spiffs())
    {
        if(ESP_OK == read_lamp_seed(out) ||
            ESP_OK == gen_lamp_seed(out))
        {
            esp_vfs_spiffs_unregister(NULL);
            return ESP_OK;
        }
        esp_vfs_spiffs_unregister(NULL);
    }
    return ESP_FAIL;
}

esp_err_t get_user_ap_password_string(spiffs_string_t *out_string)
{
    if (!out_string)
    {
        return ESP_FAIL;
    }
    static esp_err_t _err;

    if (ESP_OK == (_err = init_spiffs()))
    {
        if (ESP_OK == (_err = read_spiffs_string(user_ap_pwd_filename, out_string)))
        {
            esp_vfs_spiffs_unregister(NULL);
            return ESP_OK;
        }
        esp_vfs_spiffs_unregister(NULL);
    }
    return ESP_FAIL;
}

esp_err_t get_user_ap_ssid_string(spiffs_string_t *out_string)
{
    if (!out_string)
    {
        return ESP_FAIL;
    }

    static esp_err_t _err;

    if (ESP_OK == (_err = init_spiffs()))
    {
        if (ESP_OK == (_err = read_spiffs_string(user_ap_ssid_filename, out_string)))
        {
            esp_vfs_spiffs_unregister(NULL);
            return ESP_OK;
        }
        esp_vfs_spiffs_unregister(NULL);
    }
    return ESP_FAIL;
}

esp_err_t save_user_ap_password(const uint8_t *ap_pwd, size_t pwd_length)
{
    static esp_err_t _err;

    if (ESP_OK == (_err = init_spiffs()))
    {
        if (ESP_OK == (_err = write_spiffs_string(user_ap_pwd_filename, ap_pwd, pwd_length)))
        {
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

        if (ESP_OK == (_err = write_spiffs_string(user_ap_ssid_filename, ap_ssid, ssid_length)))
        {
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
        unlink(user_ap_ssid_filename);

        unlink(user_ap_pwd_filename);

        esp_vfs_spiffs_unregister(NULL);
    }
}
