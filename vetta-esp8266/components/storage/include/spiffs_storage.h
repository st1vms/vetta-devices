#ifndef __SPIFFS_STORAGE_H

#include <sys/unistd.h>

#ifdef __cplusplus
extern "C"
{
#endif

// 8 Numerical digits
#define AP_PASSWORD_LENGTH 12UL

/**
 * @brief Get the ap password array object
 *
 * @return const uint8_t* or NULL in case of errors
 */
const char *get_ap_password_cstring(void);


#ifdef __cplusplus
}
#endif

#define __SPIFFS_STORAGE_H
#endif //__SPIFFS_STORAGE_H
