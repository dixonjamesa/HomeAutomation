/*
 * HAS_Comms.cpp
 * 
 * (C) 2019 James Dixon
 * 
 * MQTT Pub/Sub functionality
 * Handles all the MQTT comms to the network
 * 
 */
#include "HAS_Comms.h"
#include "HAS_option.h"
#include "HAS_Main.h"

char messageBuffer[512];

void subscribeTo(const String &_topic)
{
  PSclient.subscribe(_topic.c_str());
  Serial.print("Sub to : ");
  Serial.println(_topic);
  PSclient.loop(); // do this. or too many items causes disconnect some time later. Eugh
}

/*
 * MQTT interface
 */
void setupMQTTMessages()
{
  subscribeTo( String(options.MqttTopic() + "/#"));
  subscribeTo( String(options.MqttGroupTopic() + "/#"));  
}


// callback when an MQTT message is received
// for all topics we've subscribed to
void MQTTcallback(char* topic, byte* payload, unsigned int length)
{                                                                   
  int i = 0;
  
  char *messtype = topic;
  String contents;

  // find first '/'
  while( *messtype != 0 && *messtype != '/' ) {messtype++;}
  if( *messtype == 0 ) return;
  messtype++;
  

  // uppercase so it's all case insensitive
  for (i = 0; i < strlen(messtype); i++) 
  {
      messtype[i] = toupper(messtype[i]);
  }
  for (i = 0; i < length; i++) 
  {
      payload[i] = toupper(payload[i]);
  }

  // only interested in commands, so
  // see if it's a "cmnd" message:
  if( strncmp (messtype, "CMND", 4 ) ) return;
  // advance to the good bit
  messtype += 5;
  
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  Serial.println();

  if( !strncmp(messtype, "SWITCH", 6))
  {
      //is a switch command
      Serial.print("Switch: ");
      Serial.println(messtype);
      // Don't do anything with this - the switch is ours!!
      // just carry on...
      return;
  }
  else 
  if( !strncmp(messtype, "POWER", 5))
  {
      //is a power command
      Serial.print("Power: ");
      Serial.print(messtype);
      int id = atoi(messtype+5);
      Serial.print(" to output ");
      Serial.println(id);
      contents = "{\"POWER";
      contents += id;
      contents += "\":\"";
      if( !strncmp((char *)payload, "TOGGLE", 6) ) {
        SetOutput(id, false, true);
        contents += "TOGGLE";
      }
      else if( !strncmp((char *)payload, "ON", 2) ) {
        SetOutput(id, true);
        contents += "ON";
      }
      else if( !strncmp((char *)payload, "OFF", 3) ) {
        SetOutput(id, false);
        contents += "OFF"; 
      }
      contents += "\"}";
  }
  else if( !strncmp(messtype, "ROTARY", 6))
  {
    // rotary command
  }
  else
  { // see if it matches a config command...
    Serial.print("Command: '");
    Serial.print(messtype);
    Serial.print("'...");
    // publish state message
    if(options.IsOption(messtype))
    {
      char value[256];
      Serial.println("Recognised");
      strncpy(value, (char *)payload, length);
      value[length] = 0;
      contents = "{\"";
      contents += messtype;
      contents += "\", \"";
      contents += value;
      contents += "\"}";
    }
    else
    {
      Serial.print("NOT recognised (length ");
      Serial.print(length);
      Serial.println(")");
      contents = "{\"Command\":\"Unknown\"}";
    }
  }
  
  String message = options.MqttTopic() + "/" + options.Prefix2() + "/RESULT";
  PSclient.publish(message.c_str(), contents.c_str(), false);
}
