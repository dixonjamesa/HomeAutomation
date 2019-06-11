/*
 * HAS_Pixels.cpp
 * 
 * (C) 2019 James Dixon
 * 
 * WS2812B LED support
 * Uses neopixel library
 * 
 */

#ifndef _PIXELS_H_
#define _PIXELS_H_

#define MAX_LED_COUNT 300


void InitPixels();
void UpdatePixels();
void SetPixels(int _count, int bright /* 0-255 */, int r, int g, int b);

#endif // _PIXELS_H_
