/*
 * HAS_Animation.h
 *
 * (C) 2019 James Dixon
 *
 * RGB Animations
 * Handles all the RGB LED updates
 *
 */
#ifndef _ANIMATION_H_
#define _ANIMATION_H_

enum Animations
{
  Anim_Static = 0,
  Anim_CycleRGB,
  Anim_Bounce,
  Anim_Max
};

void SetRGB(int r, int g, int b );
void GetRGB(int &r, int &g, int &b);
void SetBrightness(int bri );
int GetBrightness();
void SetHue(int hue); // 0 to 360
int GetHue();
void SetSaturation(int sat);
int GetSaturation();
void SetRGBfromHSV(float H, float S, float V);
void HSVFromRGB( float &H, float &S, float &V, float R, float G, float B);

void ChangeHue(int _amount);
void ChangeSaturation(int _amount);

void SetAnimation(int _i); // set animation number
int GetAnimation(); // get animation number
void SetStripLength(int _c); // set number of RGB pixels in the neopixel string
int GetStripLength(); // get number of RGB pixels

void UpdateAnimation(bool _enabled, int _speed);

#endif // _ANIMATION_H_
