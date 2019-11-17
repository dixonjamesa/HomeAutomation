/*
 * HAS_Pixels.cpp
 *
 * (C) 2019 James Dixon
 *
 * WS2812B LED support
 * Uses adafruit neopixel library
 *
 */
#include "HAS_Pixels.h"
#include "HAS_option.h"

//#include <Adafruit_NeoPixel.h>
#include <NeoPixelBus.h>

//NeoPixelBus<NeoGrbFeature, NeoEsp8266BitBang800KbpsMethod> *strip = NULL;
NeoPixelBus<NeoGrbFeature, NeoEsp8266Dma800KbpsMethod> strip(MAX_LED_COUNT, 0); // pin ignored - attach to GPIO3
//Adafruit_NeoPixel strip = Adafruit_NeoPixel(MAX_LED_COUNT, OUT1_PIN, NEO_GRB + NEO_KHZ800);

void InitPixels()
{
  if( options.OutType(1) == OUTTYPE_RGB )
  {
    Serial.print("Nepixel string with ");
    Serial.print(options.RGBCount());
    Serial.println(" LEDs");
    //strip = new NeoPixelBus<NeoGrbFeature, NeoEsp8266BitBang800KbpsMethod>(MAX_LED_COUNT, options.OutPin(1));
    strip.Begin();
    strip.ClearTo(0);
    strip.Show();
    SetPixels(MAX_LED_COUNT, 0,128,0,128);
  }
}

void UpdatePixels()
{
  if( options.OutType(1) == OUTTYPE_RGB )
  {
    strip.Show();
  }
}

void SetPixel(int _id, int bri /* 0-255 */, int r, int g, int b)
{
  r = r*bri/255;
  g = g*bri/255;
  b = b*bri/255;
  strip.SetPixelColor(_id, RgbColor(r,g,b));
}
void FadePixels(int _amount)
{
  RgbColor rgb;
  for(int loc = 0 ; loc < options.RGBCount() ; loc++)
  {
    rgb = strip.GetPixelColor(loc);
    rgb.Darken(_amount);
    strip.SetPixelColor(loc, rgb);
  }
}

// _id starts at 1
void SetPixels( int _count, int bri, int r, int g, int b)
{
    int loc;
 #if false
    Serial.print("Setting ");
    Serial.print(_count);
    Serial.print(" (of ");
    Serial.print(options.RGBCount());
    Serial.print(") RGBs to ");
    Serial.print(bri*100/255);
    Serial.print("% of ");
    Serial.print(r);
    Serial.print(",");
    Serial.print(g);
    Serial.print(",");
    Serial.print(b);
    Serial.println(".");
 #endif
    r = r*bri/255;
    g = g*bri/255;
    b = b*bri/255;
    for(loc = 0 ; loc < min(_count, options.RGBCount()) ; loc++)
    {
      strip.SetPixelColor(loc, RgbColor(r,g,b));
    }
    for( ; loc < MAX_LED_COUNT ; loc++)
    {
      strip.SetPixelColor(loc, RgbColor(0,0,0));
    }
}
