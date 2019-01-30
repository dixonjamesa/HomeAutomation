#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define SERIALOUT 1


// details for our AP
const char *AP_ssid = "garage101";
const char *AP_password = "password";

// details for the main Wifi (entered by user)
const char *WIFI_ssid = "PrettyFly24";
const char *WIFI_pass = "Dixonia1";
String WIFI_status="Not connected";
bool WIFI_details_ready = true;
bool WIFI_connected = false;
const char* mqtt_server = "192.168.0.99";

int stateLED = HIGH; // high is off (obviously!)

WiFiClient espClient;
PubSubClient client(espClient);

String topics_sub[] = {
   "garage/switch",
   // "kitchen/lights/l2",
   // "kitchen/lights/l3",
    };


void tryConnectMQTT() 
{
  if( WIFI_connected )
  {
    if(!client.connected())
    {
      Serial.print("Attempting MQTT connection...");
  #if true
      // Attempt to connect
      if (client.connect("ESP8266Client")) 
      {
        Serial.println("connected");
        // ... and subscribe to all topics
        //for(int i=0; i < sizeof(topics_sub)/sizeof(topics_sub[0]) ; i++)
        {
          //client.subscribe(topics_sub[i].c_str());
          //Serial.print("Sub to : ");
          //Serial.println(topics_sub[i]);
          //client.loop(); // do this. or >5 subs causes disconnect some time later. Eugh
        }
        //Serial.println(client.connected());
      } 
      else 
      {
        Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.println(" try again in 5 seconds");
      }
      #else
      // Attempt to connect
      if (client.connect("ESP8266Client")) 
      {
        Serial.println("connected");
        // Once connected, publish an announcement...
        client.publish("garage/status", "hello world");
        // ... and resubscribe
        client.subscribe("garage/switch");
      } else 
      {
        Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.println(" try again in 5 seconds");
        // Wait 5 seconds before retrying
        //delay(5000);
      }
   
      #endif
    }
  }
}

void MQTTcallback(char* topic, byte* payload, unsigned int length)
{
  int i = 0;
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (i=0 ; i<sizeof(topics_sub)/sizeof(topics_sub[0]) ; i++) 
  {
    if( !strcmp(topic, topics_sub[i].c_str() ) )
    {
      payload[length]=0;
      Serial.print("Setting topic ");
      Serial.print(i);
      Serial.print(" to ");
      Serial.print((char *)payload);
    }
      if ((char)payload[1] == 'N') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    Serial.print("LED ON");
    // but actually the LED is on; this is because
    // it is acive low on the ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
    Serial.print("LED OFF");
  }

  }
  Serial.println();
}



void tryConnectWIFI()
{
  {
    Serial.print("Connecting WIFI ");
    Serial.print(WIFI_ssid);
    WiFi.begin(WIFI_ssid, WIFI_pass);
    
    int tries = 0;

    while( WiFi.status() != WL_CONNECTED )
    {
      stateLED = 1 - stateLED;
      digitalWrite(LED_BUILTIN, stateLED);
      delay(500);
      Serial.print(".");
      if( ++tries > 20 )
      {
        break; // give up then
      }
    }
    Serial.println("Updating status...");
    switch(WiFi.status())
    {
      case WL_CONNECTED:
        WIFI_status = "Connected";
        WIFI_connected = true;
        client.setServer(mqtt_server, 1883);
        client.setCallback(MQTTcallback);
        stateLED = LOW;
        digitalWrite(LED_BUILTIN, stateLED);
        break;
      case WL_CONNECT_FAILED:
        WIFI_status = "Failed to connect";
        break;
      case WL_CONNECTION_LOST:
      case WL_DISCONNECTED:
        WIFI_status = "Disconnected";
        break;       
      default:
        WIFI_status = "Unknown problem";
        break;
    }
  }
}

void setup(void)
{
  delay(1000);
#if SERIALOUT
  // Set up the Serial Monitor
  Serial.begin(115200);
  Serial.println("Start");
#endif
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output

    tryConnectWIFI();
}

void loop() 
{
  if( client.connected() )
  {
    if( client.loop() == false )
    {
      Serial.print(client.state());    
    }
  }
  else
  {
    tryConnectMQTT();
    //Serial.print(client.state());      
  }
}

