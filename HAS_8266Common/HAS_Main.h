/*
 * HAS_Main.h
 *
 * (C) 2019 James Dixon
 *
 * Main setup functionality
 *
 */
#ifndef _MAIN_H_
#define _MAIN_H_

#include "config.h"
#include "HAS_Switch.h"
#include <PubSubClient.h>

void tryConnectWIFI();
void disconnectWIFI();
extern char WIFI_status[];
extern PubSubClient PSclient;

void PublishStatus();

/*
 * Set output state
 */
void SetOutput( int _id /* 1-n */, bool _state, bool _toggle=false, int _av=255 );
bool GetOutput(int _id /* 1-n */);
extern T_Switch switches[NUM_SWITCHES];

// Used for controlling RGB strips:
#define MODE_BRIGHTNESS 0
#define MODE_HUE 1
#define MODE_SATURATION 2
#define MODE_LENGTH 3
#define MODE_MAX 4

int GetMode();
int ChangeMode(int _m = -1);
void ChangeValue(int _amount);


#endif // _MAIN_H_
