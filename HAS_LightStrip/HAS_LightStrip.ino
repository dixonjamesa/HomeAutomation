#define DATAPIN 2
#define LED_COUNT 8

Adafruit_NeoPixel leds = Adafruit_NeoPixel(LED_COUNT, DATAPIN, NEO_GRB + NEO_KHZ800 );

void setup() {
  // put your setup code here, to run once:
  leds.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
  leds.setPixelColor(1,0xFF, 0 , 0xFF);
  leds.show();
}
