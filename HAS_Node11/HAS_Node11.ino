#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>

// Output PINs
byte outputPin = 5; // used to control the power line
byte onlinePin = 6; // used to show if we're active

byte onlineFlash = HIGH;

// PIR variables
byte pirPin = 9;
int pirCalibrationTime = 30;

// Temperature PIN
byte tempPin = A2;

// Photocell variable
byte photocellPin = A3;

// Radio with CE & CSN connected to pins 7 & 8
RF24 radio(7, 8);
RF24Network network(radio);

// Constants that identify this node and the node to send data to
const uint16_t this_node = 011;
const uint16_t control_node = 0;

// Time between packets (in ms)
const unsigned long interval = 1000;  // every sec

// Structure of our message
struct message_sensordata {
  float temperature;
  float humidity;
  byte light;
  bool motion;
  bool dooropen;
};
message_sensordata message;

// Structure of our message
struct message_action {
  byte switchid;
  bool state;
};

// The network header initialized for this node
RF24NetworkHeader writeHeader(control_node);

void setup(void)
{
  // Set up the Serial Monitor
  Serial.begin(9600);

  // Initialize all radio related modules
  SPI.begin();
  radio.begin();
  delay(5);
  network.begin(90, this_node);

  pinMode(outputPin, OUTPUT);
  pinMode(onlinePin, OUTPUT);

  // Calibrate PIR
  pinMode(pirPin, INPUT);
  digitalWrite(pirPin, LOW);
/*  Serial.print("Calibrating PIR ");
  for(int i = 0; i < pirCalibrationTime; i++)
  {
    Serial.print(".");
    delay(1000);
  }
  Serial.println(" done");
  Serial.println("PIR ACTIVE");
  */
  delay(50);
}

void loop() {

  // Update network data
  network.update();

/// RECEIVING MESSAGES
  while (network.available()) 
  {
    RF24NetworkHeader header;
    message_action message;
    network.peek(header);
    if (header.type == '2') 
    {
      network.read(header, &message, sizeof(message));
      Serial.print("Data received from node ");
      Serial.println(header.from_node);
      // Check value and turn the LED on or off
      Serial.print("Id: ");
      Serial.println(message.switchid);
      Serial.print("Value: ");
      Serial.println(message.state);
      byte targetPin = outputPin;
      if(message.switchid != 0)
      {
        Serial.print("Unexpected output action id received");
      }
      if (message.state) 
      {
        digitalWrite(targetPin, HIGH);
      } 
      else 
      {
        digitalWrite(targetPin, LOW);
      }
    } 
    else 
    {
      // This is not a type we recognize
      network.read(header, &message, sizeof(message));
      Serial.print("Unknown message received from node ");
      Serial.println(header.from_node);
    }
  }


/// SENDING MESSAGES

  // Read temperature
  float t = (5.0 * analogRead(tempPin) * 100.0) / 1024;
  Serial.print("Temperature is: ");
  Serial.println(t);

  // Read photocell
  int p = analogRead(photocellPin);
  // Testing revealed this value never goes below 50 or above 1000,
  //  so we're constraining it to that range and then mapping that range
  //  to 0-100 so it's like a percentage
  p = constrain(p, 50, 1000);
  p = map(p, 50, 1000, 0, 100);
    
  // Read motion: HIGH means motion is detected
  bool m = (digitalRead(pirPin) == HIGH);
  
  // Headers will always be type 1 for this node
  // We set it again each loop iteration because fragmentation of the messages might change this between loops
  writeHeader.type = '1';

  // Only send values if any of them are different enough from the last time we sent:
  //  0.5 degree temp difference, 1% humdity or light difference, or different motion state
  if (abs(t - message.temperature) > 0.5 || 
      /*abs(h - message.humidity) > 1.0 || */
      abs(p - message.light) > 1.0 || 
      m != message.motion /*||
      d != message.dooropen*/) 
  {
    // Construct the message we'll send
    message = (message_sensordata){ t, 0, p, m, 0 };

    //Send the message
    if (network.write(writeHeader, &message, sizeof(message))) 
    {
      Serial.print("Message sent\n"); 
    } 
    else 
    {
      Serial.print("Could not send message\n"); 
    }
  }

  // Wait a bit before we start over again
  delay(interval);

  if( onlineFlash == HIGH ) onlineFlash = LOW;
  else onlineFlash = HIGH;
  if( onlineFlash == HIGH )
  {
    digitalWrite(onlinePin, HIGH);
    delay(5\0);
    digitalWrite(onlinePin, LOW);
  }
}
