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

#include "config.h"
#include "HAS_Main.h"
#include "HAS_Pixels.h"

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
    const char *GetOption(const char *_opt, const char *_default);
    const char *GetOption(const char *_opt, int _default);
    bool SetOption(const char *_opt, const char *_value);

    uint32 UID() {return ESP.getChipId(); }
    // the options:
    //[Unit]
    const char *Unit() { return GetOption("Unit", UNIT);}
    void Unit(const char *_val) {SetOption("Unit", _val);}
    //[FriendlyName<n>]
    const char *FriendlyNameParsed(int _id) { static char tbuf[32];sprintf(tbuf, FriendlyName(_id), Unit(), UID()); return tbuf;}
    const char *FriendlyName(int _id) {char tb[16]; sprintf(tb, "FriendlyName%d", _id); return GetOption(tb, friendlyDefaults[_id-1]);}
    void FriendlyName(int _id, const char *_val) {char tb[16]; sprintf(tb, "FriendlyName%d", _id); SetOption(tb, _val);}

    // [Ssid]
    const char *WF_ssid() { return "PrettyFly24";}//GetOption("Ssid", STA_SSID);}
    void WF_ssid(const char *_val) {SetOption("Ssid", _val);}
    // [Password]
    const char *WF_pass() { return "Dixonia123456";}//GetOption("Password", STA_PASS);}
    void WF_pass(const char *_val) {SetOption("Password", _val);}
    // [WebName]
    const char *WebNameParsed() { static char tbuf[32];sprintf(tbuf, WebName(), Unit(), UID()); return (tbuf);}
    const char *WebName() { return GetOption("WebName", WEBNAME);}
    void WebName(const char *_val) {SetOption("WebName", _val);}
    // [WebPassword]
    const char *AP_pass() { return GetOption("WebPassword", AP_PASS);}
    void AP_pass(const char *_val) {SetOption("WebPassword", _val);}
    // [WebServer]
    bool APServer() { return (strcmp(GetOption("WebServer", AP_SERVER==1?"1":"0"),"0"));}
    void APServer(bool _val) {SetOption("WebServer", _val?"1":"0");}
    // [MqttHost]
    const char *MqttHost() { return GetOption("MqttHost", MQTT_HOST);}
    void MqttHost(const char *_val) {SetOption("MqttHost", _val);}
    // [MqttPort]
    const int MqttPort() { return atoi(GetOption("MqttPort", MQTT_PORT));}
    void MqttPort(int _val) {SetOption("MqttPort", String(_val).c_str());}
    // [MqttUser]
    const char *MqttUser() { return GetOption("MqttUser", MQTT_USER);}
    void MqttUser(const char *_val) {SetOption("MqttUser", _val);}
    // [MqttPass]
    const char *MqttPass() { return GetOption("MqttPass", MQTT_PASS);}
    void MqttPass(const char *_val) {SetOption("MqttPass", _val);}
    // [MqttClient]
    const char *MqttClientParsed() { static char tbuf[32];sprintf(tbuf, MqttClient(), Unit(), UID()); return tbuf;}
    const char *MqttClient() { return GetOption("MqttClient", MQTT_CLIENT);}
    void MqttClient(const char *_val) {SetOption("MqttClient", _val);}
    // [MqttTopic]
    const char *MqttTopic() { return GetOption("MqttTopic", MQTT_TOPIC);}
    void MqttTopic(const char *_val) {SetOption("MqttTopic", _val);}
    // [MqttGroupTopic]
    const char *MqttGroupTopic() { return GetOption("MqttGroupTopic", MQTT_GRPTOPIC);}
    void MqttGroupTopic(const char *_val) {SetOption("MqttGroupTopic", _val);}

    // [Prefix1]
    const char *Prefix1() { return GetOption("Prefix1", SUB_PREFIX);}
    void Prefix1(const char *_val) {SetOption("Prefix1", _val);}
    // [Prefix2]
    const char *Prefix2() { return GetOption("Prefix2", PUB_PREFIX);}
    void Prefix2(const char *_val) {SetOption("Prefix2", _val);}
    // [Prefix3]
    const char *Prefix3() { return GetOption("Prefix3", INFO_PREFIX);}
    void Prefix3(const char *_val) {SetOption("Prefix3", _val);}

  // Switches...
    // [Sw<1-8>Pin1]
    const int SwPin1(int _id) { char tb[16]; sprintf(tb, "Sw%dPin1", _id); return atoi(GetOption(tb, swPin1Defaults[_id-1]));}
    void SwPin1(int _id, int _val) { char tb[16],vb[16]; sprintf(tb, "Sw%dPin1", _id); sprintf(vb, "%d", _val); SetOption(tb, vb);}
    // [Sw<1-8>Pin2]
    const int SwPin2(int _id) { char tb[16]; sprintf(tb, "Sw%dPin2", _id); return atoi(GetOption(tb, swPin2Defaults[_id-1]));}
    void SwPin2(int _id, int _val) { char tb[16],vb[16]; sprintf(tb, "Sw%dPin2", _id); sprintf(vb, "%d", _val); SetOption(tb, vb);}
    // [Sw<1-8>Type]
    const int SwType(int _id) { char tb[16]; sprintf(tb, "Sw%dType", _id); return atoi(GetOption(tb, swTypeDefaults[_id-1]));}
    void SwType(int _id, int _val) { char tb[16],vb[16]; sprintf(tb, "Sw%dType", _id); sprintf(vb, "%d", _val); SetOption(tb, vb);}
    // [Sw<1-8>Topic]
    const char *SwTopic(int _id) { char tb[16]; sprintf(tb, "Sw%dTopic", _id); return GetOption(tb, swTopicDefaults[_id-1]);}
    void SwTopic(int _id, const char *_val) { char tb[16]; sprintf(tb, "Sw%dTopic", _id); SetOption(tb, _val);}
    // [Sw<1-8>Delay]
    const int SwDelay(int _id) { char tb[16]; sprintf(tb, "Sw%dDelay", _id); return atoi(GetOption(tb, swDelayDefaults[_id-1]));}
    void SwDelay(int _id, int _val) { char tb[16],vb[16]; sprintf(tb, "Sw%dDelay", _id); sprintf(vb, "%d", _val); SetOption(tb, vb);}

  // Rotaries
    // [Rot<n>PinU]
    const int RotPinU(int _id) { char tb[16]; sprintf(tb, "Rot%dPinU", _id); return atoi(GetOption(tb, rotUDefaults[_id-1]));}
    void RotPinU(int _id, int _val) { char tb[16],vb[16]; sprintf(tb, "Rot%dPinU", _id); sprintf(vb, "%d", _val); SetOption(tb, vb);}
    // [Rot<n>PinD]
    const int RotPinD(int _id) { char tb[16]; sprintf(tb, "Rot%dPinD", _id); return atoi(GetOption(tb, rotDDefaults[_id-1]));}
    void RotPinD(int _id, int _val) { char tb[16],vb[16]; sprintf(tb, "Rot%dPinD", _id); sprintf(vb, "%d", _val); SetOption(tb, vb);}
    // [Rot<1-4>Topic]
    const char *RotTopic(int _id) { char tb[16]; sprintf(tb, "Rot%dTopic", _id); return GetOption(tb, rotTopicDefaults[_id-1]);}
    void RotTopic(int _id, const char *_val) { char tb[16]; sprintf(tb, "Rot%dTopic", _id); SetOption(tb, _val);}


    // Outputs
    // [Out<1-8>Pin]
    const int OutPin(int _id) { char tb[16]; sprintf(tb, "Out%dPin", _id); return atoi(GetOption(tb, outDefaults[_id-1]));}
    void OutPin(int _id, int _val) { char tb[16],vb[16]; sprintf(tb, "Out%dPin", _id); sprintf(vb, "%d", _val); SetOption(tb, vb);}
    // [Out<1-8>Type]
    const int OutType(int _id) { char tb[16]; sprintf(tb, "Out%dType", _id); return atoi(GetOption(tb, outTypes[_id-1]));}
    void OutType(int _id, int _val) { char tb[16],vb[16]; sprintf(tb, "Out%dType", _id); sprintf(vb, "%d", _val); SetOption(tb, vb);}
    // These values used to remember status across reboots:
    const int Output(int _id) { char tb[16]; sprintf(tb, "Out%dVal", _id); return atoi(GetOption(tb, 0));}
    void Output(int _id, int _val) { char tb[16],vb[16]; sprintf(tb, "Out%dVal", _id); sprintf(vb, "%d", _val); SetOption(tb, vb);}


    // [Out[1-8]LED]
    const int OutLED(int _id) { char tb[16]; sprintf(tb, "Out%dLED", _id); return atoi(GetOption(tb, ledDefaults[_id-1]));}
    void OutLED(int _id, int _val) { char tb[16],vb[16]; sprintf(tb, "Out%dLED", _id); sprintf(vb, "%d", _val); SetOption(tb, vb);}
    // [LED<1-8>Topic]
    const char *LEDTopic(int _id) { char tb[16]; sprintf(tb, "LED%dTopic", _id); return GetOption(tb, LEDTopicDefaults[_id-1]);}
    void LEDTopic(int _id, const char *_val) { char tb[16]; sprintf(tb, "LED%dTopic", _id); SetOption(tb, _val);}
    // [LED<1-8>On] - value to match to turn the LED on
    const char *LEDOn(int _id) { char tb[16]; sprintf(tb, "LED%dOn", _id); return GetOption(tb, LED_ONDEFAULT);}
    void LEDOn(int _id, const char *_val) { char tb[16]; sprintf(tb, "LED%dOn", _id); SetOption(tb, _val);}
    // [LED<1-8>Flash] - value to match to mae the LED flash
    const char *LEDFlash(int _id) { char tb[16]; sprintf(tb, "LED%dFlash", _id); return GetOption(tb, LED_FLASHDEFAULT);}
    void LEDFlash(int _id, const char *_val) { char tb[16]; sprintf(tb, "LED%dFlash", _id); SetOption(tb, _val);}

    // [RGBCount]
    const int RGBCount() { return atoi(GetOption("RGBCount", RGB_COUNT));}
    void RGBCount(int _val) { char vb[8]; sprintf(vb, "%d", max(0, min(MAX_LED_COUNT, _val))); SetOption("RGBCount", vb);}
    // These values used to remember RGBS across reboots:
    const int Red() { return atoi(GetOption("Red", 128));}
    void Red(int _val) { char vb[8]; sprintf(vb, "%d", max(0, min(255, _val))); SetOption("Red", vb);}
    const int Green() { return atoi(GetOption("Green", 128));}
    void Green(int _val) { char vb[8]; sprintf(vb, "%d", max(0, min(255, _val))); SetOption("Green", vb);}
    const int Blue() { return atoi(GetOption("Blue", 128));}
    void Blue(int _val) { char vb[8]; sprintf(vb, "%d", max(0, min(255, _val))); SetOption("Blue", vb);}
    const int Bright() { return atoi(GetOption("Bright", 128));}
    void Bright(int _val) { char vb[8]; sprintf(vb, "%d", max(0, min(255, _val))); SetOption("Bright", vb);}

    // [ResetPin]
    const int ResetPin() { return atoi(GetOption("ResetPin", RESET_PIN));}
    void ResetPin(int _val) {char vb[16]; sprintf(vb,"%d", _val); SetOption("ResetPin", vb);}
    // [StatusLED]
    const int StatusLED() { return atoi(GetOption("StatusLED", STATUS_LED));}
    void StatusLED(int _val) {char vb[16]; sprintf(vb,"%d", _val); SetOption("StatusLED", vb);}
    // [InvertLED]
    bool InvertLED() { return (strcmp(GetOption("InvertLED", INVERT_LED==1?"1":"0"),"0"));}
    void InvertLED(bool _val) {SetOption("InvertLED", _val?"1":"0");}

    // [AnimSpeed]
    const int AnimSpeed() { return atoi(GetOption("AnimSpeed", 16));}
    void AnimSpeed(int _val) {SetOption("AnimSpeed", String(_val).c_str());}

    // [AnalogTopic]
    const char *AnalogTopic() { return GetOption("AnalogTopic", ANALOG_TOPIC);}
    void AnalogTopic(const char *_val) { SetOption("AnalogTopic", _val);}
    const char *AnalogTrigger() { return GetOption("AnalogTrigger", ANALOG_TRIGGER);}
    void AnalogTrigger(const char *_val) { SetOption("AnalogTrigger", _val);}
    // [ThresholdLow][ThresholdHigh][ThresholdMid]
    const int ThresholdLow() { return atoi(GetOption("ThresholdLow", 100));}
    void ThresholdLow(int _val) {SetOption("ThresholdLow", String(_val).c_str());}
    const int ThresholdMid() { return atoi(GetOption("ThresholdMid", 900));}
    void ThresholdMid(int _val) {SetOption("ThresholdMid", String(_val).c_str());}
    const int ThresholdHigh() { return atoi(GetOption("ThresholdHigh", 1100));}
    void ThresholdHigh(int _val) {SetOption("ThresholdHigh", String(_val).c_str());}

    const char *UpdateServer() { return GetOption("UpdateServer", UPDATE_URL);}
    void UpdateServer(const char *_val) { SetOption("UpdateServer", _val);}

    const char *Version() { return GetOption("Version", P_FULL_VERSION);}
    void Version(const char *_val) { SetOption("Version", _val);}
    const char *NewVersion() { return GetOption("NewVersion", P_FULL_VERSION);}
    void NewVersion(const char *_val) { SetOption("NewVersion", _val);}

    private:
    int outDefaults[NUM_OUTS]; // output pins
    int outTypes[NUM_OUTS]; // output types
    int ledDefaults[NUM_OUTS]; // output LED pins
    int swPin1Defaults[NUM_SWITCHES];
    int swPin2Defaults[NUM_SWITCHES];
    int swTypeDefaults[NUM_SWITCHES];
    int swDelayDefaults[NUM_SWITCHES];
    const char *swTopicDefaults[NUM_SWITCHES];
    const char *LEDTopicDefaults[NUM_OUTS];
    int rotUDefaults[NUM_SWITCHES];
    int rotDDefaults[NUM_SWITCHES];
    const char *rotTopicDefaults[NUM_ROTS];
    const char *friendlyDefaults[NUM_OUTS];
};


#endif //_OPTION_H_
