/*
 * HAS_Comms.h
 * 
 * (C) 2019 James Dixon
 * 
 * MQTT Pub/Sub functionality
 * Handles all the MQTT comms to the network
 * 
 */
#ifndef _COMMS_H_
#define _COMMS_H_

#include <Arduino.h>

// temp static buffer for composing payloads
extern char messageBuffer[512];

/*
 * Set up all the QTT communication topics
 */
void setupMQTTMessages();

/*
 * Handle all incoming traffic
 */
void MQTTcallback(char* topic, byte* payload, unsigned int length);


#endif // _COMMS_H_
