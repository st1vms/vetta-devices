#ifndef _STORAGE_H

#include <sys/unistd.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define MAX_STRING_LENGTH (64UL)

// 12 Numerical digits
#define AP_PASSWORD_LENGTH (12UL)
#define MAX_PASSWORD_LENGTH (64UL)
#define MAX_SSID_LENGTH (32UL)

    typedef struct spiffs_string_t
    {

        const char *filename;
        size_t string_len;
        char string_array[MAX_STRING_LENGTH + 1];
        unsigned char is_cached : 1;

    } spiffs_string_t;

    const spiffs_string_t *get_lamp_ap_password_string(void);

    const spiffs_string_t *get_user_ap_password_string(void);
    const spiffs_string_t *get_user_ap_ssid_string(void);

    esp_err_t save_user_ap_password(const char *ap_pwd, size_t pwd_length);
    esp_err_t save_user_ap_ssid(const char *ap_ssid, size_t ssid_length);

    void spiffs_data_reset(void);

#ifdef __cplusplus
}
#endif

#define _STORAGE_H
#endif //_STORAGE_H
