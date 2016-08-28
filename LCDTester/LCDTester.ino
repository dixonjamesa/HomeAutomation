#include <Adafruit_GFX.h> // Core graphics library
#include <Adafruit_TFTLCD.h> // Hardware-specific library
#include <TouchScreen.h>
//#include <SWTFT.h> // Hardware-specific library

#define LCD_CS A3
#define LCD_CD A2
#define LCD_WR A1
#define LCD_RD A0
#define LCD_RESET A4
#define SD_CS 10

#define YP A2 // must be an analog pin, use "An" notation!
#define XM A3 // must be an analog pin, use "An" notation!
#define YM 8 // can be a digital pin
#define XP 9 // can be a digital pin

#define TS_MINX 188
#define TS_MINY 186
#define TS_MAXX 958
#define TS_MAXY 913

// For better pressure precision, we need to know the resistance
// between X+ and X- Use any multimeter to read it
// For the one we're using, its 700 ohms across the X plate
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 900);

// Assign human-readable names to some common 16-bit color values:
#define BLACK 0x0000
#define BLUE 0x001F
#define RED 0xF800
#define GREEN 0x07E0
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define WHITE 0xFFFF

Adafruit_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);

#define BOXSIZE 40
#define PENRADIUS 1
int oldcolor, currentcolor;

void setup(void) {
Serial.begin(9600);
Serial.println(F("Paint!"));

tft.reset();

uint16_t identifier = tft.readID();
identifier = 0x9341;

Serial.print(F("LCD driver chip: "));
Serial.println(identifier, HEX);

tft.begin(identifier);

tft.fillScreen(BLACK);

tft.setRotation(1); // landscape 320*240 , mode 2 = 240*320

tft.fillRect(0, 0, BOXSIZE, BOXSIZE, RED);
tft.fillRect(BOXSIZE, 0, BOXSIZE, BOXSIZE, YELLOW);
tft.fillRect(BOXSIZE*2, 0, BOXSIZE, BOXSIZE, GREEN);
tft.fillRect(BOXSIZE*3, 0, BOXSIZE, BOXSIZE, CYAN);
tft.fillRect(BOXSIZE*4, 0, BOXSIZE, BOXSIZE, BLUE);
tft.fillRect(BOXSIZE*5, 0, BOXSIZE, BOXSIZE, MAGENTA);
tft.fillRect(BOXSIZE*6, 0, BOXSIZE, BOXSIZE, WHITE);

tft.drawRect(0, 0, BOXSIZE, BOXSIZE, WHITE);
currentcolor = RED;

pinMode(13, OUTPUT);
}

#define MINPRESSURE 100
#define MAXPRESSURE 1000

void loop()
{
digitalWrite(13, HIGH);
// Recently Point was renamed TSPoint in the TouchScreen library
// If you are using an older version of the library, use the
// commented definition instead.
// Point p = ts.getPoint();
TSPoint p = ts.getPoint();

digitalWrite(13, LOW);

// if sharing pins, you'll need to fix the directions of the touchscreen pins

pinMode(XM, OUTPUT);
pinMode(YP, OUTPUT);

// we have some minimum pressure we consider 'valid'
// pressure of 0 means no pressing!

if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {

// To calibrate the display by changing the MINX/Y and MAXY/X
Serial.print("X = "); Serial.print(p.x);
Serial.print("\tY = "); Serial.print(p.y);
Serial.print("\tZ = "); Serial.print(p.z);
Serial.print("\n");

if (p.y < (TS_MINY+5)) {
Serial.println("erase");
// press the bottom of the screen to erase
tft.fillRect(0, BOXSIZE, tft.width(), tft.height()-BOXSIZE, BLACK);
}
// scale from 0->1023 to tft.width
p.x = tft.width()-(map(p.x, TS_MINX, TS_MAXX, tft.width(), 0));
p.y = tft.height()-(map(p.y, TS_MINY, TS_MAXY, tft.height(), 0));

if (p.y < BOXSIZE) {
oldcolor = currentcolor;

if (p.x < BOXSIZE) {
currentcolor = RED;
tft.drawRect(0, 0, BOXSIZE, BOXSIZE, WHITE);
} else if (p.x < BOXSIZE*2) {
currentcolor = YELLOW;
tft.drawRect(BOXSIZE, 0, BOXSIZE, BOXSIZE, WHITE);
} else if (p.x < BOXSIZE*3) {
currentcolor = GREEN;
tft.drawRect(BOXSIZE*2, 0, BOXSIZE, BOXSIZE, WHITE);
} else if (p.x < BOXSIZE*4) {
currentcolor = CYAN;
tft.drawRect(BOXSIZE*3, 0, BOXSIZE, BOXSIZE, WHITE);
} else if (p.x < BOXSIZE*5) {
currentcolor = BLUE;
tft.drawRect(BOXSIZE*4, 0, BOXSIZE, BOXSIZE, WHITE);
} else if (p.x < BOXSIZE*6) {
currentcolor = MAGENTA;
tft.drawRect(BOXSIZE*5, 0, BOXSIZE, BOXSIZE, WHITE);
} else if (p.x < BOXSIZE*7) {
currentcolor = WHITE;
tft.drawRect(BOXSIZE*6, 0, BOXSIZE, BOXSIZE, BLUE);
}

if (oldcolor != currentcolor) {
if (oldcolor == RED) tft.fillRect(0, 0, BOXSIZE, BOXSIZE, RED);
if (oldcolor == YELLOW) tft.fillRect(BOXSIZE, 0, BOXSIZE, BOXSIZE, YELLOW);
if (oldcolor == GREEN) tft.fillRect(BOXSIZE*2, 0, BOXSIZE, BOXSIZE, GREEN);
if (oldcolor == CYAN) tft.fillRect(BOXSIZE*3, 0, BOXSIZE, BOXSIZE, CYAN);
if (oldcolor == BLUE) tft.fillRect(BOXSIZE*4, 0, BOXSIZE, BOXSIZE, BLUE);
if (oldcolor == MAGENTA) tft.fillRect(BOXSIZE*5, 0, BOXSIZE, BOXSIZE, MAGENTA);
if (oldcolor == WHITE) tft.fillRect(BOXSIZE*6, 0, BOXSIZE, BOXSIZE, WHITE);
}
}
if (((p.y-PENRADIUS) > BOXSIZE) && ((p.y+PENRADIUS) < tft.height())) {
  int radius = p.z-MINPRESSURE;
  radius = MAXPRESSURE-MINPRESSURE - radius;
  radius /= 10;
tft.fillCircle(p.x, p.y, (MAXPRESSURE-p.z)/25, currentcolor);
}
}
}
