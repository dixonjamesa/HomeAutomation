/*
 * HAS_Animation.cpp
 *
 * (C) 2019 James Dixon
 *
 * RGB Animations
 * Handles all the RGB LED updates
 *
 */
#include "HAS_Animation.h"
#include "HAS_Pixels.h"
#include "HAS_option.h"

#define min(a,b) (((a)<(b))?(a):(b))
#define max(a,b) (((a)<(b))?(b):(a))

int gHue = 0;
int gSat = 0;
int seed = 0;
int changeDel = 0; // track how regularly to update animation values

int outR;
int outG;
int outB;
int targetR;
int targetG;
int targetB;
int bright;
int targetBright;
int StripLength;
int AnimMode;

void SetStripLength(int _c)
{
  StripLength = max(0, min(_c, options.RGBCount()));
  if( GetOutput(0) )
  {
    SetPixels(StripLength, bright, outR, outG, outB);
  }
}
int GetStripLength()
{
  return StripLength;
}


void SetAnimation(int _i)
{
  AnimMode = _i%Anim_Max;
  outR++; // do this to ensure colour is slightly off, so it's guaranteed to update next loop
  // (see output of AnimateColor as used in the UpdateAnimation method )
}

int GetAnimation()
{
    return AnimMode;
}

int GetBrightness()
{
  return targetBright;
}

void SetBrightness(int bri)
{
  targetBright = max(0, min(255, bri));
  options.Bright(targetBright);
}

/*
 * Set RGB from HSV values
 */
void SetRGBfromHSV(float H, float S, float V)
{
  float C, M, X;
  gHue = H;

  C = V*S;
  M = V-C;
  X = C*(1.0-fabs(fmod(H/60.0,2)-1.0));
  if( gHue >= 0 && gHue < 60 )
  {
     targetR = (C+M)*255;
     targetG = (X+M)*255;
     targetB = M*255;
  }
  else if( gHue >= 60 && gHue < 120 )
  {
     targetR = (X+M)*255;
     targetG = (C+M)*255;
     targetB = (M)*255;
  }
  else if( gHue >= 120 && gHue < 180 )
  {
     targetR = (M)*255;
     targetG = (C+M)*255;
     targetB = (X+M)*255;
  }
  else if( gHue >= 180 && gHue < 240 )
  {
     targetR = (M)*255;
     targetG = (X+M)*255;
     targetB = (C+M)*255;
  }
  else if( gHue >= 240 && gHue < 300 )
  {
     targetR = (X+M)*255;
     targetG = (M)*255;
     targetB = (C+M)*255;
  }
  else if( gHue >= 300 && gHue < 360 )
  {
     targetR = (C+M)*255;
     targetG = (M)*255;
     targetB = (X+M)*255;
  }
  else
  {
     targetR = (M)*255;
     targetG = (M)*255;
     targetB = (M)*255;
  }
  options.Red(targetR);
  options.Green(targetG);
  options.Blue(targetB);
}

void ChangeHue(int _amount )
{
  // convert from RGB to hue, add, then back to RGB:
  float R = ((float)targetR) / 255.0;
  float G = ((float)targetG) / 255.0;
  float B = ((float)targetB) / 255.0;
  float H, S, V;

  HSVFromRGB(H, S, V, R, G, B);
  gHue = H + _amount;
  if( gHue < 0 ) gHue += 360;
  if( gHue >= 360 ) gHue -= 360;
  gSat = S*100.0;

  // Now convert back to RGB:
  SetRGBfromHSV(gHue, S, V);
}

void ChangeSaturation(int _amount)
{
  // convert from RGB to hue, add, then back to RGB:
  float R = ((float)targetR) / 255.0;
  float G = ((float)targetG) / 255.0;
  float B = ((float)targetB) / 255.0;
  float H, S, V;

  HSVFromRGB(H, S, V, R, G, B);
  // Now convert back to RGB:
  gHue = H;
  gSat = min(100,max(0, S*100 + _amount*5));
  SetRGBfromHSV(H, gSat*0.01f, V);

}

