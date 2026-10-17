#ifndef TempbarBridge_h
#define TempbarBridge_h

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Initializes AppleSMC connection and hardware sensor key cache
bool tempbar_init(void);

// Queries cached sensor values and returns telemetry in JSON format
const char* tempbar_get_json(void);

#ifdef __cplusplus
}
#endif

#endif
