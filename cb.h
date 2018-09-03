#ifndef __CB_H__
#define __CB_H__

// callback function prototypes

#include <stdint.h>

enum ControlMode { MODE_FIXED_RPM = 0, MODE_MIN_MAX = 1 };

// Status & Control items
struct ControlData_t{
    double avgTemp = 0.0;
    uint32_t fanRPM = 0;
    ControlMode mode = MODE_FIXED_RPM;
    uint32_t fanRPMTarget = 800;
    uint8_t fanEffort = 40;
};

void helpCallback(char *tokens);
void resetCallback(char *tokens);
void brightCallback(char *tokens);
void statusCallback(char *tokens);

#endif
