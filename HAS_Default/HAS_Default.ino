#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>

#define MSG_REGISTER 1
#define MSG_SUBSCRIBE 2
#define MSG_TX_BOOL 3
#define MSG_TX_FLOAT 4

#define SUB_BOOL 1
#define SUB_FLOAT 2

#define SUBS_RETRY_TIMEOUT 1000

byte switchPin = 4;
// LED (switch) PIN
byte led1Pin = 5;
byte led2Pin = 6;

// Radio with CE & CSN connected to pins 7 & 8
RF24 radio(7, 8);
RF24Network network(radio);

// Constants that identify this node and the node to send data to
const uint16_t this_node = 1;
const uint16_t control_node = 0;

// Time between packets (in ms)
const unsigned long interval = 1000;  // every sec

// Structure of our message
struct message_subscribe {
  byte type;
  byte id;
  char channel[64];
};

// Structure of our message
struct message_action {
  byte id;
  bool state;
};

message_action DoorState;

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

  pinMode(led1Pin, OUTPUT);
  pinMode(led2Pin, OUTPUT);
  pinMode(switchPin, INPUT_PULLUP);

  delay(50);

  DoorState.id = switchPin;
  DoorState.state = digitalRead(switchPin);
  ch_subscribe( SUB_BOOL, led1Pin, "node1/switch1");
  ch_subscribe( SUB_BOOL, led2Pin, "node1/switch2");
  ch_register( SUB_BOOL, switchPin, "node1/door");
}

void ch_register(byte t, byte i, const char *c)
{
  Serial.print("Registering channel:");
  writeHeader.type = MSG_REGISTER;
  regsub(t, i, c);
}

void ch_subscribe(byte t, byte i, const char *c)
{
  Serial.print("Subscribing to channel:");
  writeHeader.type = MSG_SUBSCRIBE;
  regsub(t, i, c);
}
void regsub(byte t, byte i, const char *c)
{
  writeHeader.from_node = this_node;
  message_subscribe mess;
  mess.type = t;
  mess.id = i;
  strcpy(mess.channel, c);
  Serial.print((char *)mess.channel); 
  Serial.print("...");
  while(!network.write(writeHeader, &mess, sizeof(mess))) 
  {
    Serial.print("."); 
    delay(SUBS_RETRY_TIMEOUT);
  }
    Serial.print("OK.\n"); 
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
    if (header.type == MSG_TX_BOOL) 
    {
      network.read(header, &message, sizeof(message));
      Serial.print("Data received from node ");
      Serial.print(header.from_node);
      // Check value and turn the LED on or off
      Serial.print(" {Id: ");
      Serial.print(message.id);
      Serial.print(", Value: ");
      Serial.print(message.state);
      Serial.println("}");
      if (message.state) 
      {
        digitalWrite(message.id, HIGH);
      } 
      else 
      {
        digitalWrite(message.id, LOW);
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

  if( DoorState.state != digitalRead(switchPin) )
  {
    DoorState.state = !DoorState.state;
    writeHeader.type = MSG_TX_BOOL;
    if( network.write(writeHeader, &DoorState, sizeof(DoorState)))
    {
      Serial.print("Door updated to ");
      Serial.println(DoorState.state);
    } 
    else
    {
      Serial.println("Door failed update");      
    }
  }


  // Wait a bit before we start over again
  delay(interval);
}
