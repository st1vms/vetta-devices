#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "mbedtls/md5.h"
#include "spiffs_storage.h"

static const char * ap_password_filename = "/spiffs/ap.txt";

static esp_vfs_spiffs_conf_t conf = {
    .base_path = "/spiffs",
    .partition_label = NULL,
    .max_files = 2,
    .format_if_mount_failed = false
};

static size_t totalSize = 0;
static size_t usedSize = 0;

static esp_err_t init_spiffs(void){
    static esp_err_t _err;

    totalSize = 0;
    usedSize = 0;

    if (ESP_OK != (_err = esp_vfs_spiffs_register(&conf)) ||
        ESP_OK != (_err = esp_spiffs_info(NULL, &totalSize, &usedSize))) {
        return _err;
    }
    return ESP_OK;
}


const char * get_ap_password_string(void){

    if(ESP_OK != init_spiffs()){
        return NULL;
    }

    ESP_LOGD("ap", "\nTot: %u\n", totalSize);
    ESP_LOGD("ap", "\nUsed: %u\n", usedSize);

    if(usedSize > AP_PASSWORD_LENGTH + 1){
        esp_vfs_spiffs_unregister(NULL);
        return NULL;
    }

    /* FILE * fp = fopen(ap_password_filename, "r");
    if(!fp){
        esp_vfs_spiffs_unregister(NULL);
        return NULL;
    }

    fclose(fp); */

    esp_vfs_spiffs_unregister(NULL);
    return NULL;
}
