/*
 * HAS_Update.h
 *
 * (C) 2019 James Dixon
 *
 * Handle OTA updating
 *
 */

#ifndef _UPDATE_H_
#define _UPDATE_H_

#include <Arduino.h>

extern String UpdateURL;
extern bool UpdateNow;

void TestUpdate(); // call each loop
void TestServerUpdate(); // check server periodically and update if available

#endif //_UPDATE_H_
