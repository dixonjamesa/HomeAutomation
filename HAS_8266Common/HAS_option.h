/*
 * HAS_option.h
 * 
 * (C) 2019 James Dixon
 * 
 * Persistent option handling
 * 
 */
#ifndef _OPTION_H_
#define _OPTION_H_

#include <Arduino.h>
#include "config.h"
#include "HAS_Main.h"

class Option; 
// global options class instance:
extern Option options;

class Option
{
  public:
    Option();
    void Begin();
    void Save();

    bool IsOption(const char *_opt);
    String &GetOption(const char *_opt, const char *_default);
    bool SetOption(const char *_opt, const char *_value);

    const String UID() {return String(ESP.getChipId()); }
    // the options:
    //[Unit]
    const String &Unit() { return GetOption("Unit", UNIT);}
    void Unit(const char *_val) {SetOption("Unit", _val);} 
    //[FriendlyName<n>]
    const char *FriendlyNameParsed(int _id) { static char tbuf[32];sprintf(tbuf, FriendlyName(_id).c_str(), Unit().c_str(), UID().c_str()); return tbuf;}
    const String &FriendlyName(int _id) { return GetOption((String("FriendlyName")+_id).c_str(), friendlyDefaults[_id-1]);}
    void FriendlyName(int _id, const char *_val) {SetOption((String("FriendlyName")+_id).c_str(), _val);} 
    
    // [Ssid]
    const String &WF_ssid() { return GetOption("Ssid", STA_SSID);}
    void WF_ssid(const char *_val) {SetOption("Ssid", _val);}
    // [Password]
    const String &WF_pass() { return GetOption("Password", STA_PASS);}
    void WF_pass(const char *_val) {SetOption("Password", _val);}
    // [WebName]
    const char *WebNameParsed() { static char tbuf[32];sprintf(tbuf, WebName().c_str(), Unit().c_str(), UID().c_str()); return (tbuf);}
    const String &WebName() { return GetOption("WebName", WEBNAME);}
    void WebName(const char *_val) {SetOption("WebName", _val);}
    // [WebPassword]
    const String &AP_pass() { return GetOption("WebPassword", AP_PASS);}
    void AP_pass(const char *_val) {SetOption("WebPassword", _val);}
    // [WebServer]
    bool APServer() { return (GetOption("WebServer", AP_SERVER==1?"1":"0")=="1");}
    void APServer(bool _val) {SetOption("WebServer", _val?"1":"0");}
    // [MqttHost]
    const String &MqttHost() { return GetOption("MqttHost", MQTT_HOST);}
    void MqttHost(const char *_val) {SetOption("MqttHost", _val);}
    // [MqttPort]
    const int MqttPort() { return GetOption("MqttPort", String(MQTT_PORT).c_str()).toInt();}
    void MqttPort(int _val) {SetOption("MqttPort", String(_val).c_str());}
    // [MqttUser]
    const String &MqttUser() { return GetOption("MqttUser", MQTT_USER);}
    void MqttUser(const char *_val) {SetOption("MqttUser", _val);}
    // [MqttPass]
    const String &MqttPass() { return GetOption("MqttPass", MQTT_PASS);}
    void MqttPass(const char *_val) {SetOption("MqttPass", _val);}
    // [MqttClient]
    const char *MqttClientParsed() { static char tbuf[32];sprintf(tbuf, MqttClient().c_str(), Unit().c_str(), UID().c_str()); return tbuf;}
    const String &MqttClient() { return GetOption("MqttClient", MQTT_CLIENT);}
    void MqttClient(const char *_val) {SetOption("MqttClient", _val);}
    // [MqttTopic]
    const String &MqttTopic() { return GetOption("MqttTopic", MQTT_TOPIC);}
    void MqttTopic(const char *_val) {SetOption("MqttTopic", _val);}
    // [MqttGroupTopic]
    const String &MqttGroupTopic() { return GetOption("MqttGroupTopic", MQTT_GRPTOPIC);}
    void MqttGroupTopic(const char *_val) {SetOption("MqttGroupTopic", _val);}

