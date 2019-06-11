/*
 * HAS_option.cpp
 * 
 * (C) 2019 James Dixon
 * 
 * Persistent option handling
 * 
 */

#include "HAS_option.h"
#include "FS.h"
#include "config.h"
#include <ArduinoJson.h>
#include <list>

class Opt;

// only ever add options to this list...
std::list<Opt *> allOptions;
#define MAXKEYLEN 32
#define MAXVALUELEN 64
// a single option
class Opt
{
  public:
  Opt( const char *_key, const char *_value=NULL )
  {
    if( strlen(_key) < MAXKEYLEN )
    {
    strcpy(key, _key);
    if( _value != NULL ) SetValue(_value); else *value = 0;
    allOptions.push_back(this);
    }
    else
    {
      Serial.print("ERROR in Options: key too long: ");
      Serial.print(_key);
    }
  }
  const char *Key() { return key; }
  const char *Value() { return value; }
  void SetValue( const char *_val )
  {
    if(strlen(_val) < MAXVALUELEN )
    {
      strcpy(value, _val);
    }
    else
    {
      Serial.print("ERROR in Options: value too long: ");
      Serial.print(key);
      Serial.print(" attempt to set to ");
      Serial.println(_val);
    }
  }
  private:
  char key[MAXKEYLEN];
  char value[MAXVALUELEN];
};

Opt *FindOption(const char *_key)
{
  for(std::list<Opt *>::const_iterator iterator = allOptions.begin() ; iterator != allOptions.end() ; ++iterator )
  {
    //if((*iterator)->Key().equalsIgnoreCase(String(_key))) return (*iterator);
    if(!strcmp((*iterator)->Key(), _key)) return (*iterator);
  }
  return NULL;
}

char sbuffer[4096];

void Option::Begin()
{
  Serial.println("Sp begin");
  SPIFFS.begin();

  if (!SPIFFS.exists("/formatComplete.txt")) 
  {
    Serial.println("Please wait 30 secs for SPIFFS to be formatted");
    SPIFFS.format();
    Serial.println("Spiffs formatted");
   
    File f = SPIFFS.open("/formatComplete.txt", "w");
    if (!f) 
    {
        Serial.println("file open failed");
    } else 
    {
        f.println("Format Complete");
        f.close();
    }
  } 
  else 
  {
    Serial.println("SPIFFS is formatted. Moving along...");
  }

  // See if there's an options file:
  if( SPIFFS.exists("/Options.opt") )
  {
    File f = SPIFFS.open("/Options.opt", "r");
    int sz = f.size();
    int i;
    Serial.print("Loading options size: ");
    Serial.println(sz);
    for(i=0;i<sz;i++)
    {
      sbuffer[i] = f.read();
    }
    sbuffer[i] = 0;
    Serial.print("File contents:");
    Serial.println(sbuffer);
    f.close();
    Serial.println("Loaded options: ");
    // Let ArduinoJson read directly from File
    DynamicJsonBuffer jb;
    JsonObject& root = jb.parseObject(sbuffer);
    root.printTo(Serial);
    Opt *o;
    i = 0;
    for (JsonObject::iterator it=root.begin(); it!=root.end(); ++it) 
    {
      o = new Opt(it->key, it->value.as<char*>());
      i++;
    } 
    Serial.print("Loaded ");
    Serial.print(i);
    Serial.println(" options");
  }
  else
  {
    Serial.println("No Options.opt file found in SPIFFS");
  }
}

/*
 * Initialiser
 * see config.h
 */
