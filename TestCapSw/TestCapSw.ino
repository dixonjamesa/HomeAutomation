#include <CapacitiveSensor.h>

/*
 * CapitiveSense Library Demo Sketch
 * Paul Badger 2008
 * Uses a high value resistor e.g. 10 megohm between send pin and receive pin
 * Resistor effects sensitivity, experiment with values, 50 kilohm - 50 megohm. Larger resistor values yield larger sensor values.
 * Receive pin is the sensor pin - try different amounts of foil/metal on this pin
 * Best results are obtained if sensor foil and wire is covered with an insulator such as paper or plastic sheet
 */


CapacitiveSensor   cs_4_2 = CapacitiveSensor(4,2);        // 10 megohm resistor between pins 4 & 2, pin 2 is sensor pin, add wire, foil
//CapacitiveSensor   cs_4_5 = CapacitiveSensor(4,5);        // 10 megohm resistor between pins 4 & 6, pin 6 is sensor pin, add wire, foil
//CapacitiveSensor   cs_4_8 = CapacitiveSensor(4,8);        // 10 megohm resistor between pins 4 & 8, pin 8 is sensor pin, add wire, foil

void setup()                    
{

  pinMode(7, OUTPUT);
  pinMode(8, OUTPUT);
   cs_4_2.set_CS_AutocaL_Millis(0xFFFFFFFF);     // turn off autocalibrate on channel 1 - just as an example
   Serial.begin(9600);

}

void loop()                    
{
    long start = millis();
    long total1 =  cs_4_2.capacitiveSensor(30);
  //  long total2 =  cs_4_5.capacitiveSensor(30);
  //  long total3 =  cs_4_8.capacitiveSensor(30);

/*
    Serial.print(millis() - start);        // check on performance in milliseconds
    Serial.print("\t");                    // tab character for debug window spacing

    Serial.print(total1);                  // print sensor output 1
    Serial.print("\t");
  //  Serial.print(total2);                  // print sensor output 2
    Serial.print("\t");
  //  Serial.print(total3);                // print sensor output 3
  Serial.println("-");
  */
  Latch(total1);
    delay(25);                             // arbitrary delay to limit data to serial port 
}

bool lState = false;
bool vOutput = LOW;
long ThresholdHigh = 150;
long ThresholdLow = 50;
void Latch(long value)
{
  if(lState == false && value > ThresholdHigh)
  {
    Toggle();
    lState = true;
  }
  else if( lState == true && value < ThresholdLow )
  {
    lState = false;
  }

  if( value > ThresholdLow)
  {
    digitalWrite(8, HIGH);
  }
  else
  {
    digitalWrite(8, LOW);
  }
}

void Toggle()
{
  if( vOutput == LOW ) vOutput = HIGH; else vOutput = LOW;
  digitalWrite(7, vOutput);
    Serial.print("Toggle ");
    Serial.println(vOutput);
}

