#pragma once

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "esp_console.h"

void register_cmd(const char* command, const char* help, const char* hint, esp_console_cmd_func_t func, void* argtable);
void shell_init(void);

#ifdef __cplusplus
}
#endif // __cplusplus