    // [Prefix1]
    const String &Prefix1() { return GetOption("Prefix1", SUB_PREFIX);}
    void Prefix1(const char *_val) {SetOption("Prefix1", _val);}
    // [Prefix2]
    const String &Prefix2() { return GetOption("Prefix2", PUB_PREFIX);}
    void Prefix2(const char *_val) {SetOption("Prefix2", _val);}
    // [Prefix3]
    const String &Prefix3() { return GetOption("Prefix3", INFO_PREFIX);}
    void Prefix3(const char *_val) {SetOption("Prefix3", _val);}

  // Switches...
    // [Sw<n>Pin1]
    const int SwPin1(int _id) { return GetOption((String("Sw")+_id+"Pin1").c_str(), String(swPin1Defaults[_id-1]).c_str()).toInt();}
    void SwPin1(int _id, int _val) {SetOption((String("Sw")+_id+"Pin1").c_str(), String(_val).c_str());}
    // [Sw<n>Pin2]
    const int SwPin2(int _id) { return GetOption((String("Sw")+_id+"Pin2").c_str(), String(swPin2Defaults[_id-1]).c_str()).toInt();}
    void SwPin2(int _id, int _val) {SetOption((String("Sw")+_id+"Pin2").c_str(), String(_val).c_str());}
    // [Sw<n>Type]
    const int SwType(int _id) { return GetOption((String("Sw")+_id+"Type").c_str(), String(swTypeDefaults[_id-1]).c_str()).toInt();}
    void SwType(int _id, int _val) {SetOption((String("Sw")+_id+"Type").c_str(), String(_val).c_str());}
    // [Sw<n>Type]
    const String &SwTopic(int _id) { return GetOption((String("Sw")+_id+"Topic").c_str(), swTopicDefaults[_id-1]);}
    void SwTopic(int _id, const char *_val) {SetOption((String("Sw")+_id+"Topic").c_str(), _val);}

  // Rotaries
    // [Rot<n>PinU]
    const int RotPinU(int _id) { return GetOption((String("Rot")+_id+"PinU").c_str(), String(rotUDefaults[_id-1]).c_str()).toInt();}
    void RotPinU(int _id, int _val) {SetOption((String("Rot")+_id+"PinU").c_str(), String(_val).c_str());}
    // [Rot<n>PinD]
    const int RotPinD(int _id) { return GetOption((String("Rot")+_id+"PinD").c_str(), String(rotDDefaults[_id-1]).c_str()).toInt();}
    void RotPinD(int _id, int _val) {SetOption((String("Rot")+_id+"PinD").c_str(), String(_val).c_str());}

  
  // Outputs
    // [Out<1-8>Pin]
    const int OutPin(int _id) { return GetOption((String("Out")+_id+"Pin").c_str(), String(outDefaults[_id-1]).c_str()).toInt();}
    void OutPin(int _id, int _val) {SetOption((String("Out")+_id+"Pin").c_str(), String(_val).c_str());}
    // [Out[1-8]LED]
    const int OutLED(int _id) { return GetOption((String("Out")+_id+"LED").c_str(), String(ledDefaults[_id-1]).c_str()).toInt();}
    void OutLED(int _id, int _val) {SetOption((String("Out")+_id+"LED").c_str(), String(_val).c_str());}

    // [StatusLED]
    const int StatusLED() { return GetOption("StatusLED", String(STATUS_LED).c_str()).toInt();}
    void StatusLED(int _val) {SetOption("StatusLED", String(_val).c_str());}
    // [InvertLED]
    bool InvertLED() { return (GetOption("InvertLED", INVERT_LED==1?"1":"0")=="1");}
    void InvertLED(bool _val) {SetOption("InvertLED", _val?"1":"0");}

    private:
    int outDefaults[NUM_OUTS];
    int ledDefaults[NUM_OUTS];
    int swPin1Defaults[NUM_SWITCHES];
    int swPin2Defaults[NUM_SWITCHES];
    int swTypeDefaults[NUM_SWITCHES];
    const char *swTopicDefaults[NUM_SWITCHES];
    int rotUDefaults[NUM_SWITCHES];
    int rotDDefaults[NUM_SWITCHES];
    const char *friendlyDefaults[NUM_OUTS];
};


#endif _OPTION_H_