void SetHue(int hue)
{
  gHue = min(359, max(0, hue));
  SetRGBfromHSV(gHue, ((float)gSat)/100.0, 1.0f);
  // note that V = Brightness, which is calculated post RGB when the pixels themselves are set
  // and the smooth transition is handled theough the RGB values
}
int GetHue()
{
  return gHue;
}

void SetSaturation(int sat)
{
  gSat = min(100,max(0,sat));
  SetRGBfromHSV(gHue, ((float)gSat)/100.0, 1.0f);
  // note that V = Brightness, which is calculated post RGB when the pixels themselves are set
  // and the smooth transition is handled theough the RGB values
}

int GetSaturation()
{
  return gSat;
}


void GetRGB(int &r, int &g, int &b)
{
  r = targetR;
  g = targetG;
  b = targetB;
}

void SetRGB(int r, int g, int b)
{
  targetR = r;
  options.Red(r);
  targetG = g;
  options.Green(g);
  targetB = b;
  options.Blue(b);
  SetPixels(StripLength, bright, outR, outG, outB);

  float R = ((float)targetR) / 255.0;
  float G = ((float)targetG) / 255.0;
  float B = ((float)targetB) / 255.0;
  float H, S, V;

  // store new hue and sat values:
  HSVFromRGB(H, S, V, R, G, B);
  gHue = H;
  gSat = min(100,max(0, S*100));
  Serial.println(gHue);
}

// H 0->360
// S 0->1
// V 0->1
void HSVFromRGB( float &H, float &S, float &V, float R, float G, float B)
{
  float minv = min( min(R, G), B);
  float maxv = max( max(R, G), B);
  if( minv != maxv ) // no hue!
  {
    //Find the minimum and maximum values of R, G and B.
    //Depending on what RGB color channel is the max value. The three different formulas are:
    if(R >= G && R >= B) { H = (G-B)/(maxv-minv); }
    if(G >= R && G >= B) { H = 2.0 + (B-R)/(maxv-minv); }
    if(B >= R && B >= G) { H = 4.0 + (R-G)/(maxv-minv); }
  }
  else
  {
    H = 0;
  }
  V = maxv;
  S = V==0?0:(maxv-minv)/maxv;
  H = H * 60.0;
  if(H<0) H+=360.0;
}


bool AnimateColor(bool);

void UpdateAnimation(bool _enabled, int _speed)
{
  bool change = AnimateColor(_enabled);
  switch( AnimMode )
  {
    case Anim_Static:
      if( change) SetPixels(StripLength, bright, outR, outG, outB);
      break;
    case Anim_CycleRGB:
      if( --changeDel <= 0 )
      {
        changeDel = _speed;
        gHue++;
        if( gHue >= 360 ) gHue -= 360;
        // Now convert back to RGB:
        SetRGBfromHSV(gHue, ((float)gSat)/100.0, 1.0f);
        //Serial.println(gHue);
      }
      SetPixels(StripLength, bright, outR, outG, outB);
      break;
    case Anim_Bounce:
      FadePixels(2);
      SetPixel(seed, bright, outR, outG, outB);
      if( --changeDel <= 0 )
      {
        changeDel = _speed;
        seed += 1;
        if( seed >= StripLength ) seed -= StripLength;
      }
      break;
  }
}

// smoothly transition colour to targetR/G/B
bool AnimateColor(bool _enabled)
{
  #define TRANSRATE 4
  int uprate = TRANSRATE;
  int downrate = TRANSRATE;//*4;
  bool change = false;

  // update RGB and brightness smoothly:
  int d = targetR - outR;
  d = min(uprate, max(-downrate, d));
  if( d != 0 ) change = true;
  outR += d;
  d = targetG - outG;
  d = min(uprate, max(-downrate, d));
  if( d != 0 ) change = true;
  outG += d;
  d = targetB - outB;
  d = min(uprate, max(-downrate, d));
  if( d != 0 ) change = true;
  outB += d;
  d = (_enabled?targetBright:0) - bright;
  d = min(uprate, max(-downrate, d));
  if( d != 0 ) change = true;
  bright += d;

  return change;
}
