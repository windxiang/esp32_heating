#pragma once

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

extern uint8_t HandleTrigger;

bool loadHeatDefConfig(int8_t index);
void flushHeatConfig(void);
void _newHeatConfig(const char* pName);
void newHeatConfig(void);
void loadHeatConfig(void);
void flushMaxTemp(void);
void saveCurrentHeatData(void);
void renameHeatConfig(void);
void delHeatConfig(void);
void initMenuExpand(void);

_HeatSystemConfig* getHeatingSystemConfig(void);
_HeatingConfig* getCurrentHeatingConfig(void);

#ifdef __cplusplus
}
#endif // __cplusplus
