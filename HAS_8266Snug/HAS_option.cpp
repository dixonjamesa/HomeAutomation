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

// a single option
class Opt
{
  public:
  Opt( const char *_key, const char *_value=NULL )
  {
    key = _key;
    if( _value != NULL ) SetValue(_value); else value = "";
    allOptions.push_back(this);
  }
  String &Key() { return key; }
  String &Value() { return value; }
  void SetValue( const char *_val )
  {
    value = _val;
  }
  private:
  String key;
  String value;
};

Opt *FindOption(const char *_key)
{
  for(std::list<Opt *>::const_iterator iterator = allOptions.begin() ; iterator != allOptions.end() ; ++iterator )
  {
    if((*iterator)->Key().equalsIgnoreCase(String(_key))) return (*iterator);
  }
  return NULL;
}

void Option::Begin()
{
  Serial.println("Sp begin");
  SPIFFS.begin();

  if( SPIFFS.exists("Options.opt") )
  {
    Serial.print("Loading options size: ");
    File f = SPIFFS.open("Options.opt", "r");
    Serial.println(f.size());
    // Let ArduinoJson read directly from File
    DynamicJsonBuffer jb;
    JsonObject& root = jb.parseObject(f);
    f.close();
    Serial.println("Loaded options: ");

    root.printTo(Serial);
    Opt *o;
    int i = 0;
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
  ledDefaults[0] = OUT1_LED;
  ledDefaults[1] = OUT2_LED;
  ledDefaults[2] = OUT3_LED;
  ledDefaults[3] = OUT4_LED;
  ledDefaults[4] = OUT5_LED;
  ledDefaults[5] = OUT6_LED;
  ledDefaults[6] = OUT7_LED;
  ledDefaults[7] = OUT8_LED;
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
  rotUDefaults[1] = ROT2_PINU;
  rotDDefaults[1] = ROT2_PIND;
  rotUDefaults[2] = ROT3_PINU;
  rotDDefaults[2] = ROT3_PIND;
  rotUDefaults[3] = ROT4_PINU;
  rotDDefaults[3] = ROT4_PIND;
  friendlyDefaults[0] = FRIENDLY1;
  friendlyDefaults[1] = FRIENDLY2;
  friendlyDefaults[2] = FRIENDLY3;
  friendlyDefaults[3] = FRIENDLY4;
  friendlyDefaults[4] = FRIENDLY5;
  friendlyDefaults[5] = FRIENDLY6;
  friendlyDefaults[6] = FRIENDLY7;
  friendlyDefaults[7] = FRIENDLY8;

}

void Option::Save()
{
  StaticJsonBuffer<5000> jsonBuffer;
  JsonObject &JsonRoot = jsonBuffer.createObject();
  int i=0;
  for(std::list<Opt *>::const_iterator iterator = allOptions.begin() ; iterator != allOptions.end() ; ++iterator )
  {
    JsonRoot.set((*iterator)->Key(), (*iterator)->Value());
    i++;
  }
  //JsonRoot.printTo(Serial);
  Serial.print("Saving ");
  Serial.print(i);
  Serial.println(" options");
  File f = SPIFFS.open("Options.opt", "w");
  JsonRoot.printTo(f);
  f.close();    
  Serial.println("Done.");
}

bool Option::IsOption(const char *_opt)
{
  return (FindOption(_opt) != NULL);
}

#if 1
String &Option::GetOption(const char *_opt, const char *_default)
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

#endif

#if 0
String Option::GetOption(const char *_opt, const char *_default)
{
  String result = _default;
  if( SPIFFS.exists(_opt) )
  {
    File f = SPIFFS.open(_opt, "r");
    result = f.readString();
    f.close();
    if( result.length() == 0) result = _default;
  }
  return result;
}

bool Option::SetOption(const char *_opt, const char *_value)
{
  File f = SPIFFS.open(_opt, "w");
  f.print(_value);
  f.close();  
}
#endif