Option::Option()
{
  outDefaults[0] = OUT1_PIN;
  outDefaults[1] = OUT2_PIN;
  outDefaults[2] = OUT3_PIN;
  outDefaults[3] = OUT4_PIN;
  outDefaults[4] = OUT5_PIN;
  outDefaults[5] = OUT6_PIN;
  outDefaults[6] = OUT7_PIN;
  outDefaults[7] = OUT8_PIN;
  outTypes[0] = OUT1_TYPE;
  outTypes[1] = OUT2_TYPE;
  outTypes[2] = OUT3_TYPE;
  outTypes[3] = OUT4_TYPE;
  outTypes[4] = OUT5_TYPE;
  outTypes[5] = OUT6_TYPE;
  outTypes[6] = OUT7_TYPE;
  outTypes[7] = OUT8_TYPE;
  ledDefaults[0] = OUT1_LED;
  ledDefaults[1] = OUT2_LED;
  ledDefaults[2] = OUT3_LED;
  ledDefaults[3] = OUT4_LED;
  ledDefaults[4] = OUT5_LED;
  ledDefaults[5] = OUT6_LED;
  ledDefaults[6] = OUT7_LED;
  ledDefaults[7] = OUT8_LED;
  LEDTopicDefaults[0] = LED1_TOPIC;
  LEDTopicDefaults[1] = LED2_TOPIC;
  LEDTopicDefaults[2] = LED3_TOPIC;
  LEDTopicDefaults[3] = LED4_TOPIC;
  LEDTopicDefaults[4] = LED5_TOPIC;
  LEDTopicDefaults[5] = LED6_TOPIC;
  LEDTopicDefaults[6] = LED7_TOPIC;
  LEDTopicDefaults[7] = LED8_TOPIC;
  
  swPin1Defaults[0] =  SW1_PIN1;
  swPin2Defaults[0] =  SW1_PIN2;
  swTypeDefaults[0] =  SW1_TYPE;
  swTopicDefaults[0] = SW1_TOPIC;
  swPin1Defaults[1] =  SW2_PIN1;
  swPin2Defaults[1] =  SW2_PIN2;
  swTypeDefaults[1] =  SW2_TYPE;
  swTopicDefaults[1] = SW2_TOPIC;
  swPin1Defaults[2] =  SW3_PIN1;
  swPin2Defaults[2] =  SW3_PIN2;
  swTypeDefaults[2] =  SW3_TYPE;
  swTopicDefaults[2] = SW3_TOPIC;
  swPin1Defaults[3] =  SW4_PIN1;
  swPin2Defaults[3] =  SW4_PIN2;
  swTypeDefaults[3] =  SW4_TYPE;
  swTopicDefaults[3] = SW4_TOPIC;
  swPin1Defaults[4] =  SW5_PIN1;
  swPin2Defaults[4] =  SW5_PIN2;
  swTypeDefaults[4] =  SW5_TYPE;
  swTopicDefaults[4] = SW5_TOPIC;
  swPin1Defaults[5] =  SW6_PIN1;
  swPin2Defaults[5] =  SW6_PIN2;
  swTypeDefaults[5] =  SW6_TYPE;
  swTopicDefaults[5] = SW6_TOPIC;
  swPin1Defaults[6] =  SW7_PIN1;
  swPin2Defaults[6] =  SW7_PIN2;
  swTypeDefaults[6] =  SW7_TYPE;
  swTopicDefaults[6] = SW7_TOPIC;
  swPin1Defaults[7] =  SW8_PIN1;
  swPin2Defaults[7] =  SW8_PIN2;
  swTypeDefaults[7] =  SW8_TYPE;
  swTopicDefaults[7] = SW8_TOPIC;
  
  rotUDefaults[0] = ROT1_PINU;
  rotDDefaults[0] = ROT1_PIND;
  rotTopicDefaults[0] = ROT1_TOPIC;
  rotUDefaults[1] = ROT2_PINU;
  rotDDefaults[1] = ROT2_PIND;
  rotTopicDefaults[1] = ROT2_TOPIC;
  rotUDefaults[2] = ROT3_PINU;
  rotDDefaults[2] = ROT3_PIND;
  rotTopicDefaults[2] = ROT3_TOPIC;
  rotUDefaults[3] = ROT4_PINU;
  rotDDefaults[3] = ROT4_PIND;
  rotTopicDefaults[3] = ROT4_TOPIC;

  friendlyDefaults[0] = FRIENDLY1;
  friendlyDefaults[1] = FRIENDLY2;
  friendlyDefaults[2] = FRIENDLY3;
  friendlyDefaults[3] = FRIENDLY4;
  friendlyDefaults[4] = FRIENDLY5;
  friendlyDefaults[5] = FRIENDLY6;
  friendlyDefaults[6] = FRIENDLY7;
  friendlyDefaults[7] = FRIENDLY8;

}

// Save all options to SPIFFS
void Option::Save()
{
  Serial.print("Saving ");
  Serial.print(allOptions.size());
  Serial.println(" options");

  File f = SPIFFS.open("/Options.opt", "w");
  if( !f)
  {
    Serial.println("Failed to open options.opt for writing");  
  }
  else
  {
    bool sep = false;
    //Serial.println("File opened");
    f.print("{");
    for(std::list<Opt *>::const_iterator iterator = allOptions.begin() ; iterator != allOptions.end() ; ++iterator )
    {
      if(sep) f.print(",");
      if(!sep) sep = true;
      f.print("\"");
      f.print((*iterator)->Key());
      f.print("\":\"");
      f.print((*iterator)->Value());
      f.print("\"");
    }
    f.print("}");
    f.close();    
  }
  //Serial.println("Done.");
}

bool Option::IsOption(const char *_opt)
{
  return (FindOption(_opt) != NULL);
}

const char *Option::GetOption(const char *_opt, int _default)
{
  char tbuf[16];
  sprintf(tbuf, "%d", _default);
  return GetOption(_opt, tbuf);
}

const char *Option::GetOption(const char *_opt, const char *_default)
{
  Opt * o = FindOption(_opt);
  if( o == NULL )
  {
    //Serial.print("Not found option ");
    //Serial.println(_opt);
    o = new Opt(_opt, _default);
  }
  //Serial.print("Returning value ");
  //Serial.println(o->Value());
  return o->Value();
}

bool Option::SetOption(const char *_opt, const char *_value)
{
  Opt *o;

  o = FindOption(_opt);
  if(o == NULL ) o = new Opt(_opt);
  o->SetValue(_value);
  Save();
}
