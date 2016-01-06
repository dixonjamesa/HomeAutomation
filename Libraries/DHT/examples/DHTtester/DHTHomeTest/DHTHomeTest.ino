#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>
#include <DHT.h>

// The DHT data line is connected to pin 2 on the Arduino
#define DHTPIN 2

// Leave as is if you're using the DHT22. Change if not.
//#define DHTTYPE DHT11   // DHT 11 
#define DHTTYPE DHT22   // DHT 22  (AM2302)
//#define DHTTYPE DHT21   // DHT 21 (AM2301)

DHT dht(DHTPIN, DHTTYPE);

// Radio with CE & CSN connected to 7 & 8
RF24 radio(7, 8);
RF24Network network(radio);

// Constants that identify this node and the node to send data to
const uint16_t this_node = 1;
const uint16_t parent_node = 0;

// Time between packets (in ms)
const unsigned long interval = 2000;

// Structure of our message
struct message_t {
  float temperature;
  float humidity;
};
message_t message;

// The network header initialized for this node
RF24NetworkHeader header(parent_node);

void setup(void)
{
  // Initialize all radio related modules
  SPI.begin();
  radio.begin();
  delay(5);
  network.begin(90, this_node);

  // Initialize the DHT library
  dht.begin();

  // Set up the Serial Monitor
  Serial.begin(9600);
}

void loop() {

  // Update network data
  network.update();

  // Read humidity (percent)
  float h = dht.readHumidity();
  // Read temperature as Celsius
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit
  float f = dht.readTemperature(true);

  // Construct the message we'll send (replace t with f if you're sending Fahrenheit)
  message = (message_t){ t, h };
  
  // Headers will always be Temperature for this node
  // We set it again each loop iteration because fragmentation of the messages might change this between loops
  header.type = 't';

  // Writing the message to the network means sending it
  if (network.write(header, &message, sizeof(message))) {
    Serial.print("Message sent\n"); 
  } else {
    Serial.print("Could not send message\n"); 
  }

  // Wait a bit before we start over again
  delay(interval);
}

