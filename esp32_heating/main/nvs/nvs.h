#pragma once

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

esp_err_t settings_storage_init(void);
int settings_read_all(void);
int settings_write_all(void);

#ifdef __cplusplus
}
#endif // __cplusplus