/*
 * A library for storing parameters in a key file
 * 
 * This library is an analog to the windows freeze library
 * developed by Jonathan Westhues.
 * 
 * R Ramana, 2018
 */

#ifndef __FREEZE_H
#define __FREEZE_H

#define FREEZE_REGISTER ".ldmicro"

#define FREEZE_SUBKEY "LDMicro"

// #ifndef FREEZE_SUBKEY
// #error must define FREEZE_SUBKEY to a string uniquely identifying the app
// #endif

#define FreezeWindowPos(hwnd) FreezeWindowPosF(hwnd, FREEZE_SUBKEY, #hwnd)
void FreezeWindowPosF(HWID hWid, char *subKey, char *name);

#define ThawWindowPos(hwnd) ThawWindowPosF(hwnd, FREEZE_SUBKEY, #hwnd)
void ThawWindowPosF(HWID hWid, char *subKey, char *name);

#define FreezeDWORD(val) FreezeDWORDF(val, FREEZE_SUBKEY, #val)
void FreezeDWORDF(DWORD val, char *subKey, char *name);

#define ThawDWORD(val) val = ThawDWORDF(val, FREEZE_SUBKEY, #val)
DWORD ThawDWORDF(DWORD val, char *subKey, char *name);

#define FreezeString(val) FreezeStringF(val, FREEZE_SUBKEY, #val)
void FreezeStringF(char *val, char *subKey, char *name);

#define ThawString(val, max) ThawStringF(val, max, FREEZE_SUBKEY, #val)
void ThawStringF(char *val, int max, char *subKey, char *name);

typedef union regKeyVal{
    int i;
    float f;
    bool b;
    DWORD D;
} KeyVal;


typedef struct regKeys{
    char type;
    KeyVal val;
} Key;

#endif
